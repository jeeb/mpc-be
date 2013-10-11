/*
 *
 * Adaptation for MPC-BE (C) 2012 Dmitry "Vortex" Koteroff (vortex@light-alloy.ru, http://light-alloy.ru)
 *
 * This file is part of MPC-BE and Light Alloy.
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
#include <windows.h>
#include <initguid.h>
#include <moreuuids.h>
#include <mmreg.h>

#include "WVSplitter.h"
#include "../DSUtil/ApeTag.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVPACK_Stream},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CWavPackSplitterFilter), WavPackSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CWavPackSplitterFilter>, NULL, &sudFilter[0]}
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

#define constrain(x,y,z) (((y) < (x)) ? (x) : ((y) > (z)) ? (z) : (y))

CUnknown *WINAPI CWavPackSplitterFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
	CWavPackSplitterFilter *pNewObject = DNew CWavPackSplitterFilter(punk, phr);
	if (!pNewObject) {
		*phr = E_OUTOFMEMORY;
	}
	return pNewObject;
}

CWavPackSplitterFilter::CWavPackSplitterFilter(LPUNKNOWN lpunk, HRESULT *phr) :
	CBaseFilter(NAME("CWavPackSplitterFilter"), lpunk, &m_Lock, CLSID_WavPackSplitter),
	m_pInputPin(NULL),
	m_pOutputPin(NULL),
	m_rtStart(0),
	m_rtStop(0),
	m_rtDuration(0),
	m_dRateSeeking(1.0)
{
	m_dwSeekingCaps = AM_SEEKING_CanGetDuration
		| AM_SEEKING_CanGetStopPos
		| AM_SEEKING_CanSeekForwards
		| AM_SEEKING_CanSeekBackwards
		| AM_SEEKING_CanSeekAbsolute;

	m_pInputPin = DNew CWavPackSplitterFilterInputPin(this, &m_Lock, phr);
	if (m_pInputPin == NULL) {
		if (phr) {
			*phr = E_OUTOFMEMORY;
		}
		return;
	}

	m_pOutputPin = DNew CWavPackSplitterFilterOutputPin(this, &m_Lock, phr);
	if (m_pOutputPin == NULL) {
		if (phr) {
			*phr = E_OUTOFMEMORY;
		}
		return;
	}
}

CWavPackSplitterFilter::~CWavPackSplitterFilter()
{
	delete m_pInputPin;
	m_pInputPin = NULL;
	delete m_pOutputPin;
	m_pOutputPin = NULL;
}

STDMETHODIMP CWavPackSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI2(IAMMediaContent)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		QI(IDSMPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// CBaseFilter
int CWavPackSplitterFilter::GetPinCount()
{
	CAutoLock lock(m_pLock);

	return 2;
}

CBasePin* CWavPackSplitterFilter::GetPin(int n)
{
	CAutoLock lock(m_pLock);

	if(n == 0) {
		return m_pInputPin;
	} else if(n == 1) {
		return m_pOutputPin;
	}

	return NULL;
}

STDMETHODIMP CWavPackSplitterFilter::Stop(void)
{
	return CBaseFilter::Stop();
}

STDMETHODIMP CWavPackSplitterFilter::Pause(void)
{
	CAutoLock cObjectLock(m_pLock);

	// notify all pins of the change to active state
	if (m_State == State_Stopped) {
		// Order is important, the output pin allocator need to be commited
		// when we activate the input pin

		// First the output pin
		if (m_pOutputPin->IsConnected()) {
			HRESULT hr = m_pOutputPin->Active();
			if (FAILED(hr)) {
				return hr;
			}
		}
		// Then the input pin
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

STDMETHODIMP CWavPackSplitterFilter::Run(REFERENCE_TIME tStart)
{
	return CBaseFilter::Run(tStart);
}

void CWavPackSplitterFilter::SetDuration(REFERENCE_TIME rtDuration)
{
	m_rtStart = 0;
	m_rtStop = rtDuration;
	m_rtDuration = rtDuration;
}

HRESULT CWavPackSplitterFilter::BeginFlush()
{
	CAutoLock lock(m_pLock);

	// Call DeliverBeginFlush on output pin first so GetDeliveryBuffer doesn't lock
	// in input pin DoProcessingLoop
	m_pOutputPin->DeliverBeginFlush();
	// Suspend DoProcessingLoop
	m_pInputPin->BeginFlush();

	return NOERROR;
}

HRESULT CWavPackSplitterFilter::EndFlush()
{
	CAutoLock lock(m_pLock);

	m_pOutputPin->DeliverEndFlush();
	m_pInputPin->EndFlush();
	return NOERROR;
}

HRESULT CWavPackSplitterFilter::DoSeeking()
{
	return m_pInputPin->DoSeeking(m_rtStart);
}

STDMETHODIMP CWavPackSplitterFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
	return CBaseFilter::JoinFilterGraph(pGraph,pName);
}

// IAMMediaContent

STDMETHODIMP CWavPackSplitterFilter::get_AuthorName(BSTR* pbstrAuthorName)
{
	return GetProperty(L"AUTH", pbstrAuthorName);
}

STDMETHODIMP CWavPackSplitterFilter::get_Title(BSTR* pbstrTitle)
{
	return GetProperty(L"TITL", pbstrTitle);
}

STDMETHODIMP CWavPackSplitterFilter::get_Description(BSTR* pbstrDescription)
{
	return GetProperty(L"DESC", pbstrDescription);
}

STDMETHODIMP CWavPackSplitterFilter::get_Copyright(BSTR* pbstrCopyright)
{
	return GetProperty(L"CPYR", pbstrCopyright);
}

// ============================================================================

CWavPackSplitterFilterInputPin::CWavPackSplitterFilterInputPin(
	CWavPackSplitterFilter *pParentFilter, CCritSec *pLock, HRESULT * phr) :
	CBaseInputPin(NAME("CWavPackSplitterFilterInputPin"),
		(CBaseFilter *) pParentFilter,
		pLock,
		phr,
		L"Input"),
	m_pParentFilter(pParentFilter),
	m_pReader(NULL),
	m_bDiscontinuity(TRUE),
	m_pIACBW(NULL),
	m_pWavPackParser(NULL)
{
}

CWavPackSplitterFilterInputPin::~CWavPackSplitterFilterInputPin()
{
	if (m_pWavPackParser) {
		wavpack_parser_free(m_pWavPackParser);
		m_pWavPackParser = NULL;
	}
}

HRESULT CWavPackSplitterFilterInputPin::CheckMediaType(const CMediaType *pmt)
{
	if (*pmt->Type() == MEDIATYPE_Stream && m_pIACBW) {
		if (m_pWavPackParser) {
			wavpack_parser_free(m_pWavPackParser);
			m_pWavPackParser = NULL;
		}

		m_pWavPackParser = wavpack_parser_new((stream_reader*)m_pIACBW, FALSE);
		if (!m_pWavPackParser) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		return S_OK;
	}
	else {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
}

HRESULT CWavPackSplitterFilterInputPin::CheckConnect(IPin* pPin)
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
		IAsyncCallBackWrapper_wv_free(m_pIACBW);
		m_pIACBW = NULL;
	}
	m_pIACBW = IAsyncCallBackWrapper_wv_new(m_pReader);

	return S_OK;
}

HRESULT CWavPackSplitterFilterInputPin::BreakConnect(void)
{
	HRESULT hr = CBaseInputPin::BreakConnect();
	if (FAILED(hr)) {
		return hr;
	}

	if (m_pIACBW) {
		IAsyncCallBackWrapper_wv_free(m_pIACBW);
		m_pIACBW = NULL;
	}

	if (m_pReader) {
		m_pReader->Release();
		m_pReader = NULL;
	}

	return S_OK;
}

HRESULT CWavPackSplitterFilterInputPin::CompleteConnect(IPin *pReceivePin)
{
	HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	if (m_pWavPackParser) {
		wavpack_parser_free(m_pWavPackParser);
		m_pWavPackParser = NULL;
	}

	m_pWavPackParser = wavpack_parser_new((stream_reader*)m_pIACBW, FALSE);
	if (!m_pWavPackParser) {
		return E_FAIL;
	}

	// Compute total duration
	REFERENCE_TIME rtDuration;
	rtDuration = m_pWavPackParser->first_wphdr.total_samples;
	rtDuration = (rtDuration * 10000000) / m_pWavPackParser->sample_rate;
	m_pParentFilter->SetDuration(rtDuration);

	// parse APE Tag Header
	stream_reader* io = m_pWavPackParser->io;
	char buf[APE_TAG_FOOTER_BYTES];
	memset(buf, 0, sizeof(buf));
	uint32_t cur_pos = io->get_pos(io);
	DWORD file_size = io->get_length(io);
	if (cur_pos + APE_TAG_FOOTER_BYTES <= file_size) {
		io->set_pos_rel(io, -APE_TAG_FOOTER_BYTES, SEEK_END);
		if (io->read_bytes(io, buf, APE_TAG_FOOTER_BYTES) == APE_TAG_FOOTER_BYTES) {
			CAPETag* APETag = DNew CAPETag;
			if (APETag->ReadFooter((BYTE*)buf, APE_TAG_FOOTER_BYTES) && APETag->GetTagSize()) {
				size_t tag_size = APETag->GetTagSize();
				io->set_pos_rel(io, file_size - tag_size, SEEK_SET);
				BYTE *p = DNew BYTE[tag_size];
				if (io->read_bytes(io, p, tag_size) == tag_size && APETag->ReadTags(p, tag_size)) {
					APETag->ParseTags(m_pParentFilter);
				}

				delete [] p;
			}

			delete APETag;
		}

		io->set_pos_rel(io, cur_pos, SEEK_SET);
	}

	return S_OK;
}

DWORD CWavPackSplitterFilterInputPin::ThreadProc()
{
	DWORD cmd;

	do
	{
		cmd = GetRequest();
		switch (cmd) {
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
	} while (cmd != CMD_EXIT);

	return NOERROR;
}

HRESULT CWavPackSplitterFilterInputPin::DeliverOneFrame(WavPack_parser* wpp)
{
	IMediaSample *pSample;
	BYTE *Buffer = NULL;
	HRESULT hr;
	unsigned long FrameLenBytes = 0, FrameLenSamples = 0, FrameIndex = 0;

	// Get a new media sample
	hr = m_pParentFilter->m_pOutputPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
	if (FAILED(hr)) {
		return hr;
	}

	hr = pSample->GetPointer(&Buffer);
	if (FAILED(hr)) {
		pSample->Release();
		return hr;
	}

	FrameLenBytes = wavpack_parser_read_frame(wpp, Buffer,
											  &FrameIndex, &FrameLenSamples);
	if (!FrameLenBytes) {
		// Something bad happened, let's end here
		pSample->Release();
		m_pParentFilter->m_pOutputPin->DeliverEndOfStream();
		// TODO : check if we need to stop the thread
		return hr;
	}
	pSample->SetActualDataLength(FrameLenBytes);

	REFERENCE_TIME rtStart, rtStop;
	rtStart = FrameIndex - wpp->first_wphdr.block_index;
	rtStop = rtStart + FrameLenSamples;
	rtStart = (rtStart * 10000000) / wpp->sample_rate;
	rtStop = (rtStop * 10000000) / wpp->sample_rate;

	rtStart -= m_pParentFilter->m_rtStart;
	rtStop  -= m_pParentFilter->m_rtStart;

	pSample->SetTime(&rtStart, &rtStop);
	pSample->SetPreroll(FALSE);
	pSample->SetDiscontinuity(m_bDiscontinuity);
	if (m_bDiscontinuity) {
		m_bDiscontinuity = FALSE;
	}
	pSample->SetSyncPoint(TRUE);

	// Deliver the sample
	hr = m_pParentFilter->m_pOutputPin->Deliver(pSample);
	pSample->Release();
	pSample = NULL;
	if (FAILED(hr)) {
		return hr;
	}

	return S_OK;
}

HRESULT CWavPackSplitterFilterInputPin::DoProcessingLoop(void)
{
	DWORD cmd;
	HRESULT hr;

	Reply(NOERROR);
	m_bAbort = FALSE;

	m_pParentFilter->m_pOutputPin->DeliverNewSegment(0,
	m_pParentFilter->m_rtStop - m_pParentFilter->m_rtStart,
	m_pParentFilter->m_dRateSeeking);

	do
	{
		if (m_pIACBW->StreamPos >= m_pIACBW->StreamLen || wavpack_parser_eof(m_pWavPackParser)) {
			// EOF
			m_pParentFilter->m_pOutputPin->DeliverEndOfStream();
			// TODO : check if we need to stop the thread
			return NOERROR;
		}

		hr = DeliverOneFrame(m_pWavPackParser);
		if (FAILED(hr)) {
			return hr;
		}
	} while (!CheckRequest(&cmd) && !m_bAbort);

	return NOERROR;
}

HRESULT CWavPackSplitterFilterInputPin::Active()
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

	// Create and start the thread

	if (!Create()) {
		return E_FAIL;
	}
	CallWorker(CMD_RUN);

	return NOERROR;
}

HRESULT CWavPackSplitterFilterInputPin::Inactive()
{
	// Stop the thread
	if (ThreadExists()) {
		m_bAbort = TRUE;
		CallWorker(CMD_EXIT);
		Close();
	}

	return CBasePin::Inactive();
}

STDMETHODIMP CWavPackSplitterFilterInputPin::BeginFlush()
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

STDMETHODIMP CWavPackSplitterFilterInputPin::EndFlush()
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

HRESULT CWavPackSplitterFilterInputPin::DoSeeking(REFERENCE_TIME rtStart)
{
	wavpack_parser_seek(m_pWavPackParser, rtStart);
	m_bDiscontinuity = TRUE;

	return S_OK;
}

// ============================================================================

CWavPackSplitterFilterOutputPin::CWavPackSplitterFilterOutputPin(
	CWavPackSplitterFilter *pParentFilter, CCritSec *pLock, HRESULT * phr) :
	CBaseOutputPin(NAME("CWavPackSplitterFilterOutputPin"),
						(CBaseFilter *) pParentFilter,
						pLock,
						phr,
						L"Output"),
	m_pParentFilter(pParentFilter)
{
}

HRESULT CWavPackSplitterFilterOutputPin::CheckMediaType(const CMediaType *pmt)
{
	// must have selected input first
	ASSERT(m_pParentFilter->m_pInputPin != NULL);
	if ((m_pParentFilter->m_pInputPin->IsConnected() == FALSE)) {
		return E_INVALIDARG;
	}

	if ((*pmt->Type() != MEDIATYPE_Audio) ||
		((*pmt->Subtype() != MEDIASUBTYPE_WAVPACK4) &&
		(*pmt->Subtype() != MEDIASUBTYPE_WavpackHybrid)) &&
		(*pmt->FormatType() != FORMAT_WaveFormatEx)) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return m_pParentFilter->m_pInputPin->CheckMediaType(&m_pParentFilter->m_pInputPin->CurrentMediaType());
}

HRESULT CWavPackSplitterFilterOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (!m_pParentFilter->m_pInputPin->IsConnected()) {
		return E_UNEXPECTED;
	}

	if (iPosition < 0){
		return E_INVALIDARG;
	}

	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	WavPack_parser* wpp = m_pParentFilter->GetWavPackParser();
	if (wpp == NULL) {
		return E_UNEXPECTED;
	}

	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Audio);
	pMediaType->SetSubtype(&MEDIASUBTYPE_WAVPACK4);
	pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
	pMediaType->SetVariableSize();

	WAVEFORMATEX *pwfxout = (WAVEFORMATEX*)pMediaType->AllocFormatBuffer(
	sizeof(WAVEFORMATEX) + sizeof(wavpack_codec_private_data));
	ZeroMemory(pwfxout, sizeof(WAVEFORMATEX) + sizeof(wavpack_codec_private_data));
	pwfxout->wFormatTag = WAVE_FORMAT_WAVPACK4;
	pwfxout->cbSize = sizeof(wavpack_codec_private_data);

	pwfxout->wBitsPerSample = wpp->bits_per_sample;
	pwfxout->nChannels = wpp->channel_count;
	pwfxout->nSamplesPerSec = wpp->sample_rate;

	wavpack_codec_private_data* pd = (wavpack_codec_private_data*)(pwfxout + 1);
	pd->version = wpp->wphdr.version;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	if (!m_pParentFilter->m_pInputPin->IsConnected()) {
		return E_UNEXPECTED;
	}

	WavPack_parser* wpp = m_pParentFilter->GetWavPackParser();
	pProp->cBuffers = 2;
	pProp->cbBuffer = wpp->suggested_buffer_size;

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

// IMediaSeeking

HRESULT CWavPackSplitterFilterOutputPin::IsFormatSupported(const GUID * pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	// only seeking in time (REFERENCE_TIME units) is supported

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CWavPackSplitterFilterOutputPin::QueryPreferredFormat(GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::SetTimeFormat(const GUID * pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	// nothing to set; just check that it's TIME_FORMAT_TIME

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}

HRESULT CWavPackSplitterFilterOutputPin::IsUsingTimeFormat(const GUID * pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CWavPackSplitterFilterOutputPin::GetTimeFormat(GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::GetDuration(LONGLONG *pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CAutoLock lock(m_pParentFilter->m_pLock);

	*pDuration = m_pParentFilter->m_rtDuration;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::GetStopPosition(LONGLONG *pStop)
{
	CheckPointer(pStop, E_POINTER);
	CAutoLock lock(m_pParentFilter->m_pLock);

	*pStop = m_pParentFilter->m_rtStop;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::GetCurrentPosition(LONGLONG *pCurrent)
{
	// GetCurrentPosition is typically supported only in renderers and
	// not in source filters.
	return E_NOTIMPL;
}

HRESULT CWavPackSplitterFilterOutputPin::GetCapabilities(DWORD * pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	*pCapabilities = m_pParentFilter->m_dwSeekingCaps;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::CheckCapabilities(DWORD * pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	// make sure all requested capabilities are in our mask

	return (~m_pParentFilter->m_dwSeekingCaps & *pCapabilities) ? S_FALSE : S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::ConvertTimeFormat(LONGLONG * pTarget,
    const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
	CheckPointer(pTarget, E_POINTER);
	// format guids can be null to indicate current format

	// since we only support TIME_FORMAT_MEDIA_TIME, we don't really
	// offer any conversions.
	if (pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME) {
		if(pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME) {
			*pTarget = Source;
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

HRESULT CWavPackSplitterFilterOutputPin::SetPositions(LONGLONG * pCurrent,
	DWORD CurrentFlags, LONGLONG * pStop, DWORD StopFlags)
{
	DWORD StopPosBits = StopFlags & AM_SEEKING_PositioningBitsMask;
	DWORD StartPosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;

	if (StopFlags) {
		CheckPointer(pStop, E_POINTER);
		// accept only relative, incremental, or absolute positioning
		if (StopPosBits != StopFlags) {
			return E_INVALIDARG;
		}
	}

	if(CurrentFlags) {
		CheckPointer(pCurrent, E_POINTER);
		if (StartPosBits != AM_SEEKING_AbsolutePositioning && StartPosBits != AM_SEEKING_RelativePositioning) {
			return E_INVALIDARG;
		}
	}

	// scope for autolock
	{
		CAutoLock lock(m_pParentFilter->m_pLock);

		// set start position
		if (StartPosBits == AM_SEEKING_AbsolutePositioning) {
			m_pParentFilter->m_rtStart = *pCurrent;
		} else if (StartPosBits == AM_SEEKING_RelativePositioning) {
			m_pParentFilter->m_rtStart += *pCurrent;
		}

		// set stop position
		if (StopPosBits == AM_SEEKING_AbsolutePositioning) {
			m_pParentFilter->m_rtStop = *pStop;
		} else if (StopPosBits == AM_SEEKING_IncrementalPositioning) {
			m_pParentFilter->m_rtStop = m_pParentFilter->m_rtStart + *pStop;
		} else if (StopPosBits == AM_SEEKING_RelativePositioning) {
			m_pParentFilter->m_rtStop = m_pParentFilter->m_rtStop + *pStop;
		}
	}

	HRESULT hr = S_OK;
	if (StartPosBits) {
		m_pParentFilter->BeginFlush();
		m_pParentFilter->DoSeeking();
		m_pParentFilter->EndFlush();
	} else if (StopPosBits) {
		// Output a new segment
		m_pParentFilter->BeginFlush();
		m_pParentFilter->EndFlush();
	}

	return hr;
}

HRESULT CWavPackSplitterFilterOutputPin::GetPositions(LONGLONG * pCurrent, LONGLONG * pStop)
{
	if (pCurrent) {
		*pCurrent = m_pParentFilter->m_rtStart;
	}
	if (pStop) {
		*pStop = m_pParentFilter->m_rtStop;
	}

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest)
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

HRESULT CWavPackSplitterFilterOutputPin::SetRate(double dRate)
{
	CAutoLock lock(m_pParentFilter->m_pLock);

	m_pParentFilter->m_dRateSeeking = dRate;

	// Output a new segment
	m_pParentFilter->BeginFlush();
	m_pParentFilter->EndFlush();

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::GetRate(double * pdRate)
{
	CheckPointer(pdRate, E_POINTER);
	CAutoLock lock(m_pParentFilter->m_pLock);

	*pdRate = m_pParentFilter->m_dRateSeeking;

	return S_OK;
}

HRESULT CWavPackSplitterFilterOutputPin::GetPreroll(LONGLONG *pPreroll)
{
	CheckPointer(pPreroll, E_POINTER);
	*pPreroll = 0;

	return S_OK;
}

// ============================================================================
// IAsyncCallBackWrapper_wv
// ============================================================================

int32_t IAsyncCallBackWrapper_wv_read_bytes(void *id, void *data, int32_t bcount)
{
	IAsyncCallBackWrapper_wv* iacbw = (IAsyncCallBackWrapper_wv*)id;
	HRESULT hr = iacbw->pReader->SyncRead(iacbw->StreamPos, bcount, (BYTE*)data);

	if (hr == S_OK) {
		iacbw->StreamPos += bcount;
		return bcount;
	} else {
		return -1;
	}
}

uint32_t IAsyncCallBackWrapper_wv_get_pos(void *id)
{
	IAsyncCallBackWrapper_wv* iacbw = (IAsyncCallBackWrapper_wv*)id;
	return (uint32_t)iacbw->StreamPos;
}

int IAsyncCallBackWrapper_wv_set_pos_abs(void *id, uint32_t pos)
{
	IAsyncCallBackWrapper_wv* iacbw = (IAsyncCallBackWrapper_wv*)id;
	iacbw->StreamPos = min(pos, iacbw->StreamLen);
	if (pos > iacbw->StreamLen) {
		return -1;
	} else {
		return 0;
	}
}

int IAsyncCallBackWrapper_wv_set_pos_rel(void *id, int32_t delta, int mode)
{
	IAsyncCallBackWrapper_wv* iacbw = (IAsyncCallBackWrapper_wv*)id;
	LONGLONG newPos = 0;
	switch (mode) {
		case SEEK_SET:
			newPos = delta;
			break;
		case SEEK_CUR:
			newPos = iacbw->StreamPos + delta;
			break;
		case SEEK_END:
			newPos = iacbw->StreamLen + delta;
			break;
	}
	iacbw->StreamPos = constrain(0, newPos, iacbw->StreamLen);

	if ((newPos < 0) || (newPos > iacbw->StreamLen)) {
		return -1;
	} else {
		return 0;
	}
}

int IAsyncCallBackWrapper_wv_push_back_byte(void *id, int c)
{
	IAsyncCallBackWrapper_wv* iacbw = (IAsyncCallBackWrapper_wv*)id;
	iacbw->StreamPos = constrain(0, iacbw->StreamPos - 1, iacbw->StreamLen);

	return c;
}

uint32_t IAsyncCallBackWrapper_wv_get_length(void *id)
{
	IAsyncCallBackWrapper_wv* iacbw = (IAsyncCallBackWrapper_wv*)id;
	return (uint32_t)iacbw->StreamLen;
}

int IAsyncCallBackWrapper_wv_can_seek(void *id)
{
	return 1;
}

IAsyncCallBackWrapper_wv* IAsyncCallBackWrapper_wv_new(IAsyncReader *pReader)
{
	IAsyncCallBackWrapper_wv* iacbw = DNew IAsyncCallBackWrapper_wv;
	if (!iacbw) {
		return NULL;
	}

	iacbw->iocallback.read_bytes = IAsyncCallBackWrapper_wv_read_bytes;
	iacbw->iocallback.get_pos = IAsyncCallBackWrapper_wv_get_pos;
	iacbw->iocallback.set_pos_abs = IAsyncCallBackWrapper_wv_set_pos_abs;
	iacbw->iocallback.set_pos_rel = IAsyncCallBackWrapper_wv_set_pos_rel;
	iacbw->iocallback.push_back_byte = IAsyncCallBackWrapper_wv_push_back_byte;
	iacbw->iocallback.get_length = IAsyncCallBackWrapper_wv_get_length;
	iacbw->iocallback.can_seek = IAsyncCallBackWrapper_wv_can_seek;
	iacbw->pReader = pReader;
	iacbw->StreamPos = 0;

	// Get total size
	LONGLONG Available;
	pReader->Length(&iacbw->StreamLen, &Available);

	return iacbw;
}

void IAsyncCallBackWrapper_wv_free(IAsyncCallBackWrapper_wv* iacbw)
{
	delete iacbw;
}
