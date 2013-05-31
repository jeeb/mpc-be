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
#include "../../../DSUtil/AudioTools.h"
#include <ks.h>
#include <ksmedia.h>
#include "AudioHelper.h"
#include "MpcAudioRenderer.h"

// options names
#define OPT_REGKEY_AudRend  _T("Software\\MPC-BE Filters\\MPC Audio Renderer")
#define OPT_SECTION_AudRend _T("Filters\\MPC Audio Renderer")
#define OPT_DeviceMode      _T("UseWasapi")
#define OPT_MuteFastForward _T("MuteFastForward")
#define OPT_AudioDevice     _T("SoundDevice")
// TODO: rename option values

// set to 1 to enable more detail debug log
#define DBGLOG_LEVEL 0

#ifdef REGISTER_FILTER

#include "../../filters/ffmpeg_fix.cpp"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpcAudioRenderer), MpcAudioRendererName, /*0x40000001*/MERIT_UNLIKELY + 1, _countof(sudpPins), sudpPins, CLSID_AudioRendererCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, &__uuidof(CMpcAudioRenderer), CreateInstance<CMpcAudioRenderer>, NULL, &sudFilter[0]},
	{L"CMpcAudioRendererPropertyPage", &__uuidof(CMpcAudioRendererSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd> >},
	{L"CMpcAudioRendererPropertyPageStatus", &__uuidof(CMpcAudioRendererStatusWnd), CreateInstance<CInternalPropertyPageTempl<CMpcAudioRendererStatusWnd> >},
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

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

static GUID lpSoundGUID = {0xdef00000, 0x9c6d, 0x47ed, {0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03}}; //DSDEVID_DefaultPlayback from dsound.h

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

static void DumpWaveFormatEx(WAVEFORMATEX* pwfx)
{
	TRACE(_T("		=> wFormatTag		= 0x%04x\n"),	pwfx->wFormatTag);
	TRACE(_T("		=> nChannels		= %d\n"),		pwfx->nChannels);
	TRACE(_T("		=> nSamplesPerSec	= %d\n"),		pwfx->nSamplesPerSec);
	TRACE(_T("		=> nAvgBytesPerSec	= %d\n"),		pwfx->nAvgBytesPerSec);
	TRACE(_T("		=> nBlockAlign		= %d\n"),		pwfx->nBlockAlign);
	TRACE(_T("		=> wBitsPerSample	= %d\n"),		pwfx->wBitsPerSample);
	TRACE(_T("		=> cbSize			= %d\n"),		pwfx->cbSize);
	if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pwfx->cbSize == sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
		WAVEFORMATEXTENSIBLE* wfe = (WAVEFORMATEXTENSIBLE*)pwfx;
		TRACE(_T("		WAVEFORMATEXTENSIBLE:\n"));
		TRACE(_T("			=> wValidBitsPerSample	= %d\n"), wfe->Samples.wValidBitsPerSample);
		TRACE(_T("			=> dwChannelMask		= 0x%x\n"), wfe->dwChannelMask);
		TRACE(_T("			=> SubFormat			= %s\n"), CStringFromGUID(wfe->SubFormat));
	}
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
	, m_pAudioClient	(NULL)
	, m_pRenderClient	(NULL)
	, m_useWASAPI		(2)
	, m_bMuteFastForward(false)
	, nFramesInBuffer	(0)
	, hnsPeriod			(0)
	, m_hTask			(NULL)
	, isAudioClientStarted (false)
	, lastBufferTime	(0)
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
	, m_bIsBitstream	(false)
	, m_hModule			(NULL)
	, pfAvSetMmThreadCharacteristicsW	(NULL)
	, pfAvRevertMmThreadCharacteristics	(NULL)
{
	TRACE(_T("CMpcAudioRenderer::CMpcAudioRenderer()\n"));

#ifdef REGISTER_FILTER
	CRegKey key;
	TCHAR buff[256];
	ULONG len;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_AudRend, KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_DeviceMode, dw)) {
			m_useWASAPI = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_MuteFastForward, dw)) {
			m_bMuteFastForward = !!dw;
		}
		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_AudioDevice, buff, &len)) {
			m_csSound_Device = CString(buff);
		}
	}
#else
	m_useWASAPI			= AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_DeviceMode, m_useWASAPI);
	m_bMuteFastForward	= !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_MuteFastForward, m_bMuteFastForward);
	m_csSound_Device	= AfxGetApp()->GetProfileString(OPT_SECTION_AudRend, OPT_AudioDevice, m_csSound_Device);
