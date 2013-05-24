/*
 * $Id$
 *
 * (C) 2009-2013 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioParser.h"
#include <ks.h>
#include <ksmedia.h>
#include "AudioHelper.h"
#include "MpcAudioRenderer.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&GUID_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpcAudioRenderer), MpcAudioRendererName, 0x40000001, _countof(sudpPins), sudpPins, CLSID_AudioRendererCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, &__uuidof(CMpcAudioRenderer), CreateInstance<CMpcAudioRenderer>, NULL, &sudFilter[0]},
	{L"CMpcAudioRendererPropertyPage", &__uuidof(CMpcAudioRendererSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd> >},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../core/FilterApp.h"

CFilterApp theApp;

#endif

static GUID lpSoundGUID = DSDEVID_DefaultPlayback;

bool CALLBACK DSEnumProc2(LPGUID lpGUID,
						  LPCTSTR lpszDesc,
						  LPCTSTR lpszDrvName,
						  LPVOID lpContext )
{
	CString *pStr = (CString *)lpContext;
	ASSERT ( pStr );
	CString strGUID = *pStr;

	if (lpGUID != NULL) { // NULL only for "Primary Sound Driver".
		if (strGUID == lpszDesc) {
			memcpy((VOID *)&lpSoundGUID, lpGUID, sizeof(GUID));
		}
	}

	return TRUE;
}

CMpcAudioRenderer::CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr)
	: CBaseRenderer(__uuidof(this), NAME("CMpcAudioRenderer"), punk, phr)
	, m_pDSBuffer		(NULL)
	, m_pSoundTouch		(NULL)
	, m_pDS				(NULL)
	, m_dwDSWriteOff	(0)
	, m_nDSBufSize		(0)
	, m_dRate			(1.0)
	, m_pReferenceClock	(NULL)
	, m_pWaveFileFormat	(NULL)
	, m_pWaveFileFormatOutput (NULL)
	, pMMDevice			(NULL)
	, pAudioClient		(NULL)
	, pRenderClient		(NULL)
	, m_useWASAPI		(2)
	, m_bMuteFastForward(false)
	, nFramesInBuffer	(0)
	, hnsPeriod			(0)
	, hTask				(NULL)
	, m_nBufferSize		(0)
	, isAudioClientStarted (false)
	, lastBufferTime	(0)
	, hnsActualDuration (0)
	, m_lVolume			(DSBVOLUME_MIN)
	, m_dVolume			(0.0)
	, m_nThreadId		(0)
	, m_hRenderThread	(NULL)
	, m_bThreadPaused	(FALSE)
	, m_hDataEvent		(NULL)
	, m_hPauseEvent		(NULL)
	, m_hWaitPauseEvent	(NULL)
	, m_hResumeEvent	(NULL)
	, m_hWaitResumeEvent(NULL)
	, m_hStopRenderThreadEvent(NULL)

{
	TRACE(_T("CMpcAudioRenderer::CMpcAudioRenderer()\n"));

	HMODULE hLib = NULL;

#ifdef REGISTER_FILTER
	CRegKey key;
	TCHAR buff[256];
	ULONG len;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\MPC-BE Filters\\MPC Audio Renderer"), KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("UseWasapi"), dw)) {
			m_useWASAPI = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("MuteFastForward"), dw)) {
			m_bMuteFastForward = !!dw;
		}
		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(_T("SoundDevice"), buff, &len)) {
			m_csSound_Device = CString(buff);
		}
	}
#else
	m_useWASAPI			= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Audio Renderer"), _T("UseWasapi"), m_useWASAPI);
	m_bMuteFastForward	= !!AfxGetApp()->GetProfileInt(_T("Filters\\MPC Audio Renderer"), _T("MuteFastForward"), m_bMuteFastForward);
	m_csSound_Device	= AfxGetApp()->GetProfileString(_T("Filters\\MPC Audio Renderer"), _T("SoundDevice"), m_csSound_Device);
#endif

	m_useWASAPI				= (max(0, min(m_useWASAPI, 2)));
	m_useWASAPIAfterRestart	= m_useWASAPI;

	// Load Vista & above specifics DLLs
	hLib = LoadLibrary (L"AVRT.dll");
	if (hLib != NULL) {
		pfAvSetMmThreadCharacteristicsW		= (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW");
		pfAvRevertMmThreadCharacteristics	= (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress (hLib, "AvRevertMmThreadCharacteristics");
	} else {
		m_useWASAPI = false; // Wasapi not available below Vista
	}

	if (!m_useWASAPI) {
		DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc2, (VOID*)&m_csSound_Device);
		m_pSoundTouch = DNew soundtouch::SoundTouch();
		*phr = DirectSoundCreate8 (&lpSoundGUID, &m_pDS, NULL);
	}

	if (m_useWASAPI) {
		m_hDataEvent				= CreateEvent(NULL, FALSE, FALSE, NULL);	

		m_hPauseEvent				= CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hWaitPauseEvent			= CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hResumeEvent				= CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hWaitResumeEvent			= CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hStopRenderThreadEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);
	}
}

CMpcAudioRenderer::~CMpcAudioRenderer()
{
	TRACE(_T("CMpcAudioRenderer::~CMpcAudioRenderer()\n"));

	Stop();
	StopAudioClient();
	StopRendererThread();

	if (m_hStopRenderThreadEvent) {
		CloseHandle(m_hStopRenderThreadEvent);
	}
	if (m_hDataEvent) {
		CloseHandle(m_hDataEvent);
	}
	if (m_hPauseEvent) {
		CloseHandle(m_hPauseEvent);
	}
	if (m_hWaitPauseEvent) {
		CloseHandle(m_hWaitPauseEvent);
	}
	if (m_hResumeEvent) {
		CloseHandle(m_hResumeEvent);
	}
	if (m_hWaitResumeEvent) {
		CloseHandle(m_hWaitResumeEvent);
	}

	SAFE_DELETE  (m_pSoundTouch);
	SAFE_RELEASE (m_pDSBuffer);
	SAFE_RELEASE (m_pDS);

	SAFE_RELEASE (pRenderClient);
	SAFE_RELEASE (pAudioClient);
	SAFE_RELEASE (pMMDevice);

	if (m_pReferenceClock) {
		SetSyncSource(NULL);
		SAFE_RELEASE (m_pReferenceClock);
	}

	SAFE_DELETE_ARRAY(m_pWaveFileFormat);
	SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);

	if (hTask != NULL && pfAvRevertMmThreadCharacteristics != NULL) {
		pfAvRevertMmThreadCharacteristics(hTask);
	}
}

HRESULT CMpcAudioRenderer::CheckInputType(const CMediaType *pmt)
{
	return CheckMediaType(pmt);
}

HRESULT	CMpcAudioRenderer::CheckMediaType(const CMediaType *pmt)
{
	HRESULT hr = S_OK;
	if (pmt == NULL) {
		return E_INVALIDARG;
	}

	TRACE(_T("CMpcAudioRenderer::CheckMediaType()\n"));
	if (pmt->subtype != MEDIASUBTYPE_PCM && pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT) {
		TRACE(_T("CMpcAudioRenderer::CheckMediaType() - allow only PCM/IEEE_FLOAT input\n"));
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pmt->Format();

	if (pwfx == NULL) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if ((pmt->majortype != MEDIATYPE_Audio) || (pmt->formattype != FORMAT_WaveFormatEx)) {
		TRACE(_T("CMpcAudioRenderer::CheckMediaType() - Not supported\n"));
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (m_useWASAPI) {
		hr = CheckAudioClient((WAVEFORMATEX *)NULL);
		if (FAILED(hr)) {
			TRACE(_T("CMpcAudioRenderer::CheckMediaType() - Error on check audio client\n"));
			return hr;
		}
		if (!pAudioClient) {
			TRACE(_T("CMpcAudioRenderer::CheckMediaType() - Error, audio client not loaded\n"));
			return VFW_E_CANNOT_CONNECT;
		}

		CopyWaveFormat(pwfx, &m_pWaveFileFormatOutput);

		WAVEFORMATEX *pFormat		= NULL;
		WAVEFORMATEX* pDeviceFormat	= NULL;

		WAVEFORMATEX *sharedClosestMatch	= NULL;
		HRESULT hr							= VFW_E_TYPE_NOT_ACCEPTED;

		IPropertyStore *pProps = NULL;
		PROPVARIANT varConfig;

		if (m_useWASAPIAfterRestart == 1 || IsBitstream(pwfx)) { // EXCLUSIVE
			/*
			if (IsBitstream(pwfx)) {
				pFormat = pwfx;
			} else {
				pMMDevice->OpenPropertyStore(STGM_READ, &pProps);
				PropVariantInit(&varConfig);
				hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varConfig);
				if (SUCCEEDED(hr) && varConfig.vt == VT_BLOB && varConfig.blob.pBlobData != NULL) {
					pFormat = (WAVEFORMATEX*)varConfig.blob.pBlobData;
				}
			}
			*/
			pFormat = pwfx;

			hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pFormat, NULL);
			if (S_OK == hr) {
				CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
			}
		} else if (m_useWASAPIAfterRestart == 2) { // SHARED
			pFormat = pwfx;
			hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
			if (FAILED(hr)) {
				hr = pAudioClient->GetMixFormat(&pDeviceFormat);
				if (FAILED(hr)) {
					TRACE(_T("CMpcAudioRenderer::CheckMediaType() - GetMixFormat() failed\n"));
					return hr;
				}

				pFormat = pDeviceFormat;
				hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
				}
			}
		}
		TRACE(_T("CMpcAudioRenderer::CheckMediaType() - IsFormatSupported()\n"));
		TRACE(_T("	Input format:\n"));
		TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	pwfx->wFormatTag);
		TRACE(_T("		=> nChannels		= %d\n"),		pwfx->nChannels);
		TRACE(_T("		=> nSamplesPerSec	= %d\n"),		pwfx->nSamplesPerSec);
		TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		pwfx->nAvgBytesPerSec);
		TRACE(_T("		=> nBlockAlign		= %d\n"),		pwfx->nBlockAlign);
		TRACE(_T("		=> wBitsPerSample	= %d\n"),		pwfx->wBitsPerSample);
		TRACE(_T("		=> cbSize			= %d\n"),		pwfx->cbSize);
		TRACE(_T("	Output format:\n"));
		TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	pFormat->wFormatTag);
		TRACE(_T("		=> nChannels		= %d\n"),		pFormat->nChannels);
		TRACE(_T("		=> nSamplesPerSec	= %d\n"),		pFormat->nSamplesPerSec);
		TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		pFormat->nAvgBytesPerSec);
		TRACE(_T("		=> nBlockAlign		= %d\n"),		pFormat->nBlockAlign);
		TRACE(_T("		=> wBitsPerSample	= %d\n"),		pFormat->wBitsPerSample);
		TRACE(_T("		=> cbSize			= %d\n"),		pFormat->cbSize);

		if (pProps) {
			PropVariantClear(&varConfig);
			SAFE_RELEASE(pProps);
		}

		if (pDeviceFormat) {
			CoTaskMemFree(pDeviceFormat);
		}

		if (S_OK == hr) {
			TRACE(_T("CMpcAudioRenderer::CheckMediaType() - WASAPI client accepted the format\n"));
		} else {
			if (S_FALSE == hr) {
				TRACE(_T("CMpcAudioRenderer::CheckMediaType() - WASAPI client refused the format with a closest match\n"));
			} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
				TRACE(_T("CMpcAudioRenderer::CheckMediaType() - WASAPI client refused the format\n"));
			} else {
				TRACE(_T("CMpcAudioRenderer::CheckMediaType() - WASAPI failed = 0x%08x\n"), hr);
			}

			if (sharedClosestMatch) {
				TRACE(_T("	=> ppClosestMatch:\n"));
				TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	sharedClosestMatch->wFormatTag);
				TRACE(_T("		=> nChannels		= %d\n"),		sharedClosestMatch->nChannels);
				TRACE(_T("		=> nSamplesPerSec	= %d\n"),		sharedClosestMatch->nSamplesPerSec);
				TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		sharedClosestMatch->nAvgBytesPerSec);
				TRACE(_T("		=> nBlockAlign		= %d\n"),		sharedClosestMatch->nBlockAlign);
				TRACE(_T("		=> wBitsPerSample	= %d\n"),		sharedClosestMatch->wBitsPerSample);
				TRACE(_T("		=> cbSize			= %d\n"),		sharedClosestMatch->cbSize);

				CopyWaveFormat(sharedClosestMatch, &m_pWaveFileFormatOutput);
				CoTaskMemFree(sharedClosestMatch);

				hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, m_pWaveFileFormatOutput, &sharedClosestMatch);
				if (S_OK == hr) {
					TRACE(_T("CMpcAudioRenderer::CheckMediaType() - WASAPI client accepted the closest match format\n"));
					return hr;
				}
				if (sharedClosestMatch) {
					CoTaskMemFree(sharedClosestMatch);
				}
			}

			SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} else if (pwfx->wFormatTag != WAVE_FORMAT_PCM && pwfx->wFormatTag != WAVE_FORMAT_IEEE_FLOAT) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	return S_OK;
}

