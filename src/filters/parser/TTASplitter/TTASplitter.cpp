/*
 *
 * Adaptation for MPC-BE (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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
#include <initguid.h>
#endif
#include <moreuuids.h>

#include "TTASplitter.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CTTASplitter), TTASplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CTTASplitter>, NULL, &sudFilter[0]},
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

CUnknown *WINAPI CTTASplitter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CTTASplitter *pNewObject = DNew CTTASplitter(punk, phr);

	if (!pNewObject) {
		*phr = E_OUTOFMEMORY;
	}

	return pNewObject;
}

CTTASplitter::CTTASplitter(LPUNKNOWN lpunk, HRESULT *phr) :
	CBaseFilter(NAME("CTTASplitter"), lpunk, &m_Lock, CLSID_TTASplitter),
	m_pInputPin(NULL),
	m_pOutputPin(NULL),
	m_rtStart(0),
	m_rtStop(0),
	m_rtDuration(0),
	m_dRateSeeking(1.0)
{
	m_dwSeekingCaps = AM_SEEKING_CanGetDuration | AM_SEEKING_CanGetStopPos | AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanSeekAbsolute;

	m_pInputPin = DNew CTTASplitterInputPin(this, &m_Lock, phr);
	if (m_pInputPin == NULL) {
		if (phr) {
			*phr = E_OUTOFMEMORY;
		}
		return;
	}

	m_pOutputPin = DNew CTTASplitterOutputPin(this, &m_Lock, phr);
	if (m_pOutputPin == NULL) {
		if (phr) {
			*phr = E_OUTOFMEMORY;
		}
		return;
	}
}

CTTASplitter::~CTTASplitter()
{
	delete m_pInputPin;
	m_pInputPin = NULL;
	delete m_pOutputPin;
	m_pOutputPin = NULL;
}

STDMETHODIMP CTTASplitter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv,E_POINTER);

	if (riid == IID_IMediaSeeking) {
		return GetInterface((IMediaSeeking*)this, ppv);
	}

	return CBaseFilter::NonDelegatingQueryInterface(riid,ppv);
}

int CTTASplitter::GetPinCount()
{
	CAutoLock lock(m_pLock);
	return 2;
}

CBasePin* CTTASplitter::GetPin(int n)
{
	CAutoLock lock(m_pLock);

	if (n == 0) {
		return m_pInputPin;
	} else if (n == 1) {
		return m_pOutputPin;
	}

	return NULL;
}

STDMETHODIMP CTTASplitter::Stop(void)
{
	return CBaseFilter::Stop();
}

STDMETHODIMP CTTASplitter::Pause(void)
{
	CAutoLock cObjectLock(m_pLock); 

	if (m_State == State_Stopped) {

		if (m_pOutputPin->IsConnected()) {
			HRESULT hr = m_pOutputPin->Active();

			if (FAILED(hr)) {
				return hr;
			}
		}

		if (m_pInputPin->IsConnected()) {
			HRESULT hr = m_pInputPin->Active();

			if (FAILED(hr)) {
				return hr;
			}
		}
	}

	m_State = State_Paused;

	return S_OK;
}

STDMETHODIMP CTTASplitter::Run(REFERENCE_TIME tStart)
{
	return CBaseFilter::Run(tStart);
}

void CTTASplitter::SetDuration(REFERENCE_TIME rtDuration)
{
	m_rtStart		= 0; 
	m_rtStop		= rtDuration; 
	m_rtDuration	= rtDuration;
}

HRESULT CTTASplitter::BeginFlush()
{
	CAutoLock lock(m_pLock);

	m_pOutputPin->DeliverBeginFlush();
	m_pInputPin->BeginFlush();

	return NOERROR;
}

HRESULT CTTASplitter::EndFlush()
{
	CAutoLock lock(m_pLock);

	m_pOutputPin->DeliverEndFlush();
	m_pInputPin->EndFlush();

	return NOERROR;
}

HRESULT CTTASplitter::DoSeeking()
{
	return m_pInputPin->DoSeeking(m_rtStart);
}

CTTASplitterInputPin::CTTASplitterInputPin(CTTASplitter *pParentFilter, CCritSec *pLock, HRESULT *phr) :
	CBaseInputPin(NAME("CTTASplitterInputPin"), (CBaseFilter*)pParentFilter, pLock, phr, L"Input"),      
	m_pParentFilter(pParentFilter),
	m_pReader(NULL),
	m_bDiscontinuity(FALSE),
	m_pIACBW(NULL),
	m_pTTAParser(NULL)
{
}

CTTASplitterInputPin::~CTTASplitterInputPin()
{
	if (m_pTTAParser) {
		tta_parser_free(&m_pTTAParser);
	}
}

HRESULT CTTASplitterInputPin::CheckMediaType(const CMediaType *pmt)
{
	if (*pmt->Type() != MEDIATYPE_Stream) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CTTASplitterInputPin::CheckConnect(IPin *pPin)
{
	HRESULT hr = CBaseInputPin::CheckConnect(pPin);
	if (FAILED(hr)) {
		return hr;
	}

	hr = pPin->QueryInterface(IID_IAsyncReader, (void**)&m_pReader);
	if (FAILED(hr)) {
		return S_FALSE;
	}

	if (m_pIACBW) {
		IAsyncCallBackWrapper_tta_free(&m_pIACBW);
	}

	m_pIACBW = IAsyncCallBackWrapper_tta_new(m_pReader);

	return S_OK;
}

HRESULT CTTASplitterInputPin::BreakConnect(void)
{
	HRESULT hr = CBaseInputPin::BreakConnect();
	if (FAILED(hr)) {
		return hr;
	}

	if(m_pIACBW) {
		IAsyncCallBackWrapper_tta_free(&m_pIACBW);
	}

	if (m_pReader) {
		m_pReader->Release(); 
		m_pReader = NULL; 
	}

	return S_OK;
}

HRESULT CTTASplitterInputPin::CompleteConnect(IPin *pReceivePin)
{
	HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	if(!m_pIACBW) {
		return E_FAIL;
	}

	if(m_pTTAParser) {
		tta_parser_free(&m_pTTAParser);
	}

	m_pTTAParser = tta_parser_new((TTA_io_callback*)m_pIACBW);
	if (!m_pTTAParser) {
		return E_FAIL;
	}

	REFERENCE_TIME rtDuration;
	rtDuration = ((m_pTTAParser->FrameTotal - 1) * m_pTTAParser->FrameLen + m_pTTAParser->LastFrameLen);
	rtDuration = (rtDuration * 10000000) / m_pTTAParser->TTAHeader.SampleRate;
	m_pParentFilter->SetDuration(rtDuration);

	return S_OK;
}

DWORD CTTASplitterInputPin::ThreadProc()
{
	DWORD com;

	do {
		com = GetRequest();

		switch (com) {
			case CMD_EXIT:
				Reply(NOERROR); 
				break; 
			case CMD_STOP:
				Reply(NOERROR); 
				break; 
			case CMD_RUN:
				DoProcessingLoop();
				break; 
		}
	} while (com != CMD_EXIT);

	return NOERROR; 
}

HRESULT CTTASplitterInputPin::DoProcessingLoop(void)
{
	DWORD com;
	IMediaSample *pSample;
	BYTE *Buffer;
	HRESULT hr;
	unsigned long FrameLenBytes, FrameLenSamples, FrameIndex;

	Reply(NOERROR);
	m_bAbort = FALSE;

	m_pParentFilter->m_pOutputPin->DeliverNewSegment(0,
	m_pParentFilter->m_rtStop - m_pParentFilter->m_rtStart,
	m_pParentFilter->m_dRateSeeking);

	do {
		if (m_pIACBW->StreamPos >= m_pIACBW->StreamLen || tta_parser_eof(m_pTTAParser)) {
			m_pParentFilter->m_pOutputPin->DeliverEndOfStream();
			return NOERROR;
		}

		hr = m_pParentFilter->m_pOutputPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0); 
		if (FAILED(hr)) {
			return hr;
		}

		hr = pSample->GetPointer(&Buffer);
		if (FAILED(hr)) {
			pSample->Release();
			return hr;
		}

		FrameLenBytes = tta_parser_read_frame(m_pTTAParser, Buffer, &FrameIndex, &FrameLenSamples);
		if (!FrameLenBytes) {
			pSample->Release();
			m_pParentFilter->m_pOutputPin->DeliverEndOfStream();
			return hr;
		}

		pSample->SetActualDataLength(FrameLenBytes);

		REFERENCE_TIME rtStart, rtStop;
		rtStart	= (FrameIndex * m_pTTAParser->FrameLen);
		rtStop	= rtStart + FrameLenSamples;
		rtStart	= (rtStart * 10000000) / m_pTTAParser->TTAHeader.SampleRate;
		rtStop	= (rtStop * 10000000) / m_pTTAParser->TTAHeader.SampleRate;

		rtStart	-= m_pParentFilter->m_rtStart;
		rtStop	-= m_pParentFilter->m_rtStart;

		pSample->SetTime(&rtStart, &rtStop);
		pSample->SetPreroll(FALSE);
		pSample->SetDiscontinuity(m_bDiscontinuity);

		if (m_bDiscontinuity) {
			m_bDiscontinuity = FALSE;
		}
		pSample->SetSyncPoint(TRUE);

		hr = m_pParentFilter->m_pOutputPin->Deliver(pSample);
		pSample->Release();
		pSample = NULL;

		if (FAILED(hr)) {
			return hr;
		}
	} while (!CheckRequest((DWORD*)&com) && !m_bAbort);

	return NOERROR;
}

HRESULT CTTASplitterInputPin::Active()
{
	HRESULT hr;

	if (m_pParentFilter->IsActive()) {
		return S_FALSE;
	}

	if (!IsConnected()) {
		return NOERROR;
	}

	hr = CBaseInputPin::Active();
	if (FAILED(hr)) {
		return hr;
	}

	if (!Create()) {
		return E_FAIL;
	}

	CallWorker(CMD_RUN);

	return NOERROR;
}

HRESULT CTTASplitterInputPin::Inactive()
{
	if (ThreadExists()) {
		m_bAbort = TRUE;
		CallWorker(CMD_EXIT);
		Close();
	}

	return CBasePin::Inactive();
}

STDMETHODIMP CTTASplitterInputPin::BeginFlush()
{
	HRESULT hr = CBaseInputPin::BeginFlush();
	if (FAILED(hr)) {
		return hr;
	}

	if (!ThreadExists()) {
		return NOERROR;
	}

	m_bAbort = TRUE;
	CallWorker(CMD_STOP);

	return  NOERROR;
}

STDMETHODIMP CTTASplitterInputPin::EndFlush()
{
	HRESULT hr = CBaseInputPin::EndFlush();
	if (FAILED(hr)) {
		return hr;
	}

	if (ThreadExists()) {
		CallWorker(CMD_RUN);
	}

	return NOERROR; 
}

HRESULT CTTASplitterInputPin::DoSeeking(REFERENCE_TIME rtStart)
{
	tta_parser_seek(m_pTTAParser, rtStart);
	m_bDiscontinuity = TRUE;

	return S_OK;
}

CTTASplitterOutputPin::CTTASplitterOutputPin(CTTASplitter *pParentFilter, CCritSec *pLock, HRESULT *phr) :
	CBaseOutputPin(NAME("CTTASplitterOutputPin"), (CBaseFilter*)pParentFilter, pLock, phr, L"Output"),
	m_pParentFilter(pParentFilter)
{
}

HRESULT CTTASplitterOutputPin::CheckMediaType(const CMediaType *pmt)
{
	ASSERT(m_pParentFilter->m_pInputPin != NULL);

	if ((m_pParentFilter->m_pInputPin->IsConnected() == FALSE)) {
		return E_INVALIDARG;
	}

	if ((*pmt->Type() != MEDIATYPE_Audio) || (*pmt->Subtype() != MEDIASUBTYPE_TTA1) || (*pmt->FormatType() != FORMAT_WaveFormatEx)) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return m_pParentFilter->m_pInputPin->CheckMediaType(&m_pParentFilter->m_pInputPin->CurrentMediaType());
}

HRESULT CTTASplitterOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (!m_pParentFilter->m_pInputPin->IsConnected()) {
		return E_UNEXPECTED;
	}

	if (iPosition < 0) {
		return E_INVALIDARG;
	}

	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	TTA_header *ttahdr = m_pParentFilter->GetTTAHeader();
	if (ttahdr == NULL) {
		return E_UNEXPECTED;
	}

	TTA_parser *ttaparser = m_pParentFilter->GetTTAParser();
	if (ttaparser == NULL) {
		return E_UNEXPECTED;
	}

	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Audio);
	pMediaType->SetSubtype(&MEDIASUBTYPE_TTA1);
	pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
	pMediaType->SetVariableSize();

	WAVEFORMATEX* wfe	= (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX) + ttaparser->extradata_size);
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	wfe->wFormatTag		= WAVE_FORMAT_TTA1;
	wfe->wBitsPerSample	= ttahdr->BitsPerSample;
	wfe->nChannels		= ttahdr->NumChannels;
	wfe->nSamplesPerSec	= ttahdr->SampleRate;
	wfe->cbSize			= ttaparser->extradata_size;

	if (wfe->cbSize) {
		memcpy((BYTE*)(wfe+1), ttaparser->extradata, ttaparser->extradata_size);
	}

	return S_OK;
}

HRESULT CTTASplitterOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	if (!m_pParentFilter->m_pInputPin->IsConnected()) {
		return E_UNEXPECTED;
	}

	TTA_header* ttahdr	= m_pParentFilter->GetTTAHeader();
	pProp->cBuffers		= 1;
	pProp->cbBuffer		= m_pParentFilter->GetMaxFrameLenBytes();

	if (pProp->cbBuffer == 0) {
		pProp->cbBuffer = tta_codec_get_frame_len(ttahdr->SampleRate) * ttahdr->NumChannels * (ttahdr->BitsPerSample / 8) + sizeof(ttahdr->CRC32);
	}

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) {
		return hr;
	}

	if (Actual.cbBuffer < pProp->cbBuffer || Actual.cBuffers < pProp->cBuffers) {
		return E_INVALIDARG;
	}

	return S_OK;
}

HRESULT CTTASplitterOutputPin::IsFormatSupported(const GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CTTASplitterOutputPin::QueryPreferredFormat(GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

HRESULT CTTASplitterOutputPin::SetTimeFormat(const GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}

HRESULT CTTASplitterOutputPin::IsUsingTimeFormat(const GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CTTASplitterOutputPin::GetTimeFormat(GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

HRESULT CTTASplitterOutputPin::GetDuration(LONGLONG *pDuration)
{
	CheckPointer(pDuration, E_POINTER);

	CAutoLock lock(m_pParentFilter->m_pLock);
	*pDuration = m_pParentFilter->m_rtDuration;

	return S_OK;
}

HRESULT CTTASplitterOutputPin::GetStopPosition(LONGLONG *pStop)
{
	CheckPointer(pStop, E_POINTER);

	CAutoLock lock(m_pParentFilter->m_pLock);
	*pStop = m_pParentFilter->m_rtStop;

	return S_OK;
}

HRESULT CTTASplitterOutputPin::GetCurrentPosition(LONGLONG *pCurrent)
{
	return E_NOTIMPL;
}

HRESULT CTTASplitterOutputPin::GetCapabilities(DWORD *pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	*pCapabilities = m_pParentFilter->m_dwSeekingCaps;

	return S_OK;
}

HRESULT CTTASplitterOutputPin::CheckCapabilities(DWORD *pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	return (~m_pParentFilter->m_dwSeekingCaps & *pCapabilities) ? S_FALSE : S_OK;
}

HRESULT CTTASplitterOutputPin::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
	CheckPointer(pTarget, E_POINTER);

	if (pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME) {
		if (pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME) {
			*pTarget = Source;
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

HRESULT CTTASplitterOutputPin::SetPositions(LONGLONG *pCurrent, DWORD CurrentFlags, LONGLONG *pStop, DWORD StopFlags)
{
	DWORD StopPosBits	= StopFlags & AM_SEEKING_PositioningBitsMask;
	DWORD StartPosBits	= CurrentFlags & AM_SEEKING_PositioningBitsMask;

	if (StopFlags) {
		CheckPointer(pStop, E_POINTER);

		if (StopPosBits != StopFlags) {
			return E_INVALIDARG;
		}
	}

	if (CurrentFlags) {
		CheckPointer(pCurrent, E_POINTER);

		if (StartPosBits != AM_SEEKING_AbsolutePositioning && StartPosBits != AM_SEEKING_RelativePositioning) {
			return E_INVALIDARG;
		}
	}

	CAutoLock lock(m_pParentFilter->m_pLock);

	if (StartPosBits == AM_SEEKING_AbsolutePositioning) {
		m_pParentFilter->m_rtStart = *pCurrent;
	} else if (StartPosBits == AM_SEEKING_RelativePositioning) {
		m_pParentFilter->m_rtStart += *pCurrent;
	}
	
	if (StopPosBits == AM_SEEKING_AbsolutePositioning) {
		m_pParentFilter->m_rtStop = *pStop;
	} else if (StopPosBits == AM_SEEKING_IncrementalPositioning) {
		m_pParentFilter->m_rtStop = m_pParentFilter->m_rtStart + *pStop;
	} else if (StopPosBits == AM_SEEKING_RelativePositioning) {
		m_pParentFilter->m_rtStop = m_pParentFilter->m_rtStop + *pStop;
	}

	HRESULT hr = S_OK;

	if (StartPosBits) {      
		m_pParentFilter->BeginFlush();
		m_pParentFilter->DoSeeking();
		m_pParentFilter->EndFlush();
	} else if (StopPosBits) {
		m_pParentFilter->BeginFlush();
		m_pParentFilter->EndFlush();
	}

	return hr;
}

HRESULT CTTASplitterOutputPin::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
	if (pCurrent) {
		*pCurrent = m_pParentFilter->m_rtStart;
	}

	if (pStop) {
		*pStop = m_pParentFilter->m_rtStop;
	}

	return S_OK;
}

HRESULT CTTASplitterOutputPin::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
{
	if (pEarliest) {
		*pEarliest = 0;
	}

	if (pLatest) {
		CAutoLock lock(m_pParentFilter->m_pLock);
		*pLatest = m_pParentFilter->m_rtDuration;
	}

	return S_OK;
}

HRESULT CTTASplitterOutputPin::SetRate(double dRate)
{
	CAutoLock lock(m_pParentFilter->m_pLock);

	m_pParentFilter->m_dRateSeeking = dRate;
	m_pParentFilter->BeginFlush();
	m_pParentFilter->EndFlush();

	return S_OK;
}

HRESULT CTTASplitterOutputPin::GetRate(double *pdRate)
{
	CheckPointer(pdRate, E_POINTER);

	CAutoLock lock(m_pParentFilter->m_pLock);
	*pdRate = m_pParentFilter->m_dRateSeeking;

	return S_OK;
}

HRESULT CTTASplitterOutputPin::GetPreroll(LONGLONG *pPreroll)
{
	CheckPointer(pPreroll, E_POINTER);
	*pPreroll = 0;

	return S_OK;
}

int IAsyncCallBackWrapper_tta_read(TTA_io_callback *io, void *buff, int size)
{
	IAsyncCallBackWrapper_tta* iacbw = (IAsyncCallBackWrapper_tta*)io;

	HRESULT hr = iacbw->pReader->SyncRead(iacbw->StreamPos, size, (BYTE*)buff);
	if (hr == S_OK) {
		iacbw->StreamPos += size;
	}

	return (hr == S_OK);
}

int IAsyncCallBackWrapper_tta_seek(TTA_io_callback *io, int offset, int origin)
{
	IAsyncCallBackWrapper_tta* iacbw = (IAsyncCallBackWrapper_tta*)io;

	switch (origin) {
		case SEEK_SET:
			iacbw->StreamPos = offset;
			break;
		case SEEK_CUR:
			iacbw->StreamPos += offset;
			break;
		case SEEK_END:
			iacbw->StreamPos = iacbw->StreamLen - offset;
			break;
	}

	return 1;
}

IAsyncCallBackWrapper_tta* IAsyncCallBackWrapper_tta_new(IAsyncReader *pReader)
{
	IAsyncCallBackWrapper_tta* iacbw = DNew IAsyncCallBackWrapper_tta;

	if (!iacbw) {
		return NULL;
	}

	iacbw->iocallback.read = IAsyncCallBackWrapper_tta_read;
	iacbw->iocallback.seek = IAsyncCallBackWrapper_tta_seek;
	iacbw->pReader = pReader;
	iacbw->StreamPos = 0;

	LONGLONG Available;
	pReader->Length(&iacbw->StreamLen, &Available);

	return iacbw;
}

void IAsyncCallBackWrapper_tta_free(IAsyncCallBackWrapper_tta **iacbw)
{
	delete (*iacbw);
	*iacbw = NULL;
}