#endif

	m_useWASAPI				= min(max(m_useWASAPI, 0), 2);
	m_useWASAPIAfterRestart	= m_useWASAPI;

	// Load Vista & above specifics DLLs
	m_hModule = LoadLibrary(L"AVRT.dll");
	if (m_hModule != NULL) {
		pfAvSetMmThreadCharacteristicsW		= (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress(m_hModule, "AvSetMmThreadCharacteristicsW");
		pfAvRevertMmThreadCharacteristics	= (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress(m_hModule, "AvRevertMmThreadCharacteristics");
	} else {
		m_useWASAPI = 0; // Wasapi not available below Vista
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
	StopAudioClient(&m_pAudioClient);
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

	SAFE_RELEASE (m_pRenderClient);
	SAFE_RELEASE (m_pAudioClient);
	SAFE_RELEASE (pMMDevice);

	if (m_pReferenceClock) {
		SetSyncSource(NULL);
		SAFE_RELEASE (m_pReferenceClock);
	}

	SAFE_DELETE_ARRAY(m_pWaveFileFormat);
	SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);

	if (m_hModule) {
		FreeLibrary(m_hModule);
	}
}

HRESULT CMpcAudioRenderer::CheckInputType(const CMediaType *pmt)
{
	return CheckMediaType(pmt);
}

HRESULT	CMpcAudioRenderer::CheckMediaType(const CMediaType *pmt)
{
	CAutoLock cAutoLock(&m_csCheck);

	if (pmt == NULL) {
		return E_INVALIDARG;
	}

	if ((pmt->majortype != MEDIATYPE_Audio) || (pmt->formattype != FORMAT_WaveFormatEx)) {
		TRACE(_T("CMpcAudioRenderer::CheckMediaType() - Not supported\n"));
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	HRESULT hr = S_OK;

	TRACE(_T("CMpcAudioRenderer::CheckMediaType()\n"));
	if (pmt->subtype != MEDIASUBTYPE_PCM && pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT) {
		TRACE(_T("CMpcAudioRenderer::CheckMediaType() - allow only PCM/IEEE_FLOAT input\n"));
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pmt->Format();

	if (pwfx == NULL) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (m_useWASAPI) {
		hr = CheckAudioClient((WAVEFORMATEX *)NULL);
		if (FAILED(hr)) {
			TRACE(_T("CMpcAudioRenderer::CheckMediaType() - Error on check audio client\n"));
			return hr;
		}
		if (!m_pAudioClient) {
			TRACE(_T("CMpcAudioRenderer::CheckMediaType() - Error, audio client not loaded\n"));
			return VFW_E_CANNOT_CONNECT;
		}

		CopyWaveFormat(pwfx, &m_pWaveFileFormatOutput);

		WAVEFORMATEX *pFormat		= NULL;
		WAVEFORMATEX* pDeviceFormat	= NULL;

		WAVEFORMATEX *sharedClosestMatch	= NULL;

		IPropertyStore *pProps = NULL;
		PROPVARIANT varConfig;

		if (m_useWASAPI == 1 || IsBitstream(pwfx)) { // EXCLUSIVE
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

			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pFormat, NULL);
			if (S_OK == hr) {
				CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
			}
		} else if (m_useWASAPI == 2) { // SHARED
			pFormat = pwfx;
			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
			if (FAILED(hr)) {
				hr = m_pAudioClient->GetMixFormat(&pDeviceFormat);
				if (FAILED(hr)) {
					TRACE(_T("CMpcAudioRenderer::CheckMediaType() - GetMixFormat() failed\n"));
					return hr;
				}

				pFormat = pDeviceFormat;
				hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
				}
			}
		}
		TRACE(_T("CMpcAudioRenderer::CheckMediaType() - IsFormatSupported()\n"));
		TRACE(_T("	Input format:\n"));
		DumpWaveFormatEx(pwfx);

		TRACE(_T("	Output format:\n"));
		DumpWaveFormatEx(pFormat);

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
				DumpWaveFormatEx(sharedClosestMatch);

				CopyWaveFormat(sharedClosestMatch, &m_pWaveFileFormatOutput);
				CoTaskMemFree(sharedClosestMatch);

				hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, m_pWaveFileFormatOutput, &sharedClosestMatch);
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
		if (m_pRenderClient != NULL) {
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
		m_hRenderThread = ::CreateThread(NULL, 0, RenderThreadEntryPoint, (LPVOID)this, /*CREATE_SUSPENDED*/0, &m_nThreadId);

		if (m_hRenderThread) {
			SetThreadPriority(m_hRenderThread, /*THREAD_PRIORITY_HIGHEST*/THREAD_PRIORITY_TIME_CRITICAL);
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
	TRACE(L"CMpcAudioRenderer::RenderThread() - start, thread ID = 0x%x\n", m_nThreadId);

	HRESULT hr = S_OK;

	// These are wait handles for the thread stopping, new sample arrival and pausing redering
	HANDLE renderHandles[3] = {
		m_hStopRenderThreadEvent,
		m_hPauseEvent,
		m_hDataEvent
	};

	// These are wait handles for resuming or exiting the thread
	HANDLE resumeHandles[2] = {
		m_hStopRenderThreadEvent,
		m_hResumeEvent
	};

	EnableMMCSS();

	while (true) {
		// 1) Waiting for the next WASAPI buffer to be available to be filled
		// 2) Exit requested for the thread
		// 3) For a pause request
		DWORD result = WaitForMultipleObjects(3, renderHandles, FALSE, INFINITE);
		switch (result) {
			case WAIT_OBJECT_0:
				TRACE(_T("CMpcAudioRenderer::RenderThread() - exit events\n"));
				RevertMMCSS();
				return 0;
			case WAIT_OBJECT_0 + 1: 
				{
					TRACE(_T("CMpcAudioRenderer::RenderThread() - pause events\n"));

					m_bThreadPaused = TRUE;
					ResetEvent(m_hResumeEvent);
					SetEvent(m_hWaitPauseEvent);

					DWORD resultResume = WaitForMultipleObjects(2, resumeHandles, FALSE, INFINITE);
					if (resultResume == WAIT_OBJECT_0) { // exit event
						TRACE(_T("CMpcAudioRenderer::RenderThread() - exit events\n"));
						RevertMMCSS();
						return 0;
					}

					if (resultResume == WAIT_OBJECT_0 + 1) { // resume event
						TRACE(_T("CMpcAudioRenderer::RenderThread() - resume events\n"));
						m_bThreadPaused = FALSE;
						SetEvent(m_hWaitResumeEvent);
					}
				}
				break;
			case WAIT_OBJECT_0 + 2:
				{
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::RenderThread() - Data Event, Audio client state = %s\n"), isAudioClientStarted ? _T("Started") : _T("Stoped"));
#endif
					RenderWasapiBuffer();
				}
				break;
			default:
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
				TRACE(L"CMpcAudioRenderer::RenderThread() - WaitForMultipleObjects() failed: %d\n", GetLastError());
#endif
				break;
		}
	}

	RevertMMCSS();

	TRACE(L"CMpcAudioRenderer::RenderThread() - close, thread ID = 0x%x\n", m_nThreadId);

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
		m_Resampler.FlushBuffers();
		m_WasapiBuf.RemoveAll();

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
		if (m_lVolume <= -10000) {
			m_dVolume = 0.0;
		} else if (m_lVolume >= 0) {
			m_dVolume = 1.0;
		} else {
			m_dVolume = pow(10.0, m_lVolume / 2000.0);
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

	BOOL bShowStatusPage	= m_pInputPin && m_pInputPin->IsConnected();

	pPages->cElems			= bShowStatusPage ? 2 : 1;
	pPages->pElems			= (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	CheckPointer(pPages->pElems, E_OUTOFMEMORY);

	pPages->pElems[0]		= __uuidof(CMpcAudioRendererSettingsWnd);
	if (bShowStatusPage) {
		pPages->pElems[1]	= __uuidof(CMpcAudioRendererStatusWnd);
	}

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
	} else if (guid == __uuidof(CMpcAudioRendererStatusWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpcAudioRendererStatusWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// === IMpcAudioRendererFilter
STDMETHODIMP CMpcAudioRenderer::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_AudRend)) {
		key.SetDWORDValue(OPT_DeviceMode, m_useWASAPIAfterRestart);
		key.SetDWORDValue(OPT_MuteFastForward, m_bMuteFastForward);
		key.SetStringValue(OPT_AudioDevice, m_csSound_Device);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_DeviceMode, m_useWASAPIAfterRestart);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_MuteFastForward, m_bMuteFastForward);
	AfxGetApp()->WriteProfileString(OPT_SECTION_AudRend, OPT_AudioDevice, m_csSound_Device);
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
		if (m_bIsBitstream) {
			return MODE_WASAPI_EXCLUSIVE_BITSTREAM;
		} else if (m_useWASAPI == 1) {
			return MODE_WASAPI_EXCLUSIVE;
		} else if (m_useWASAPI == 2) {
			return MODE_WASAPI_SHARED;
		}
	} else if (m_pDSBuffer) {
		return MODE_DIRECTSOUND;
	}

	return MODE_NONE;
}

STDMETHODIMP CMpcAudioRenderer::GetStatus(WAVEFORMATEX** ppWfxIn, WAVEFORMATEX** ppWfxOut)
{
	CAutoLock cAutoLock(&m_csProps);

	*ppWfxIn	= m_pWaveFileFormat;
	*ppWfxOut	= m_pWaveFileFormatOutput;

	return S_OK;
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
	hr = StopAudioClient(&m_pAudioClient);
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


const TCHAR *GetAVSampleFormatString(AVSampleFormat value)
{
#define UNPACK_VALUE(VALUE) case VALUE: return _T( #VALUE );
	switch (value) {
		UNPACK_VALUE(AV_SAMPLE_FMT_U8);
		UNPACK_VALUE(AV_SAMPLE_FMT_S16);
		UNPACK_VALUE(AV_SAMPLE_FMT_S32);
		UNPACK_VALUE(AV_SAMPLE_FMT_FLT);
		UNPACK_VALUE(AV_SAMPLE_FMT_DBL);
	};
#undef UNPACK_VALUE
	return L"Error value";
};

HRESULT	CMpcAudioRenderer::DoRenderSampleWasapi(IMediaSample *pMediaSample)
{
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
	TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi()\n"));
#endif

	HRESULT	hr					= S_OK;
	BYTE *pMediaBuffer			= NULL;
	BYTE *pInputBufferPointer	= NULL;

	long lSize					= pMediaSample->GetActualDataLength();

	AVSampleFormat in_avsf;
	DWORD in_layout, out_layout;
	int in_channels, out_channels;
	int in_samplerate, out_samplerate;
	int in_samples;
	bool isInt24			= false;
	float* buff				= NULL;

	WORD out_BitsPerSample	= 0;
	BOOL out_IsFloat		= FALSE;
	BYTE* out_buf			= NULL;

	AM_MEDIA_TYPE *pmt;
	if (SUCCEEDED(pMediaSample->GetMediaType(&pmt)) && pmt!=NULL) {
		CMediaType mt(*pmt);
		if ((WAVEFORMATEXTENSIBLE*)mt.Format() != NULL) {
			hr = CheckAudioClient(&(((WAVEFORMATEXTENSIBLE*)mt.Format())->Format));
		} else {
			hr = CheckAudioClient((WAVEFORMATEX*)mt.Format());
		}
		if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
			TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - Error while checking audio client with input media type\n"));
#endif
			return hr;
		}
		DeleteMediaType(pmt);
	}

	if (m_pWaveFileFormat == NULL || m_pWaveFileFormatOutput == NULL) {
		return E_FAIL;
	}

	bool bFormatChanged = !IsBitstream(m_pWaveFileFormat) && IsFormatChanged(m_pWaveFileFormat, m_pWaveFileFormatOutput);

	if (bFormatChanged) {
		// prepare for resample ... if needed
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
			out_IsFloat	= (wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT);
			out_layout	= wfex->dwChannelMask;
		} else {
			out_IsFloat	= (wfeOutput->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
			out_layout	= GetDefChannelMask(wfeOutput->nChannels);
		}

		out_BitsPerSample	= wfeOutput->wBitsPerSample;
		out_channels		= wfeOutput->nChannels;
		out_samplerate		= wfeOutput->nSamplesPerSec;
	}

	hr = pMediaSample->GetPointer(&pMediaBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	if (bFormatChanged) {
		BYTE* in_buff = NULL;
		if (isInt24) {
			in_buff = DNew BYTE[lSize / 3 * 4];
			convert_int24_to_int32(in_samples * in_channels, &pMediaBuffer[0], (int32_t*)in_buff);
		} else {
			in_buff = &pMediaBuffer[0];
		}

		bool bUseMixer		= false;
		int out_samples		= in_samples;
		AVSampleFormat avsf	= in_avsf;

		if (in_layout != out_layout || in_samplerate != out_samplerate) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
			TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - use Mixer\n"));
			TRACE(_T("	input:\n"));
			TRACE(_T("		layout		= 0x%x\n"), in_layout);
			TRACE(_T("		channels	= %d\n"), in_channels);
			TRACE(_T("		samplerate	= %d\n"), in_samplerate);
			TRACE(_T("	output:\n"));
			TRACE(_T("		layout		= 0x%x\n"), out_layout);
			TRACE(_T("		channels	= %d\n"), out_channels);
			TRACE(_T("		samplerate	= %d\n"), out_samplerate);
#endif

			m_Resampler.Update(in_avsf, in_layout, out_layout, 0.0f, in_samplerate, out_samplerate);
			out_samples	= m_Resampler.CalcOutSamples(in_samples);
			buff		= DNew float[out_samples * out_channels];
			if (!buff) {
				if (isInt24) {
					SAFE_DELETE_ARRAY(in_buff);
				}
				return E_OUTOFMEMORY;
			}
			out_samples = m_Resampler.Mixing(buff, out_samples, in_buff, in_samples);

			if (isInt24) {
				isInt24 = false;
				SAFE_DELETE_ARRAY(in_buff);
			}

			bUseMixer	= true;
			avsf		= AV_SAMPLE_FMT_FLT;
		} else {
			buff = (float*)in_buff;
		}

		{
			WAVEFORMATEX* wfeOutput				= (WAVEFORMATEX*)m_pWaveFileFormatOutput;
			WAVEFORMATEXTENSIBLE* wfexOutput	= (WAVEFORMATEXTENSIBLE*)wfeOutput;

			WORD tag	= wfeOutput->wFormatTag;
			bool fPCM	= tag == WAVE_FORMAT_PCM || tag == WAVE_FORMAT_EXTENSIBLE && wfexOutput->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;
			bool fFloat	= tag == WAVE_FORMAT_IEEE_FLOAT || tag == WAVE_FORMAT_EXTENSIBLE && wfexOutput->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

			if (fPCM) {
				if (wfeOutput->wBitsPerSample == 8) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - unsupported convert from '%s' to 8bit PCM\n"), GetAVSampleFormatString(avsf));
#endif
					;// TODO ...
				} else if (wfeOutput->wBitsPerSample == 16) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 16bit PCM\n"), GetAVSampleFormatString(avsf));
#endif
					lSize	= out_samples * out_channels * sizeof(int16_t);
					out_buf	= DNew BYTE[lSize];
					convert_to_int16(avsf, out_channels, out_samples, (BYTE*)buff, (int16_t*)out_buf);
				} else if (wfeOutput->wBitsPerSample == 24) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 24bit PCM\n"), GetAVSampleFormatString(avsf));
#endif
					lSize	= out_samples * out_channels * sizeof(BYTE) * 3;
					out_buf	= DNew BYTE[lSize];
					convert_to_int24(avsf, out_channels, out_samples, (BYTE*)buff, out_buf);
				} else if (wfeOutput->wBitsPerSample == 32) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 32bit PCM\n"), GetAVSampleFormatString(avsf));
#endif
					lSize	= out_samples * out_channels * sizeof(int32_t);
					out_buf	= DNew BYTE[lSize];
					convert_to_int32(avsf, out_channels, out_samples, (BYTE*)buff, (int32_t*)out_buf);
				}
			} else if (fFloat) {
				if (wfeOutput->wBitsPerSample == 32) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 32bit FLOAT\n"), GetAVSampleFormatString(avsf));
#endif					
					lSize	= out_samples * out_channels * sizeof(float);
					out_buf	= DNew BYTE[lSize];
					convert_to_float(avsf, out_channels, out_samples, (BYTE*)buff, (float*)out_buf);
				} else if (wfeOutput->wBitsPerSample == 64) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
					TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi() - unsupported convert from '%s' to 64bit FLOAT\n"), GetAVSampleFormatString(avsf));
#endif
					;// TODO ...
				}
			}

			if (isInt24) {
				SAFE_DELETE_ARRAY(in_buff);
			}

			if (bUseMixer) {
				SAFE_DELETE_ARRAY(buff);
			}

			if (!out_buf) {
				return E_INVALIDARG;
			}

		}

		pInputBufferPointer	= &out_buf[0];
	} else {
		pInputBufferPointer	= &pMediaBuffer[0];
	}

	{
		CAutoLock cRenderLock(&m_csRender);

		size_t bufflen = m_WasapiBuf.GetCount();
		m_WasapiBuf.SetCount(bufflen + lSize);
		memcpy(m_WasapiBuf.GetData() + bufflen, pInputBufferPointer, lSize);
	}

	SAFE_DELETE_ARRAY(out_buf);

	if (!isAudioClientStarted) {
		StartAudioClient(&m_pAudioClient);
	}

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
	if (m_pAudioClient != NULL && pWaveFormatEx == NULL) {
		return hr;
	}

	// Just create the audio client if no WAVEFORMATEX provided
	if (m_pAudioClient == NULL && pWaveFormatEx == NULL) {
		if (SUCCEEDED(hr)) {
			hr = CreateAudioClient(pMMDevice, &m_pAudioClient);
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

		if (m_useWASAPI == 1 || IsBitstream(pWaveFormatEx)) { // EXCLUSIVE
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

			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pFormat, NULL);
			if (S_OK == hr) {
				CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
			}
		} else if (m_useWASAPI == 2) { // SHARED
			pFormat = pWaveFormatEx;
			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
			if (FAILED(hr)) {
				hr = m_pAudioClient->GetMixFormat(&pDeviceFormat);
				if (FAILED(hr)) {
					TRACE(_T("CMpcAudioRenderer::CheckMediaType() - GetMixFormat() failed\n"));
					return hr;
				}

				pFormat = pDeviceFormat;
				hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
				}
			}
		}

		TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - IsFormatSupported()\n"));
		TRACE(_T("	Input format:\n"));
		DumpWaveFormatEx(pWaveFormatEx);

		TRACE(_T("	Output format:\n"));
		DumpWaveFormatEx(pFormat);

		if (pProps) {
			PropVariantClear(&varConfig);
			SAFE_RELEASE(pProps);
		}

		if (pDeviceFormat) {
			CoTaskMemFree(pDeviceFormat);
		}

		if (S_OK == hr) {
			TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the format\n"));
			StopAudioClient(&m_pAudioClient);
			SAFE_RELEASE(m_pRenderClient);
			SAFE_RELEASE(m_pAudioClient);
			if (SUCCEEDED(hr)) {
				hr = CreateAudioClient(pMMDevice, &m_pAudioClient);
			}
		} else if (S_FALSE == hr) {
			TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client refused the format with a closest match\n"));
			TRACE(_T("	=> ppClosestMatch:\n"));
			DumpWaveFormatEx(sharedClosestMatch);

			CopyWaveFormat(sharedClosestMatch, &m_pWaveFileFormatOutput);
			CoTaskMemFree(sharedClosestMatch);

			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, m_pWaveFileFormatOutput, &sharedClosestMatch);
			if (S_OK == hr) {
				TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the closest match format\n"));
				StopAudioClient(&m_pAudioClient);
				SAFE_RELEASE(m_pRenderClient);
				SAFE_RELEASE(m_pAudioClient);
				if (SUCCEEDED(hr)) {
					hr = CreateAudioClient(pMMDevice, &m_pAudioClient);
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
	} else if (m_pRenderClient == NULL) {
		TRACE(_T("CMpcAudioRenderer::CheckAudioClient() - First initialization of the audio renderer\n"));
	} else {
		return hr;
	}

	SAFE_RELEASE(m_pRenderClient);

	if (SUCCEEDED(hr)) {
		hr = InitAudioClient(m_pWaveFileFormatOutput, &m_pAudioClient, &m_pRenderClient);
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

		for (UINT i = 0 ; i < count ; i++) {
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

BOOL CMpcAudioRenderer::IsBitstream(WAVEFORMATEX *pWaveFormatEx)
{
	if (!pWaveFormatEx) {
		return FALSE;
	}

	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		return FALSE;
	}

	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pWaveFormatEx->cbSize == (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))) {
		WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
		return (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS
				|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD
				|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP);
	}

	return FALSE;
}

