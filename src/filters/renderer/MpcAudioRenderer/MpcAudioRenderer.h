/*
 * (C) 2009-2014 see Authors.txt
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

#pragma once

#include <BaseClasses/streams.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <FunctionDiscoveryKeys_devpkey.h>

#include "MpcAudioRendererSettingsWnd.h"

#include "Mixer.h"
#include "../../../DSUtil/Packet.h"

#define MpcAudioRendererName L"MPC Audio Renderer"

// if you get a compilation error on AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED,
// uncomment the #define below
// #define AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED	AUDCLNT_ERR(0x019)

class __declspec(uuid("601D2A2B-9CDE-40bd-8650-0485E3522727"))
	CMpcAudioRenderer : public CBaseRenderer
	, public IBasicAudio
	, public ISpecifyPropertyPages2
	, public IMpcAudioRendererFilter
{
	CCritSec			m_csRender;
	CMixer				m_Resampler;

	CPacketQueue		m_WasapiQueue;
	CAutoPtr<Packet>	m_CurrentPacket;

	REFERENCE_TIME		m_rtStartTime;

public:
	CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr);
	virtual ~CMpcAudioRenderer();

	HRESULT						Receive(IMediaSample* pSample);
	HRESULT						CheckInputType(const CMediaType* mtIn);
	virtual HRESULT				CheckMediaType(const CMediaType *pmt);
	virtual HRESULT				SetMediaType(const CMediaType *pmt);
	virtual HRESULT				DoRenderSample(IMediaSample *pMediaSample) {return E_NOTIMPL;}

	virtual HRESULT				BeginFlush();
	virtual HRESULT				EndFlush();

	HRESULT						EndOfStream(void);

	DECLARE_IUNKNOWN

	STDMETHODIMP				NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// === IMediaFilter
	STDMETHOD(Run)				(REFERENCE_TIME rtStart);
	STDMETHOD(Stop)				();
	STDMETHOD(Pause)			();

	// === IDispatch (pour IBasicAudio)
	STDMETHOD(GetTypeInfoCount)	(UINT * pctinfo) { return E_NOTIMPL; };
	STDMETHOD(GetTypeInfo)		(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo) { return E_NOTIMPL; };
	STDMETHOD(GetIDsOfNames)	(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) { return E_NOTIMPL; };
	STDMETHOD(Invoke)			(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) { return E_NOTIMPL; };

	// === IBasicAudio
	STDMETHOD(put_Volume)		(long lVolume);
	STDMETHOD(get_Volume)		(long *plVolume);
	STDMETHOD(put_Balance)		(long lBalance) {return E_NOTIMPL;}
	STDMETHOD(get_Balance)		(long *plBalance) {return E_NOTIMPL;}

	// === ISpecifyPropertyPages2
	STDMETHODIMP				GetPages(CAUUID* pPages);
	STDMETHODIMP				CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// === IMpcAudioRendererFilter
	STDMETHODIMP					Apply();
	STDMETHODIMP					SetWasapiMode(INT nValue);
	STDMETHODIMP_(INT)				GetWasapiMode();
	STDMETHODIMP					SetSoundDevice(CString nValue);
	STDMETHODIMP_(CString)			GetSoundDevice();
	STDMETHODIMP_(UINT)				GetMode();
	STDMETHODIMP					GetStatus(WAVEFORMATEX** ppWfxIn, WAVEFORMATEX** ppWfxOut);
	STDMETHODIMP					SetBitExactOutput(BOOL nValue);
	STDMETHODIMP_(BOOL)				GetBitExactOutput();
	STDMETHODIMP					SetSystemLayoutChannels(BOOL nValue);
	STDMETHODIMP_(BOOL)				GetSystemLayoutChannels();
	STDMETHODIMP_(BITSTREAM_MODE)	GetBitstreamMode();

	// CMpcAudioRenderer
private:
	HRESULT						GetReferenceClockInterface(REFIID riid, void **ppv);

	CCritSec					m_csProps;
	CCritSec					m_csCheck;

	WAVEFORMATEX				*m_pWaveFileFormat;
	WAVEFORMATEX				*m_pWaveFileFormatOutput;
	CBaseReferenceClock*		m_pReferenceClock;
	double						m_dRate;
	long						m_lVolume;

	// CMpcAudioRenderer WASAPI methods
	HRESULT						GetAvailableAudioDevices(IMMDeviceCollection **ppMMDevices);
	HRESULT						GetAudioDevice(IMMDevice **ppMMDevice);
	HRESULT						CreateAudioClient(IMMDevice *pMMDevice, IAudioClient **ppAudioClient);
	HRESULT						InitAudioClient(WAVEFORMATEX *pWaveFormatEx, IAudioClient **pAudioClient, IAudioRenderClient **ppRenderClient);
	HRESULT						CheckAudioClient(WAVEFORMATEX *pWaveFormatEx);
	HRESULT						DoRenderSampleWasapi(IMediaSample *pMediaSample);
	HRESULT						GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod);

	bool						IsFormatChanged(const WAVEFORMATEX *pWaveFormatEx, const WAVEFORMATEX *pNewWaveFormatEx);
	bool						CheckFormatChanged(WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx);
	bool						CopyWaveFormat(WAVEFORMATEX *pSrcWaveFormatEx, WAVEFORMATEX **ppDestWaveFormatEx);

	BOOL						IsBitstream(WAVEFORMATEX *pWaveFormatEx);
	HRESULT						SelectFormat(WAVEFORMATEX* pwfx, WAVEFORMATEXTENSIBLE& wfex);
	void						CreateFormat(WAVEFORMATEXTENSIBLE& wfex, WORD wBitsPerSample, WORD nChannels, DWORD dwChannelMask, DWORD nSamplesPerSec);

	HRESULT						StartAudioClient(IAudioClient **ppAudioClient);
	HRESULT						StopAudioClient(IAudioClient **ppAudioClient);

	HRESULT						RenderWasapiBuffer();
	void						CheckBufferStatus();
	void						WasapiFlush();

	inline REFERENCE_TIME		GetClockTime();

	// WASAPI variables
	HMODULE						m_hModule;
	HANDLE						m_hTask;
	WASAPI_MODE					m_WASAPIMode;
	WASAPI_MODE					m_WASAPIModeAfterRestart;
	CString						m_DeviceName;
	IMMDevice					*pMMDevice;
	IAudioClient				*m_pAudioClient;
	IAudioRenderClient			*m_pRenderClient;
	UINT32						nFramesInBuffer;
	REFERENCE_TIME				hnsPeriod;
	bool						isAudioClientStarted;
	double						m_dVolume;
	BOOL						m_bIsBitstream;
	BOOL						m_bUseBitExactOutput;
	BOOL						m_bUseSystemLayoutChannels;
	FILTER_STATE				m_filterState;

	typedef HANDLE				(__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
	typedef BOOL				(__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

	PTR_AvSetMmThreadCharacteristicsW	pfAvSetMmThreadCharacteristicsW;
	PTR_AvRevertMmThreadCharacteristics	pfAvRevertMmThreadCharacteristics;
	
	HRESULT						EnableMMCSS();
	HRESULT						RevertMMCSS();

	// Rendering thread
	static DWORD WINAPI			RenderThreadEntryPoint(LPVOID lpParameter);
	DWORD						RenderThread();
	DWORD						m_nThreadId;
 
	BOOL						m_bThreadPaused;

	HRESULT						StopRendererThread();
	HRESULT						StartRendererThread();
	HRESULT						PauseRendererThread();

	HANDLE						m_hRenderThread;

	HANDLE						m_hDataEvent;
	HANDLE						m_hPauseEvent;
	HANDLE						m_hResumeEvent;
	HANDLE						m_hWaitPauseEvent;
	HANDLE						m_hWaitResumeEvent;
	HANDLE						m_hStopRenderThreadEvent;

	HANDLE						m_hRendererNeedMoreData;
	HANDLE						m_hStopWaitingRenderer;

	CSimpleArray<WORD>			m_wBitsPerSampleList;
	CSimpleArray<WORD>			m_nChannelsList;
	CSimpleArray<DWORD>			m_dwChannelMaskList;

	struct AudioParams {
		WORD	wBitsPerSample;
		DWORD	nSamplesPerSec;

		struct AudioParams(WORD v1, DWORD v2) {
			wBitsPerSample = v1;
			nSamplesPerSec = v2;
		}

		bool operator == (const struct AudioParams& ap) const {
			return ((*this).wBitsPerSample == ap.wBitsPerSample
					&& (*this).nSamplesPerSec == ap.nSamplesPerSec);
		}
	};
	CSimpleArray<AudioParams>	m_AudioParamsList;
};