void CMpcAudioRenderer::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
	if (!m_useWASAPI) {
		ClearBuffer();
	}
}

BOOL CMpcAudioRenderer::ScheduleSample(IMediaSample *pMediaSample)
{
	REFERENCE_TIME	StartSample;
	REFERENCE_TIME	EndSample;

	// Is someone pulling our leg
	if (pMediaSample == NULL) {
		return FALSE;
	}

	// Get the next sample due up for rendering.  If there aren't any ready
	// then GetNextSampleTimes returns an error.  If there is one to be done
	// then it succeeds and yields the sample times. If it is due now then
	// it returns S_OK other if it's to be done when due it returns S_FALSE
	HRESULT hr = GetSampleTimes(pMediaSample, &StartSample, &EndSample);
	if (FAILED(hr)) {
		return FALSE;
	}

	// If we don't have a reference clock then we cannot set up the advise
	// time so we simply set the event indicating an image to render. This
	// will cause us to run flat out without any timing or synchronisation
	if (hr == S_OK) {
		EXECUTE_ASSERT(SetEvent((HANDLE) m_RenderEvent));
		return TRUE;
	}

	if (m_dRate <= 1.1) {
		ASSERT(m_dwAdvise == 0);
		ASSERT(m_pClock);
		WaitForSingleObject((HANDLE)m_RenderEvent,0);

		hr = m_pClock->AdviseTime( (REFERENCE_TIME) m_tStart, StartSample, (HEVENT)(HANDLE) m_RenderEvent, &m_dwAdvise);
		if (SUCCEEDED(hr)) {
			return TRUE;
		}
	} else {
		hr = DoRenderSample (pMediaSample);
	}

	// We could not schedule the next sample for rendering despite the fact
	// we have a valid sample here. This is a fair indication that either
	// the system clock is wrong or the time stamp for the sample is duff
	ASSERT(m_dwAdvise == 0);

	return FALSE;
}

HRESULT	CMpcAudioRenderer::DoRenderSample(IMediaSample *pMediaSample)
{
	if (m_useWASAPI) {
		return DoRenderSampleWasapi(pMediaSample);
	} else {
		return DoRenderSampleDirectSound(pMediaSample);
	}
}

STDMETHODIMP CMpcAudioRenderer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IReferenceClock) {
		return GetReferenceClockInterface(riid, ppv);
	} else if (riid == IID_IDispatch) {
		return GetInterface(static_cast<IDispatch*>(this), ppv);
	} else if (riid == IID_IBasicAudio) {
		return GetInterface(static_cast<IBasicAudio*>(this), ppv);
	} else if (riid == __uuidof(ISpecifyPropertyPages)) {
		return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
	} else if (riid == __uuidof(ISpecifyPropertyPages2)) {
		return GetInterface(static_cast<ISpecifyPropertyPages2*>(this), ppv);
	} else if (riid == __uuidof(IMpcAudioRendererFilter)) {
		return GetInterface(static_cast<IMpcAudioRendererFilter*>(this), ppv);
	}

	return CBaseRenderer::NonDelegatingQueryInterface (riid, ppv);
}

HRESULT CMpcAudioRenderer::SetMediaType(const CMediaType *pmt)
{
	if (!pmt) {
		return E_POINTER;
	}
	TRACE(_T("CMpcAudioRenderer::SetMediaType\n"));

	if (m_useWASAPI) {
		// New media type set but render client already initialized => reset it
		if (pRenderClient != NULL) {
			WAVEFORMATEX *pNewWf = (WAVEFORMATEX *)pmt->Format();
			TRACE(_T("CMpcAudioRenderer::SetMediaType() : Render client already initialized. Reinitialization...\n"));
			CheckAudioClient(pNewWf);
		}
	}

	SAFE_DELETE_ARRAY(m_pWaveFileFormat);

	WAVEFORMATEX *pwf = (WAVEFORMATEX *)pmt->Format();
	if (pwf != NULL) {
		CopyWaveFormat(pwf, &m_pWaveFileFormat);

		if (!m_useWASAPI && m_pSoundTouch && (pwf->nChannels <= 2)) {
			m_pSoundTouch->setSampleRate (pwf->nSamplesPerSec);
			m_pSoundTouch->setChannels (pwf->nChannels);
			m_pSoundTouch->setTempoChange (0);
			m_pSoundTouch->setPitchSemiTones(0);
		}
	}

	return CBaseRenderer::SetMediaType(pmt);
}