HRESULT CMpcAudioRenderer::StartAudioClient(IAudioClient **ppAudioClient)
{
	HRESULT hr = S_OK;

	if (!isAudioClientStarted && (*ppAudioClient)) {
		TRACE(_T("CMpcAudioRenderer::StartAudioClient()\n"));

		// To reduce latency, load the first buffer with data
		// from the audio source before starting the stream.
		RenderWasapiBuffer();

		if (FAILED(hr = (*ppAudioClient)->Start())) {
			TRACE(_T("CMpcAudioRenderer::StartAudioClient() - start audio client failed\n"));
			return hr;
		}
		isAudioClientStarted = true;
		StartRendererThread();
	} else {
		TRACE(_T("CMpcAudioRenderer::StartAudioClient() - already started\n"));
	}

	return hr;
}

HRESULT CMpcAudioRenderer::StopAudioClient(IAudioClient **ppAudioClient)
{
	HRESULT hr = S_OK;

	if (isAudioClientStarted) {
		TRACE(_T("CMpcAudioRenderer::StopAudioClient\n"));

		isAudioClientStarted = false;

		PauseRendererThread();

		if ((*ppAudioClient)) {
			if (FAILED(hr = (*ppAudioClient)->Stop())) {
				TRACE(_T("CMpcAudioRenderer::StopAudioClient() - stopp audio client failed\n"));
				TRACE(_T("Stopping audio client failed\n"));
				return hr;
			}
			if (FAILED(hr = (*ppAudioClient)->Reset())) {
				TRACE(_T("CMpcAudioRenderer::StopAudioClient() - reset audio client failed\n"));
				return hr;
			}
		}
	}

	return hr;
}

HRESULT CMpcAudioRenderer::RenderWasapiBuffer()
{
	CAutoLock cRenderLock(&m_csRender);

	HRESULT hr = S_OK;
	
	UINT32 numFramesPadding = 0;
	if (m_useWASAPI == 2 && !m_bIsBitstream) { // SHARED
		m_pAudioClient->GetCurrentPadding(&numFramesPadding);
	}
	UINT32 numFramesAvailable	= nFramesInBuffer - numFramesPadding;
	UINT32 nAvailableBytes		= numFramesAvailable * m_pWaveFileFormatOutput->nBlockAlign;

	BYTE* pData = NULL;
	hr = m_pRenderClient->GetBuffer(numFramesAvailable, &pData);
	if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
		TRACE(_T("CMpcAudioRenderer::RenderWasapiBuffer() - GetBuffer failed with size %ld : (error %lx)\n"), numFramesAvailable, hr);
#endif
		return hr;
	}

	DWORD bufferFlags = 0;
	if (nAvailableBytes > m_WasapiBuf.GetCount() || m_State != State_Running) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
		if (nAvailableBytes > m_WasapiBuf.GetCount()) {
			TRACE(_T("CMpcAudioRenderer::RenderWasapiBuffer() - Data Event, not enough data, requested: %u[%u], available: %u\n"), nAvailableBytes, numFramesAvailable, m_WasapiBuf.GetCount());
		} else if (m_State != State_Running) {
			TRACE(_T("CMpcAudioRenderer::RenderWasapiBuffer() - Data Event, not running\n"), nAvailableBytes, numFramesAvailable, m_WasapiBuf.GetCount());
		}