HRESULT CMpcAudioRenderer::CompleteConnect(IPin *pReceivePin)
{
	HRESULT hr = S_OK;
	TRACE(_T("CMpcAudioRenderer::CompleteConnect()\n"));

	if (!m_useWASAPI && !m_pDS) {
		return E_FAIL;
	}

	if (SUCCEEDED(hr)) {
		hr = CBaseRenderer::CompleteConnect(pReceivePin);
	}
	if (SUCCEEDED(hr)) {
		hr = InitCoopLevel();
	}

	if (!m_useWASAPI) {
		if (SUCCEEDED(hr)) {
			hr = CreateDSBuffer();
		}
	}
	if (SUCCEEDED(hr)) {
		TRACE(_T("CMpcAudioRenderer::CompleteConnect() - Success\n"));
	}
	return hr;
}

HRESULT CMpcAudioRenderer::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMpcAudioRenderer::EndFlush()
{
	CAutoLock cAutoLock(&m_csRender);

	StopAudioClient();
	m_WasapiBuf.RemoveAll();

	return __super::EndFlush();
}

DWORD WINAPI CMpcAudioRenderer::RenderThreadEntryPoint(LPVOID lpParameter)
{
  return ((CMpcAudioRenderer*)lpParameter)->RenderThread();
}