#endif
		bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
	} else {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
		TRACE(_T("CMpcAudioRenderer::RenderWasapiBuffer() - Data Event, requested: %u[%u], available: %u\n"), nAvailableBytes, numFramesAvailable, m_WasapiBuf.GetCount());
#endif
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

				size_t samples = nAvailableBytes / (wfeOutput->wBitsPerSample >> 3);

				if (fPCM) {
					if (wfeOutput->wBitsPerSample == 8) {
						gain_uint8(m_dVolume, samples, (uint8_t*)pData);
					} else if (wfeOutput->wBitsPerSample == 16) {
						gain_int16(m_dVolume, samples, (int16_t*)pData);
					} else if (wfeOutput->wBitsPerSample == 24) {
						gain_int24(m_dVolume, samples, pData);
					} else if (wfeOutput->wBitsPerSample == 32) {
						gain_int32(m_dVolume, samples, (int32_t*)pData);
					}
				} else if (fFloat) {
					if (wfeOutput->wBitsPerSample == 32) {
						gain_float(m_dVolume, samples, (float*)pData);
					} else if (wfeOutput->wBitsPerSample == 64) {
						gain_double(m_dVolume, samples, (double*)pData);
					}
				}
			}

			p += nAvailableBytes;

			memmove(base, p, end - p);
			m_WasapiBuf.SetCount(end - p);
		}
	}
				
	hr = m_pRenderClient->ReleaseBuffer(numFramesAvailable, bufferFlags);
	if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
		TRACE(_T("CMpcAudioRenderer::RenderWasapiBuffer() - ReleaseBuffer failed with size %ld (error %lx)\n"), numFramesAvailable, hr);
#endif
		return hr;
	}

	return hr;
}

HRESULT CMpcAudioRenderer::GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod)
{
	if (pWaveFormatEx == NULL) {
		return S_OK;
	}

	UINT32 nBufferSize = 0;

	if (pWaveFormatEx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
		if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
			nBufferSize = 6144;
		} else {
			return S_OK;
		}
	} else {
		WAVEFORMATEXTENSIBLE *wfext = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;

		if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
			nBufferSize = 61440;
		} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
			nBufferSize = 32768;
		} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
			nBufferSize = 24576;
		} else {
			return S_OK;
		}
	}

	*pHnsBufferPeriod = (REFERENCE_TIME)((REFERENCE_TIME)nBufferSize * 10000 * 8 / ((REFERENCE_TIME)pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample *
						1.0 * pWaveFormatEx->nSamplesPerSec) /*+ 0.5*/);
	*pHnsBufferPeriod *= 1000;

	TRACE(_T("CMpcAudioRenderer::GetBufferSize() - set a %lld period for a %ld buffer size\n"), *pHnsBufferPeriod, nBufferSize);

	return S_OK;
}

HRESULT CMpcAudioRenderer::InitAudioClient(WAVEFORMATEX *pWaveFormatEx, IAudioClient **pAudioClient, IAudioRenderClient **ppRenderClient)
{
	TRACE(_T("CMpcAudioRenderer::InitAudioClient()\n"));
	HRESULT hr = S_OK;

	// Initialize the stream to play at the minimum latency.
	hnsPeriod = 0;
	hr = (*pAudioClient)->GetDevicePeriod(NULL, &hnsPeriod);
	hnsPeriod = max(500000, hnsPeriod); // 50 ms - minimal size of buffer

	WAVEFORMATEX *pClosestMatch = NULL;
	hr = (*pAudioClient)->IsFormatSupported((m_useWASAPI == 1 || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
											pWaveFormatEx,
											&pClosestMatch);
	if (pClosestMatch) {
		CoTaskMemFree(pClosestMatch);
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
		hr = (*pAudioClient)->Initialize((m_useWASAPI == 1 || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
									  AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
									  hnsPeriod,
									  (m_useWASAPI == 1 || IsBitstream(pWaveFormatEx)) ? hnsPeriod : 0,
									  pWaveFormatEx, NULL);
	}

	if (FAILED(hr) && hr != AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - FAILED(0x%08x)\n"), hr);
		return hr;
	}

	if (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED == hr) {
		// if the buffer size was not aligned, need to do the alignment dance
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - Buffer size not aligned. Realigning\n"));

		// get the buffer size, which will be aligned
		hr = (*pAudioClient)->GetBufferSize(&nFramesInBuffer);

		// throw away this IAudioClient
		StopAudioClient(pAudioClient);
		SAFE_RELEASE((*ppRenderClient));
		SAFE_RELEASE((*pAudioClient));
		if (SUCCEEDED(hr)) {
			hr = CreateAudioClient(pMMDevice, pAudioClient);
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
			hr = (*pAudioClient)->Initialize((m_useWASAPI == 1 || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
										  AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
										  hnsPeriod,
										  (m_useWASAPI == 1 || IsBitstream(pWaveFormatEx)) ? hnsPeriod : 0,
										  pWaveFormatEx, NULL);
		}

		if (FAILED(hr)) {
			TRACE(_T("CMpcAudioRenderer::InitAudioClient() - Failed to reinitialize the audio client\n"));
			return hr;
		}
	}

	// get the buffer size, which is aligned
	if (SUCCEEDED(hr)) {
		hr = (*pAudioClient)->GetBufferSize(&nFramesInBuffer);
	}

	// enables a client to write output data to a rendering endpoint buffer
	if (SUCCEEDED(hr)) {
		hr = (*pAudioClient)->GetService(__uuidof(IAudioRenderClient), (void**)(ppRenderClient));
	}

	if (FAILED(hr)) {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - service initialization FAILED(0x%08x)\n"), hr);
	} else {
		TRACE(_T("CMpcAudioRenderer::InitAudioClient() - service initialization success, with periodicity of %I64u hundred-nanoseconds = %u frames\n"), hnsPeriod, nFramesInBuffer);

		m_bIsBitstream = IsBitstream(pWaveFormatEx);
	}

	if (SUCCEEDED(hr)) {
		hr = (*pAudioClient)->SetEventHandle(m_hDataEvent);
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

HRESULT CMpcAudioRenderer::EnableMMCSS()
{
	HRESULT hr = S_OK;

	if (m_hTask == NULL) {
		// Ask MMCSS to temporarily boost the thread priority
		// to reduce glitches while the low-latency stream plays.
		DWORD taskIndex = 0;

		if (pfAvSetMmThreadCharacteristicsW) {
			m_hTask = pfAvSetMmThreadCharacteristicsW(_T("Pro Audio"), &taskIndex);
			TRACE(L"CMpcAudioRenderer::EnableMMCSS - Putting thread 0x%x in higher priority for Wasapi mode (lowest latency)\n", ::GetCurrentThreadId());
			if (m_hTask == NULL) {
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}
	}
	return S_OK;
}

HRESULT CMpcAudioRenderer::RevertMMCSS()
{
	HRESULT hr = S_OK;

	if (m_hTask != NULL && pfAvRevertMmThreadCharacteristics != NULL) {
		if (pfAvRevertMmThreadCharacteristics(m_hTask)) {
			return hr; 
		} else {
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	return S_FALSE;
}