HRESULT CMpcAudioRenderer::StopRendererThread()
{
	if (m_hRenderThread) {
		SetEvent(m_hStopRenderThreadEvent);
		if (WaitForSingleObject(m_hRenderThread, 10000) == WAIT_TIMEOUT) {
			TerminateThread(m_hRenderThread, 0xDEAD);
		}

		CloseHandle(m_hRenderThread);
		m_hRenderThread = NULL;
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::StartRendererThread()
{
	if (!m_hRenderThread) {
		m_hRenderThread = CreateThread(0, 0, CMpcAudioRenderer::RenderThreadEntryPoint, (LPVOID)this, 0, &m_nThreadId);

		if (m_hRenderThread) {
			return S_OK;
		} else {
			return S_FALSE;
		}
	} else {
		if (m_bThreadPaused) {
			SetEvent(m_hResumeEvent);
			WaitForSingleObject(m_hWaitResumeEvent, INFINITE);
		}
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::PauseRendererThread()
{
	if (m_bThreadPaused) {
		return S_OK;
	}

	// Pause the render thread
	if (m_hRenderThread) {

		SetEvent(m_hPauseEvent);
		DWORD result = WaitForSingleObject(m_hWaitPauseEvent, INFINITE);
		if (result != 0) {
			DWORD error = GetLastError();
		}
	}
	else {
		return S_FALSE;
	}

	return S_OK;
}

template<class T>
T clamp(double s, T smin, T smax)
{
	if (s < -1) {
		s = -1;
	} else if (s > 1) {
		s = 1;
	}
	T t = static_cast<T>(s * smax);
	if (t < smin) {
		t = smin;
	} else if (t > smax) {
		t = smax;
	}
	return t;
}

DWORD CMpcAudioRenderer::RenderThread()
{
	HRESULT hr = S_OK;

	// These are wait handles for the thread stopping, new sample arrival and pausing redering
	HANDLE renderHandles[3] = {
		m_hStopRenderThreadEvent,
		m_hPauseEvent,
		m_hDataEvent
	};

	/*
	handles[0] = m_hStopRenderThreadEvent;
	handles[1] = m_hPauseEvent;
	handles[2] = m_hDataEvent;
	*/

	// These are wait handles for resuming or exiting the thread
	HANDLE resumeHandles[2] = {
		m_hStopRenderThreadEvent,
		m_hResumeEvent
	};
	/*
	resumeHandles[0] = m_hStopRenderThreadEvent;
	resumeHandles[1] = m_hResumeEvent;
	*/

	while (true) {
		// 1) Waiting for the next WASAPI buffer to be available to be filled
		// 2) Exit requested for the thread
		// 3) For a pause request
		DWORD result = WaitForMultipleObjects(3, renderHandles, FALSE, INFINITE);
		switch (result) {
			case WAIT_OBJECT_0:
				return 0;
			case WAIT_OBJECT_0 + 1: 
				{
					m_bThreadPaused = TRUE;
					ResetEvent(m_hResumeEvent);
					SetEvent(m_hWaitPauseEvent);

					DWORD resultResume = WaitForMultipleObjects(2, resumeHandles, FALSE, INFINITE);
					if (resultResume == WAIT_OBJECT_0) { // exit event
						return 0;
					}

					if (resultResume == WAIT_OBJECT_0 + 1) { // resume event
						m_bThreadPaused = FALSE;
						SetEvent(m_hWaitResumeEvent);
					}
				}
				break;
			case WAIT_OBJECT_0 + 2:
				{
					CAutoLock cAutoLock(&m_csRender);

					if (/*isAudioClientStarted*/TRUE) {
						//hr = pAudioClient->GetBufferSize(&nFramesInBuffer);

						UINT32 numFramesPadding = 0;
						if (m_useWASAPIAfterRestart == 2 && !IsBitstream(m_pWaveFileFormatOutput)) { // EXCLUSIVE
							pAudioClient->GetCurrentPadding(&numFramesPadding);
						}
						UINT32 numFramesAvailable	= nFramesInBuffer - numFramesPadding;
						UINT32 nAvailableBytes		= numFramesAvailable * m_pWaveFileFormatOutput->nBlockAlign;

						BYTE* pData = NULL;
						hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
						if (FAILED(hr)) {
							TRACE(_T("CMpcAudioRenderer::RenderThread() - GetBuffer failed with size %ld : (error %lx)\n"), nFramesInBuffer, hr);
							break;
						}

						DWORD bufferFlags = 0;
						if (nAvailableBytes > m_WasapiBuf.GetCount()) {
							bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
						} else {
							BYTE* const base = m_WasapiBuf.GetData();
							BYTE* const end = base + m_WasapiBuf.GetCount();
							BYTE* p = base;

							if (pData != NULL) {
								memcpy(&pData[0], p, nAvailableBytes);
								
								if (!IsBitstream(m_pWaveFileFormat) && (m_dVolume >= 0.0 && m_dVolume < 1.0)) {
									// Adjusting volume ...
									WAVEFORMATEX* wfeOutput				= (WAVEFORMATEX*)m_pWaveFileFormatOutput;
									WAVEFORMATEXTENSIBLE* wfexOutput	= (WAVEFORMATEXTENSIBLE*)wfeOutput;

									WORD tag	= wfeOutput->wFormatTag;
									bool fPCM	= tag == WAVE_FORMAT_PCM || tag == WAVE_FORMAT_EXTENSIBLE && wfexOutput->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;
									bool fFloat	= tag == WAVE_FORMAT_IEEE_FLOAT || tag == WAVE_FORMAT_EXTENSIBLE && wfexOutput->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

									int samples	= nAvailableBytes / (wfeOutput->wBitsPerSample >> 3);

									for (int i = 0; i < samples; i++) {
										if (fPCM && wfeOutput->wBitsPerSample == 8) {
											double s = reinterpret_cast<double*>(pData)[i] / UCHAR_MAX;
											s *= m_dVolume;
											pData[i] = clamp<BYTE>(s, 0, UCHAR_MAX);
										} else if (fPCM && wfeOutput->wBitsPerSample == 16) {
											double s = static_cast<double>(reinterpret_cast<short*>(pData)[i]) / SHRT_MAX;
											s *= m_dVolume;
											reinterpret_cast<short*>(pData)[i] = clamp<short>(s, SHRT_MIN, SHRT_MAX);
										} else if (fPCM && wfeOutput->wBitsPerSample == 24) {
											int tmp;
											memcpy((reinterpret_cast<BYTE*>(&tmp))+1, &pData[i*3], 3);
											double s = static_cast<double>(static_cast<float>(tmp >> 8) / INT24_MAX);
											s *= m_dVolume;
											tmp = clamp<int>(s, INT24_MIN, INT24_MAX);
											memcpy(&pData[i*3], reinterpret_cast<BYTE*>(&tmp), 3);
										} else if (fPCM && wfeOutput->wBitsPerSample == 32) {
											double s = static_cast<double>(reinterpret_cast<int*>(pData)[i]) / INT_MAX;
											s *= m_dVolume;
											reinterpret_cast<int*>(pData)[i] = clamp<int>(s, INT_MIN, INT_MAX);
										} else if (fFloat && wfeOutput->wBitsPerSample == 32) {
											double s = static_cast<double>(reinterpret_cast<float*>(pData)[i]);
											s *= m_dVolume;
											reinterpret_cast<float*>(pData)[i] = clamp<float>(s, -1, +1);
										} else if (fFloat && wfeOutput->wBitsPerSample == 64) {
											double s = reinterpret_cast<double*>(pData)[i];
											s *= m_dVolume;
											reinterpret_cast<double*>(pData)[i] = clamp<double>(s, -1, +1);
										}
									}
								}

								p += nAvailableBytes;

								memmove(base, p, end - p);
								m_WasapiBuf.SetCount(end - p);
							}
						}
				
						hr = pRenderClient->ReleaseBuffer(numFramesAvailable, bufferFlags);
						if (FAILED(hr)) {
							TRACE(_T("CMpcAudioRenderer::RenderThread() - ReleaseBuffer failed with size %ld (error %lx)\n"), nFramesInBuffer, hr);
							break;
						}
					}
				}
				break;
			default:
				TRACE(L"CMpcAudioRenderer::RenderThread() - WaitForMultipleObjects() failed: %d\n", GetLastError());
				break;
		}
	}

	return 0;
}

STDMETHODIMP CMpcAudioRenderer::Run(REFERENCE_TIME tStart)
{
	TRACE(_T("CMpcAudioRenderer::Run()\n"));

	HRESULT hr;

	if (m_State == State_Running) {
		return NOERROR;
	}

	if (m_useWASAPI) {
		hr = CheckAudioClient(m_pWaveFileFormat);
		if (FAILED(hr)) {
			TRACE(_T("CMpcAudioRenderer::Run() - Error on check audio client\n"));
			return hr;
		}
	} else {
		if (m_pDSBuffer && m_pPosition && m_pWaveFileFormat && SUCCEEDED(m_pPosition->GetRate(&m_dRate))) {
			if (m_dRate < 1.0) {
				hr = m_pDSBuffer->SetFrequency ((long)(m_pWaveFileFormat->nSamplesPerSec * m_dRate));
				if (FAILED(hr)) {
					return hr;
				}
			} else {
				hr = m_pDSBuffer->SetFrequency ((long)m_pWaveFileFormat->nSamplesPerSec);
				m_pSoundTouch->setRateChange((float)(m_dRate-1.0)*100);

				if (m_bMuteFastForward) {
					if (m_dRate == 1.0) {
						m_pDSBuffer->SetVolume(m_lVolume);
					} else {
						m_pDSBuffer->SetVolume(DSBVOLUME_MIN);
					}
				}
			}
		}

		ClearBuffer();
	}
	hr = CBaseRenderer::Run(tStart);

	return hr;
}

STDMETHODIMP CMpcAudioRenderer::Stop()
{
	TRACE(_T("CMpcAudioRenderer::Stop()\n"));
	if (m_pDSBuffer) {
		m_pDSBuffer->Stop();
	}

	return CBaseRenderer::Stop();
};

STDMETHODIMP CMpcAudioRenderer::Pause()
{
	TRACE(_T("CMpcAudioRenderer::Pause()\n"));
	if (m_pDSBuffer) {
		m_pDSBuffer->Stop();
	}

	return CBaseRenderer::Pause();
};

// === IDispatch
STDMETHODIMP CMpcAudioRenderer::GetTypeInfoCount(UINT * pctinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CMpcAudioRenderer::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CMpcAudioRenderer::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	return E_NOTIMPL;
}

STDMETHODIMP CMpcAudioRenderer::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	return E_NOTIMPL;
}

// === IBasicAudio
STDMETHODIMP CMpcAudioRenderer::put_Volume(long lVolume)
{
	m_lVolume = lVolume;

	if (m_useWASAPI) {
		m_dVolume = pow(10, m_lVolume / 4000.0f);
		if (m_lVolume == -10000) {
			m_dVolume = 0.0;
		} else if (m_lVolume == 0) {
			m_dVolume = 1.0;
		}
	} else if (m_pDSBuffer) {
		return m_pDSBuffer->SetVolume(lVolume);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::get_Volume(long *plVolume)
{
	if (!m_useWASAPI && m_pDSBuffer) {
		return m_pDSBuffer->GetVolume(plVolume);
	}

	*plVolume = m_lVolume;

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::put_Balance(long lBalance)
{
	if (!m_useWASAPI && m_pDSBuffer) {
		return m_pDSBuffer->SetPan(lBalance);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::get_Balance(long *plBalance)
{
	if (!m_useWASAPI && m_pDSBuffer) {
		return m_pDSBuffer->GetPan(plBalance);
	}

	return S_OK;
}

// === ISpecifyPropertyPages2
STDMETHODIMP CMpcAudioRenderer::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems		= 1;

	pPages->pElems		= (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0]	= __uuidof(CMpcAudioRendererSettingsWnd);

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpcAudioRendererSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// === IMpcAudioRendererFilter
STDMETHODIMP CMpcAudioRenderer::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, _T("Software\\MPC-BE Filters\\MPC Audio Renderer"))) {
		key.SetDWORDValue(_T("UseWasapi"), m_useWASAPIAfterRestart);
		key.SetDWORDValue(_T("MuteFastForward"), m_bMuteFastForward);
		key.SetStringValue(_T("SoundDevice"), m_csSound_Device);
	}
#else
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Audio Renderer"), _T("UseWasapi"), m_useWASAPIAfterRestart);
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Audio Renderer"), _T("MuteFastForward"), m_bMuteFastForward);
	AfxGetApp()->WriteProfileString(_T("Filters\\MPC Audio Renderer"), _T("SoundDevice"), m_csSound_Device);
#endif

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::SetWasapiMode(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_useWASAPIAfterRestart = nValue;
	return S_OK;
}
STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetWasapiMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_useWASAPIAfterRestart;
}

STDMETHODIMP CMpcAudioRenderer::SetMuteFastForward(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bMuteFastForward = !!nValue;
	return S_OK;
}
STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetMuteFastForward()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bMuteFastForward;
}

STDMETHODIMP CMpcAudioRenderer::SetSoundDevice(CString nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_csSound_Device = nValue;
	return S_OK;
}
STDMETHODIMP_(CString) CMpcAudioRenderer::GetSoundDevice()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_csSound_Device;
}

STDMETHODIMP_(UINT) CMpcAudioRenderer::GetMode()
{
	CAutoLock cAutoLock(&m_csProps);

	if (!m_pGraph) {
		return MODE_NONE;
	}

	if (m_useWASAPI) {
		if (m_useWASAPIAfterRestart == 1 || IsBitstream(m_pWaveFileFormat)) {
			return MODE_WASAPI_EXCLUSIVE;
		} else {
			return MODE_WASAPI_SHARED;
		}
	} else if (m_pDSBuffer) {
		return MODE_DIRECTSOUND;
	}

	return MODE_NONE;
}

HRESULT CMpcAudioRenderer::GetReferenceClockInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	if (m_pReferenceClock) {
		return m_pReferenceClock->NonDelegatingQueryInterface(riid, ppv);
	}

	m_pReferenceClock = DNew CBaseReferenceClock (NAME("Mpc Audio Clock"), NULL, &hr);
	if (! m_pReferenceClock) {
		return E_OUTOFMEMORY;
	}

	m_pReferenceClock->AddRef();

	hr = SetSyncSource(m_pReferenceClock);
	if (FAILED(hr)) {
		SetSyncSource(NULL);
		return hr;
	}

	return GetReferenceClockInterface(riid, ppv);
}

HRESULT CMpcAudioRenderer::EndOfStream(void)
{
	HRESULT hr = S_OK;

	if (m_pDSBuffer) {
		hr = m_pDSBuffer->Stop();
	}
#if !FILEWRITER
	hr = StopAudioClient();
#endif
	isAudioClientStarted = false;

	return CBaseRenderer::EndOfStream();
}


#pragma region 
// ==== DirectSound

HRESULT CMpcAudioRenderer::CreateDSBuffer()
{
	if (!m_pWaveFileFormat) {
		return E_POINTER;
	}

	HRESULT					hr				= S_OK;
	LPDIRECTSOUNDBUFFER		pDSBPrimary		= NULL;
	DSBUFFERDESC			dsbd;
	DSBUFFERDESC			cDSBufferDesc;
	DSBCAPS					bufferCaps;
	DWORD					dwDSBufSize		= m_pWaveFileFormat->nAvgBytesPerSec * 4;

	ZeroMemory(&bufferCaps, sizeof(bufferCaps));
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));

	dsbd.dwSize			= sizeof(DSBUFFERDESC);
	dsbd.dwFlags		= DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes	= 0;
	dsbd.lpwfxFormat	= NULL;
	if (SUCCEEDED(hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ))) {
		hr = pDSBPrimary->SetFormat(m_pWaveFileFormat);
		ATLASSERT(SUCCEEDED(hr));
		SAFE_RELEASE (pDSBPrimary);
	}


	SAFE_RELEASE (m_pDSBuffer);
	cDSBufferDesc.dwSize			= sizeof (DSBUFFERDESC);
	cDSBufferDesc.dwFlags			= DSBCAPS_GLOBALFOCUS			|
									  DSBCAPS_GETCURRENTPOSITION2	|
									  DSBCAPS_CTRLVOLUME 			|
									  DSBCAPS_CTRLPAN				|
									  DSBCAPS_CTRLFREQUENCY;
	cDSBufferDesc.dwBufferBytes		= dwDSBufSize;
	cDSBufferDesc.dwReserved		= 0;
	cDSBufferDesc.lpwfxFormat		= m_pWaveFileFormat;
	cDSBufferDesc.guid3DAlgorithm	= GUID_NULL;

	hr = m_pDS->CreateSoundBuffer (&cDSBufferDesc,  &m_pDSBuffer, NULL);

	m_nDSBufSize = 0;
	if (SUCCEEDED(hr)) {
		bufferCaps.dwSize = sizeof(bufferCaps);
		hr = m_pDSBuffer->GetCaps(&bufferCaps);
	}
	if (SUCCEEDED(hr)) {
		m_nDSBufSize = bufferCaps.dwBufferBytes;
		hr = ClearBuffer();
		m_pDSBuffer->SetFrequency ((long)(m_pWaveFileFormat->nSamplesPerSec * m_dRate));
	}

	return hr;
}

HRESULT CMpcAudioRenderer::ClearBuffer()
{
	HRESULT	hr				= S_FALSE;
	VOID*	pDSLockedBuffer	= NULL;
	DWORD	dwDSLockedSize	= 0;

	if (m_pDSBuffer) {
		m_dwDSWriteOff = 0;
		m_pDSBuffer->SetCurrentPosition(0);

		hr = m_pDSBuffer->Lock (0, 0, &pDSLockedBuffer, &dwDSLockedSize, NULL, NULL, DSBLOCK_ENTIREBUFFER);
		if (SUCCEEDED(hr)) {
			memset (pDSLockedBuffer, 0, dwDSLockedSize);
			hr = m_pDSBuffer->Unlock (pDSLockedBuffer, dwDSLockedSize, NULL, NULL);
		}
	}

	return hr;
}

HRESULT CMpcAudioRenderer::InitCoopLevel()
{
	HRESULT			hr				= S_OK;
	IVideoWindow*	pVideoWindow	= NULL;
	HWND			hWnd			= NULL;
	CComBSTR		bstrCaption;

	hr = m_pGraph->QueryInterface (__uuidof(IVideoWindow), (void**) &pVideoWindow);
	if (SUCCEEDED(hr)) {
		pVideoWindow->get_Owner((OAHWND*)&hWnd);
		SAFE_RELEASE (pVideoWindow);
	}
	if (!hWnd) {
		hWnd = GetTopWindow(NULL);
	}

	ATLASSERT(hWnd != NULL);
	if (!m_useWASAPI) {
		hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	} else if (hTask == NULL) {
		// Ask MMCSS to temporarily boost the thread priority
		// to reduce glitches while the low-latency stream plays.
		DWORD taskIndex = 0;

		if (pfAvSetMmThreadCharacteristicsW) {
			hTask = pfAvSetMmThreadCharacteristicsW(_T("Pro Audio"), &taskIndex);
			TRACE(_T("CMpcAudioRenderer::InitCoopLevel() - Putting thread in higher priority for Wasapi mode (lowest latency)\n"));
			hr = GetLastError();
			if (hTask == NULL) {
				return hr;
			}
		}
	}

	return hr;
}

HRESULT	CMpcAudioRenderer::DoRenderSampleDirectSound(IMediaSample *pMediaSample)
{
	HRESULT		hr					= S_OK;
	DWORD		dwStatus			= 0;
	const long	lSize				= pMediaSample->GetActualDataLength();
	DWORD		dwPlayCursor		= 0;
	DWORD		dwWriteCursor		= 0;

	hr = m_pDSBuffer->GetStatus(&dwStatus);

	if (SUCCEEDED(hr) && (dwStatus & DSBSTATUS_BUFFERLOST)) {
		hr = m_pDSBuffer->Restore();
	}

	if ((SUCCEEDED(hr)) && ((dwStatus & DSBSTATUS_PLAYING) != DSBSTATUS_PLAYING)) {
		hr = m_pDSBuffer->Play( 0, 0, DSBPLAY_LOOPING);
		ATLASSERT(SUCCEEDED(hr));
	}

	if (SUCCEEDED(hr)) {
		hr = m_pDSBuffer->GetCurrentPosition(&dwPlayCursor, &dwWriteCursor);
	}

	if (SUCCEEDED(hr)) {
		if ( (  (dwPlayCursor < dwWriteCursor)		&&
				(
					((m_dwDSWriteOff >= dwPlayCursor) && (m_dwDSWriteOff <=  dwWriteCursor))
					||
					((m_dwDSWriteOff < dwPlayCursor) && (m_dwDSWriteOff + lSize >= dwPlayCursor))
				)
			 )
				||
				(  (dwWriteCursor < dwPlayCursor) &&
				   (
					   (m_dwDSWriteOff >= dwPlayCursor) || (m_dwDSWriteOff <  dwWriteCursor)
				   ) ) ) {
			m_dwDSWriteOff = dwWriteCursor;
		}

		if (m_dwDSWriteOff >= (DWORD)m_nDSBufSize) {
			m_dwDSWriteOff = 0;
		}
	}

	if (SUCCEEDED(hr)) {
		hr = WriteSampleToDSBuffer(pMediaSample, NULL);
	}

	return hr;
}

HRESULT CMpcAudioRenderer::WriteSampleToDSBuffer(IMediaSample *pMediaSample, bool *looped)
{
	if (! m_pDSBuffer) {
		return E_POINTER;
	}

	REFERENCE_TIME	rtStart				= 0;
	REFERENCE_TIME	rtStop				= 0;
	HRESULT			hr					= S_OK;
	bool			loop				= false;
	VOID*			pDSLockedBuffers[2]	= { NULL, NULL };
	DWORD			dwDSLockedSize[2]	= {    0,    0 };
	BYTE*			pMediaBuffer		= NULL;
	long			lSize				= pMediaSample->GetActualDataLength();

	hr = pMediaSample->GetPointer(&pMediaBuffer);

	if (m_dRate > 1.0) {
		int nBytePerSample = m_pWaveFileFormat->nChannels * (m_pWaveFileFormat->wBitsPerSample/8);
		m_pSoundTouch->putSamples((const short*)pMediaBuffer, lSize / nBytePerSample);
		lSize = m_pSoundTouch->receiveSamples ((short*)pMediaBuffer, lSize / nBytePerSample) * nBytePerSample;
	}

	pMediaSample->GetTime (&rtStart, &rtStop);

	if (rtStart < 0) {
		DWORD dwPercent	= (DWORD) ((-rtStart * 100) / (rtStop - rtStart));
		DWORD dwRemove	= (lSize*dwPercent/100);

		dwRemove		 = (dwRemove / m_pWaveFileFormat->nBlockAlign) * m_pWaveFileFormat->nBlockAlign;
		pMediaBuffer	+= dwRemove;
		lSize			-= dwRemove;
	}

	if (SUCCEEDED(hr)) {
		hr = m_pDSBuffer->Lock (m_dwDSWriteOff, lSize, &pDSLockedBuffers[0], &dwDSLockedSize[0], &pDSLockedBuffers[1], &dwDSLockedSize[1], 0 );
	}
	if (SUCCEEDED(hr)) {
		if (pDSLockedBuffers [0] != NULL) {
			memcpy(pDSLockedBuffers[0], pMediaBuffer, dwDSLockedSize[0]);
			m_dwDSWriteOff += dwDSLockedSize[0];
		}

		if (pDSLockedBuffers [1] != NULL) {
			memcpy(pDSLockedBuffers[1], &pMediaBuffer[dwDSLockedSize[0]], dwDSLockedSize[1]);
			m_dwDSWriteOff = dwDSLockedSize[1];
			loop = true;
		}

		hr = m_pDSBuffer->Unlock(pDSLockedBuffers[0], dwDSLockedSize[0], pDSLockedBuffers[1], dwDSLockedSize[1]);
		ATLASSERT (dwDSLockedSize [0] + dwDSLockedSize [1] == (DWORD)lSize);
	}

	if (SUCCEEDED(hr) && looped) {
		*looped = loop;
	}

	return hr;
}

#pragma endregion

#pragma region
// ==== WASAPI

HRESULT	CMpcAudioRenderer::DoRenderSampleWasapi(IMediaSample *pMediaSample)
{
	CAutoLock cAutoLock(&m_csRender);

	if (!isAudioClientStarted) {
		TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - Starting audio client\n"));
		HRESULT hr = S_OK;
		if (FAILED(hr = pAudioClient->Start())) {
			TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - Starting audio client failed\n"));
			return hr;
		}
		isAudioClientStarted = true;
		StartRendererThread();
	}

	HRESULT	hr					= S_OK;
	BYTE *pMediaBuffer			= NULL;
	BYTE *pInputBufferPointer	= NULL;
	BYTE *pInputBufferEnd		= NULL;

	m_nBufferSize				= pMediaSample->GetActualDataLength();
	const long lSize			= m_nBufferSize;

	AVSampleFormat in_avsf;
	DWORD in_layout, out_layout;
	int in_channels, out_channels;
	int in_samplerate, out_samplerate;
	int in_samples;
	bool isInt24 = false;
	float* buff  = NULL;

	AM_MEDIA_TYPE *pmt;
	if (SUCCEEDED(pMediaSample->GetMediaType(&pmt)) && pmt!=NULL) {
		CMediaType mt(*pmt);
		if ((WAVEFORMATEXTENSIBLE*)mt.Format() != NULL) {
			hr = CheckAudioClient(&(((WAVEFORMATEXTENSIBLE*)mt.Format())->Format));
		} else {
			hr = CheckAudioClient((WAVEFORMATEX*)mt.Format());
		}
		if (FAILED(hr)) {
			TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - Error while checking audio client with input media type\n"));
			return hr;
		}
		DeleteMediaType(pmt);
	}

	if (m_pWaveFileFormat == NULL || m_pWaveFileFormatOutput == NULL) {
		return E_FAIL;
	}

	bool bFormatChanged = !IsBitstream(m_pWaveFileFormat) && IsFormatChanged(m_pWaveFileFormat, m_pWaveFileFormatOutput);

	if (bFormatChanged) {
		// prepare for resample
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pWaveFileFormat;
		bool isfloat = false;
		if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->cbSize == (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) {
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)m_pWaveFileFormat;
			in_layout = wfex->dwChannelMask;
			if (wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT) {
				isfloat = true;
			}
		} else {
			in_layout = GetDefChannelMask(wfe->nChannels);
			if (wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
				isfloat = true;
			}
		}
		if (isfloat) {
			switch (wfe->wBitsPerSample) {
				case 32:
					in_avsf = AV_SAMPLE_FMT_FLT;
					break;
				case 64:
					in_avsf = AV_SAMPLE_FMT_DBL;
					break;
				default:
					return E_INVALIDARG;
			}
		} else {
			switch (wfe->wBitsPerSample) {
				case 8:
					in_avsf = AV_SAMPLE_FMT_U8;
					break;
				case 16:
					in_avsf = AV_SAMPLE_FMT_S16;
					break;
				case 24:
					isInt24 = true;
				case 32:
					in_avsf = AV_SAMPLE_FMT_S32;
					break;
				default:
					return E_INVALIDARG;
			}
		}
		in_channels    = wfe->nChannels;
		in_samplerate  = wfe->nSamplesPerSec;
		in_samples     = lSize / wfe->nBlockAlign;

		WAVEFORMATEX* wfeOutput = (WAVEFORMATEX*)m_pWaveFileFormatOutput;
		if (wfeOutput->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfeOutput->cbSize == (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) {
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)m_pWaveFileFormatOutput;
			out_layout = wfex->dwChannelMask;
		} else {
			out_layout = GetDefChannelMask(wfeOutput->nChannels);
		}

		out_channels   = wfeOutput->nChannels;
		out_samplerate = wfeOutput->nSamplesPerSec;
	}

	hr = pMediaSample->GetPointer(&pMediaBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	if (bFormatChanged) {
		// Use mixer ...
		BYTE* in_buff = NULL;
		if (isInt24) {
			in_buff = DNew BYTE[lSize / 3 * 4];
			convert_int24_to_int32(in_samples * in_channels, &pMediaBuffer[0], (int32_t*)in_buff);
		} else {
			in_buff = &pMediaBuffer[0];
		}

		m_Resampler.Update(in_avsf, in_layout, out_layout, 0.0f, in_samplerate, out_samplerate);
		int out_samples	= m_Resampler.CalcOutSamples(in_samples);
		buff			= DNew float[out_samples * out_channels];
		if (!buff) {
			if (isInt24) {
				SAFE_DELETE_ARRAY(in_buff);
			}
			return E_OUTOFMEMORY;
		}
		out_samples = m_Resampler.Mixing(buff, out_samples, in_buff, in_samples);

		if (isInt24) {
			SAFE_DELETE_ARRAY(in_buff);
		}

		pInputBufferPointer	= (BYTE*)buff;
		pInputBufferEnd		= (BYTE*)buff + out_samples * out_channels * sizeof(float);
	} else {
		pInputBufferPointer	= &pMediaBuffer[0];
		pInputBufferEnd		= &pMediaBuffer[0] + lSize;
	}

	size_t bufflen = m_WasapiBuf.GetCount();
	m_WasapiBuf.SetCount(bufflen + (pInputBufferEnd - pInputBufferPointer));
	memcpy(m_WasapiBuf.GetData() + bufflen, pInputBufferPointer, (pInputBufferEnd - pInputBufferPointer));

	SAFE_DELETE_ARRAY(buff);

	return S_OK;
}
#pragma endregion

HRESULT CMpcAudioRenderer::CheckAudioClient(WAVEFORMATEX *pWaveFormatEx)
{
	HRESULT hr = S_OK;
	CAutoLock cAutoLock(&m_csCheck);
	TRACE(_T("CMpcAudioRenderer::CheckAudioClient()\n"));
	if (pMMDevice == NULL) {
		hr = GetAudioDevice(&pMMDevice);
	}

	// If no WAVEFORMATEX structure provided and client already exists, return it
	if (pAudioClient != NULL && pWaveFormatEx == NULL) {
		return hr;
	}

	// Just create the audio client if no WAVEFORMATEX provided
	if (pAudioClient == NULL && pWaveFormatEx == NULL) {
		if (SUCCEEDED(hr)) {
			hr = CreateAudioClient(pMMDevice, &pAudioClient);
		}
		return hr;
	}

	// Compare the exisiting WAVEFORMATEX with the one provided
	WAVEFORMATEX *pNewWaveFormatEx = NULL;
	if (CheckFormatChanged(pWaveFormatEx, &pNewWaveFormatEx)) {
		// Format has changed, audio client has to be reinitialized
		TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - Format changed, reinitialize the audio client\n"));

		m_pWaveFileFormat = pNewWaveFormatEx;

		CopyWaveFormat(pNewWaveFormatEx, &m_pWaveFileFormatOutput);
		
		WAVEFORMATEX *pFormat		= NULL;
		WAVEFORMATEX* pDeviceFormat	= NULL;

		WAVEFORMATEX *sharedClosestMatch = NULL;

		IPropertyStore *pProps = NULL;
		PROPVARIANT varConfig;

		if (m_useWASAPIAfterRestart == 1 || IsBitstream(pWaveFormatEx)) { // EXCLUSIVE
			/*
			if (IsBitstream(pWaveFormatEx)) {
				pFormat = pWaveFormatEx;
			} else {
				pMMDevice->OpenPropertyStore(STGM_READ, &pProps);
				PropVariantInit(&varConfig);
				hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varConfig);
				if (SUCCEEDED(hr) && varConfig.vt == VT_BLOB && varConfig.blob.pBlobData != NULL) {
					pFormat = (WAVEFORMATEX*)varConfig.blob.pBlobData;
				}
			}
			*/
			pFormat = pWaveFormatEx;

			hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pFormat, NULL);
			if (S_OK == hr) {
				CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
			}
		} else if (m_useWASAPIAfterRestart == 2) { // SHARED
			pFormat = pWaveFormatEx;
			hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
			if (FAILED(hr)) {
				hr = pAudioClient->GetMixFormat(&pDeviceFormat);
				if (FAILED(hr)) {
					TRACE(_T("CMpcAudioRenderer::CheckMediaType() - GetMixFormat() failed\n"));
					return hr;
				}

				pFormat = pDeviceFormat;
				hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
				}
			}
		}

		TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - IsFormatSupported()\n"));
		TRACE(_T("	Input format:\n"));
		TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	pWaveFormatEx->wFormatTag);
		TRACE(_T("		=> nChannels		= %d\n"),		pWaveFormatEx->nChannels);
		TRACE(_T("		=> nSamplesPerSec	= %d\n"),		pWaveFormatEx->nSamplesPerSec);
		TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		pWaveFormatEx->nAvgBytesPerSec);
		TRACE(_T("		=> nBlockAlign		= %d\n"),		pWaveFormatEx->nBlockAlign);
		TRACE(_T("		=> wBitsPerSample	= %d\n"),		pWaveFormatEx->wBitsPerSample);
		TRACE(_T("		=> cbSize			= %d\n"),		pWaveFormatEx->cbSize);
		TRACE(_T("	Output format:\n"));
		TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	pFormat->wFormatTag);
		TRACE(_T("		=> nChannels		= %d\n"),		pFormat->nChannels);
		TRACE(_T("		=> nSamplesPerSec	= %d\n"),		pFormat->nSamplesPerSec);
		TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		pFormat->nAvgBytesPerSec);
		TRACE(_T("		=> nBlockAlign		= %d\n"),		pFormat->nBlockAlign);
		TRACE(_T("		=> wBitsPerSample	= %d\n"),		pFormat->wBitsPerSample);
		TRACE(_T("		=> cbSize			= %d\n"),		pFormat->cbSize);

		if (pProps) {
			PropVariantClear(&varConfig);
			SAFE_RELEASE(pProps);
		}

		if (pDeviceFormat) {
			CoTaskMemFree(pDeviceFormat);
		}

		if (S_OK == hr) {
			TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the format\n"));
			StopAudioClient();
			SAFE_RELEASE(pRenderClient);
			SAFE_RELEASE(pAudioClient);
			if (SUCCEEDED(hr)) {
				hr = CreateAudioClient(pMMDevice, &pAudioClient);
			}
		} else if (S_FALSE == hr) {
			TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client refused the format with a closest match\n"));
			TRACE(_T("	=> ppClosestMatch:\n"));
			TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	sharedClosestMatch->wFormatTag);
			TRACE(_T("		=> nChannels		= %d\n"),		sharedClosestMatch->nChannels);
			TRACE(_T("		=> nSamplesPerSec	= %d\n"),		sharedClosestMatch->nSamplesPerSec);
			TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		sharedClosestMatch->nAvgBytesPerSec);
			TRACE(_T("		=> nBlockAlign		= %d\n"),		sharedClosestMatch->nBlockAlign);
			TRACE(_T("		=> wBitsPerSample	= %d\n"),		sharedClosestMatch->wBitsPerSample);
			TRACE(_T("		=> cbSize			= %d\n"),		sharedClosestMatch->cbSize);

			CopyWaveFormat(sharedClosestMatch, &m_pWaveFileFormatOutput);
			CoTaskMemFree(sharedClosestMatch);

			hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, m_pWaveFileFormatOutput, &sharedClosestMatch);
			if (S_OK == hr) {
				TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the closest match format\n"));
				StopAudioClient();
				SAFE_RELEASE(pRenderClient);
				SAFE_RELEASE(pAudioClient);
				if (SUCCEEDED(hr)) {
					hr = CreateAudioClient(pMMDevice, &pAudioClient);
				}
			}

			if (hr != S_OK) {
				if (sharedClosestMatch) {
					CoTaskMemFree(sharedClosestMatch);
				}
				SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);

				return hr;
			}
		} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
			TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client refused the format\n"));
			SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);
			return hr;
		} else {
			TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI failed = 0x%08x\n"), hr);
			SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);
			return hr;
		}
	} else if (pRenderClient == NULL) {
		TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - First initialization of the audio renderer\n"));
	} else {
		return hr;
	}

	SAFE_RELEASE(pRenderClient);

	if (SUCCEEDED(hr)) {
		hr = InitAudioClient(m_pWaveFileFormatOutput, pAudioClient, &pRenderClient);
	}
	return hr;
}

HRESULT CMpcAudioRenderer::GetAvailableAudioDevices(IMMDeviceCollection **ppMMDevices)
{
	HRESULT hr;

	CComPtr<IMMDeviceEnumerator> enumerator;
	TRACE(_T("CMpcAudioRenderer::GetAvailableAudioDevices()\n"));
	hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));

	if (FAILED(hr)) {
		TRACE(_T("CMpcAudioRenderer::GetAvailableAudioDevices() - failed to get MMDeviceEnumerator\n"));
		return S_FALSE;
	}

	//IMMDevice* pEndpoint = NULL;
	//IPropertyStore* pProps = NULL;
	//LPWSTR pwszID = NULL;

	enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, ppMMDevices);
	UINT count(0);
	hr = (*ppMMDevices)->GetCount(&count);
	return hr;
}

HRESULT CMpcAudioRenderer::GetAudioDevice(IMMDevice **ppMMDevice)
{
	TRACE(_T("CMpcAudioRenderer::GetAudioDevice()\n"));

	CComPtr<IMMDeviceEnumerator> enumerator;
	IMMDeviceCollection* devices;
	IPropertyStore* pProps = NULL;
	HRESULT hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));

	if (hr != S_OK) {
		TRACE(_T("CMpcAudioRenderer::GetAudioDevice() - failed to create MMDeviceEnumerator!\n"));
		return hr;
	}

	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - Target end point: '%s'", m_csSound_Device));

	if (GetAvailableAudioDevices(&devices) == S_OK && devices) {
		UINT count(0);
		hr = devices->GetCount(&count);
		if (hr != S_OK) {
			TRACE(_T("CMpcAudioRenderer::GetAudioDevice() - devices->GetCount failed: (0x%08x)\n"), hr);
			return hr;
		}

		for (ULONG i = 0 ; i < count ; i++) {
			LPWSTR pwszID = NULL;
			IMMDevice *endpoint = NULL;
			hr = devices->Item(i, &endpoint);
			if (hr == S_OK) {
				hr = endpoint->GetId(&pwszID);
				if (hr == S_OK) {
					if (endpoint->OpenPropertyStore(STGM_READ, &pProps) == S_OK) {

						PROPVARIANT varName;
						PropVariantInit(&varName);

						// Found the configured audio endpoint
						if ((pProps->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK) && (m_csSound_Device == varName.pwszVal)) {
							DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - devices->GetId OK, num: (%d), pwszVal: '%s', pwszID: '%s'", i, varName.pwszVal, pwszID));
							enumerator->GetDevice(pwszID, ppMMDevice);
							SAFE_RELEASE(devices);
							*(ppMMDevice) = endpoint;
							CoTaskMemFree(pwszID);
							pwszID = NULL;
							PropVariantClear(&varName);
							SAFE_RELEASE(pProps);
							return S_OK;
						} else {
							PropVariantClear(&varName);
							SAFE_RELEASE(pProps);
							SAFE_RELEASE(endpoint);
							CoTaskMemFree(pwszID);
							pwszID = NULL;
						}
					}
				} else {
					TRACE(_T("CMpcAudioRenderer::GetAudioDevice() - devices->GetId failed: (0x%08x)\n"), hr);
				}
			} else {
				TRACE(_T("CMpcAudioRenderer::GetAudioDevice() - devices->Item failed: (0x%08x)\n"), hr);
			}

			CoTaskMemFree(pwszID);
			pwszID = NULL;
		}
	}

	TRACE(_T("CMpcAudioRenderer::GetAudioDevice() - Unable to find selected audio device, using the default end point!\n"));
	hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, ppMMDevice);

	SAFE_RELEASE(devices);

	return hr;
}

bool CMpcAudioRenderer::IsFormatChanged(const WAVEFORMATEX *pWaveFormatEx, const WAVEFORMATEX *pNewWaveFormatEx)
{
	if (!pWaveFormatEx || !pNewWaveFormatEx) {
		return true;
	}

	if (pWaveFormatEx->wFormatTag != pNewWaveFormatEx->wFormatTag
			|| pWaveFormatEx->nChannels != pNewWaveFormatEx->nChannels
			|| pWaveFormatEx->wBitsPerSample != pNewWaveFormatEx->wBitsPerSample
			|| pWaveFormatEx->nSamplesPerSec != pNewWaveFormatEx->nSamplesPerSec) {
		return true;
	}

	WAVEFORMATEXTENSIBLE* wfex		= NULL;
	WAVEFORMATEXTENSIBLE* wfexNew	= NULL;
	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pWaveFormatEx->cbSize == (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) {
		wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
	}
	if ((pNewWaveFormatEx)->wFormatTag == WAVE_FORMAT_EXTENSIBLE && (pNewWaveFormatEx)->cbSize == (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) {
		wfexNew	= (WAVEFORMATEXTENSIBLE*)pNewWaveFormatEx;
	}

	if (wfex && wfexNew &&
			(wfex->SubFormat != wfexNew->SubFormat
			|| wfex->dwChannelMask != wfexNew->dwChannelMask
			|| wfex->Samples.wSamplesPerBlock != wfexNew->Samples.wSamplesPerBlock
			|| wfex->Samples.wValidBitsPerSample != wfexNew->Samples.wValidBitsPerSample)) {
		return true;
	}

	return false;
}

bool CMpcAudioRenderer::CheckFormatChanged(WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx)
{
	bool formatChanged = false;
	if (m_pWaveFileFormat == NULL) {
		formatChanged = true;
	} else {
		formatChanged = IsFormatChanged(pWaveFormatEx, m_pWaveFileFormat);
	}

	if (!formatChanged) {
		return false;
	}

	return CopyWaveFormat(pWaveFormatEx, ppNewWaveFormatEx);
}

bool CMpcAudioRenderer::CopyWaveFormat(WAVEFORMATEX *pSrcWaveFormatEx, WAVEFORMATEX **ppDestWaveFormatEx)
{
	SAFE_DELETE_ARRAY(*ppDestWaveFormatEx);

	size_t size = sizeof(WAVEFORMATEX) + pSrcWaveFormatEx->cbSize; // Always true, even for WAVEFORMATEXTENSIBLE and WAVEFORMATEXTENSIBLE_IEC61937
	*ppDestWaveFormatEx = (WAVEFORMATEX *)DNew BYTE[size];
	if (!(*ppDestWaveFormatEx)) {
		return false;
	}
	memcpy(*ppDestWaveFormatEx, pSrcWaveFormatEx, size);

	return true;
}

bool CMpcAudioRenderer::IsBitstream(WAVEFORMATEX *pWaveFormatEx)
{
	if (!pWaveFormatEx) {
		return false;
	}

	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		return true;
	}

	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pWaveFormatEx->cbSize == (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) {
		WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
		return (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS
				|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD
				|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP);
	}

	return false;
}

HRESULT CMpcAudioRenderer::StopAudioClient()
{
	HRESULT hr = S_OK;

	PauseRendererThread();

	if (pAudioClient && isAudioClientStarted) {
		hr = pAudioClient->Stop();
		hr = pAudioClient->Reset();

		m_Resampler.FlushBuffers();
		hnsActualDuration = 0;
	}
	isAudioClientStarted = false;

	return hr;
}

HRESULT CMpcAudioRenderer::GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod)
{
	if (pWaveFormatEx == NULL) {
		return S_OK;
	}

	if (!m_nBufferSize) {
		if (pWaveFormatEx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
			if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
				m_nBufferSize = 6144;
			} else {
				return S_OK;
			}
		} else {
			WAVEFORMATEXTENSIBLE *wfext = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;

			if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
				m_nBufferSize = 61440;
			} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
				m_nBufferSize = 32768;
			} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
				m_nBufferSize = 24576;
			} else {
				return S_OK;
			}
		}
	}

	*pHnsBufferPeriod = (REFERENCE_TIME)((REFERENCE_TIME)m_nBufferSize * 10000 * 8 / ((REFERENCE_TIME)pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample *
						1.0 * pWaveFormatEx->nSamplesPerSec) /*+ 0.5*/);
	*pHnsBufferPeriod *= 1000;

	TRACE(_T("CMpcAudioRenderer::GetBufferSize() - set a %lld period for a %ld buffer size\n"), *pHnsBufferPeriod, m_nBufferSize);

	return S_OK;
}

HRESULT CMpcAudioRenderer::InitAudioClient(WAVEFORMATEX *pWaveFormatEx, IAudioClient *pAudioClient, IAudioRenderClient **ppRenderClient)
{
	TRACE(_T("CMpcAudioRenderer::InitAudioClient()\n"));
	HRESULT hr = S_OK;
	// Initialize the stream to play at the minimum latency.
	hr = pAudioClient->GetDevicePeriod(NULL, &hnsPeriod);
	if (FAILED(hr)) {
		hnsPeriod = 500000; //50 ms is the best according to James @Slysoft
	}

	if (m_useWASAPIAfterRestart == 1 || IsBitstream(pWaveFormatEx)) { // EXCLUSIVE
		hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pWaveFormatEx, NULL);
	} else if (m_useWASAPIAfterRestart == 2) { // SHARED
		WAVEFORMATEX *sharedClosestMatch = NULL;
		hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pWaveFormatEx, &sharedClosestMatch);
		if (sharedClosestMatch) {
			CoTaskMemFree(sharedClosestMatch);
		}
	}

	if (S_OK == hr) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - WASAPI client accepted the format\n"));;
	} else if (S_FALSE == hr) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - WASAPI client refused the format with a closest match\n"));
		return hr;
	} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - WASAPI client refused the format\n"));
		return hr;
	} else {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - WASAPI failed = 0x%08x\n"), hr);
		return hr;
	}

	GetBufferSize(pWaveFormatEx, &hnsPeriod);

	if (SUCCEEDED(hr)) {
		hr = pAudioClient->Initialize((m_useWASAPIAfterRestart == 1 || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
									  AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
									  hnsPeriod, hnsPeriod, pWaveFormatEx, NULL);
	}

	if (FAILED(hr) && hr != AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - FAILED(0x%08x)\n"), hr);
		return hr;
	}

	if (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED == hr) {
		// if the buffer size was not aligned, need to do the alignment dance
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - Buffer size not aligned. Realigning\n"));

		// get the buffer size, which will be aligned
		hr = pAudioClient->GetBufferSize(&nFramesInBuffer);

		// throw away this IAudioClient
		StopAudioClient();
		SAFE_RELEASE(pRenderClient);
		SAFE_RELEASE(pAudioClient);
		if (SUCCEEDED(hr)) {
			hr = CreateAudioClient(pMMDevice, &pAudioClient);
		}

		// calculate the new aligned periodicity
		hnsPeriod = // hns =
			(REFERENCE_TIME)(
				10000.0 * // (hns / ms) *
				1000 * // (ms / s) *
				nFramesInBuffer / // frames /
				pWaveFormatEx->nSamplesPerSec  // (frames / s)
				+ 0.5 // rounding
			);

		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - Trying again with periodicity of %I64u hundred-nanoseconds, or %u frames.\n"), hnsPeriod, nFramesInBuffer);
		if (SUCCEEDED(hr)) {
			hr = pAudioClient->Initialize((m_useWASAPIAfterRestart == 1 || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
										  AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
										  hnsPeriod, hnsPeriod, pWaveFormatEx, NULL);
		}

		if (FAILED(hr)) {
			TRACE(_T("CMpcAudioRenderer::InitAudioClient() - Failed to reinitialize the audio client\n"));
			return hr;
		}
	}

	// get the buffer size, which is aligned
	if (SUCCEEDED(hr)) {
		hr = pAudioClient->GetBufferSize(&nFramesInBuffer);
	}

	// enables a client to write output data to a rendering endpoint buffer
	if (SUCCEEDED(hr)) {
		hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)(ppRenderClient));
	}

	if (FAILED(hr)) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - service initialization FAILED(0x%08x)\n"), hr);
	} else {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - service initialization success\n"));
	}

	if (SUCCEEDED(hr)) {
		hr = pAudioClient->SetEventHandle(m_hDataEvent);
	}

	return hr;
}

HRESULT CMpcAudioRenderer::CreateAudioClient(IMMDevice *pMMDevice, IAudioClient **ppAudioClient)
{
	HRESULT hr	= S_OK;
	hnsPeriod	= 0;

	TRACE(_T("CMpcAudioRenderer::CreateAudioClient()\n"));

	if (*ppAudioClient) {
		if (isAudioClientStarted) {
			(*ppAudioClient)->Stop();
		}
		SAFE_RELEASE(*ppAudioClient);
		isAudioClientStarted = false;
	}

	if (pMMDevice == NULL) {
		TRACE(_T("CMpcAudioRenderer::CreateAudioClient() - failed, device not loaded\n"));
		return E_FAIL;
	}

	hr = pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(ppAudioClient));
	if (FAILED(hr)) {
		TRACE(_T("CMpcAudioRenderer::CreateAudioClient() - activation FAILED(0x%08x)\n"), hr);
	} else {
		TRACE(_T("CMpcAudioRenderer::CreateAudioClient() - success\n"));
	}
	return hr;
}