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
#include <windows.h>
#include <initguid.h>
#include <moreuuids.h>
#include <mmreg.h>

#include "AMRSplitter.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_AMR},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CAMRSplitter), AMRSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CAMRSplitter>, NULL, &sudFilter[0]}
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

//-----------------------------------------------------------------------------
//
//	CAMRSplitter class
//
//-----------------------------------------------------------------------------

CUnknown *CAMRSplitter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	return DNew CAMRSplitter(pUnk, phr);
}

CAMRSplitter::CAMRSplitter(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseFilter(NAME("CAMRSplitter"), pUnk, &lock_filter, CLSID_AMRSplitter, phr),
	CAMThread(),
	reader(NULL),
	file(NULL),
	rtCurrent(0),
	rtStop(0xFFFFFFFFFFFFFF),
	rate(1.0),
	ev_abort(TRUE)
{
	input = DNew CAMRInputPin(NAME("AMR Input Pin"), this, phr, L"In");
	output.RemoveAll();
	retired.RemoveAll();

	ev_abort.Reset();
}

CAMRSplitter::~CAMRSplitter()
{
	// just to be sure
	if (reader) {
		delete reader;
		reader = NULL;
	}
	for (int i=0; i<output.GetCount(); i++) {
		CAMROutputPin *pin = output[i];
		if (pin) {
			delete pin;
		}
	}
	output.RemoveAll();
	for (int i=0; i<retired.GetCount(); i++) {
		CAMROutputPin *pin = retired[i];
		if (pin) {
			delete pin;
		}
	}
	retired.RemoveAll();
	if (input) {
		delete input; input = NULL;
	}
}

STDMETHODIMP CAMRSplitter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv,E_POINTER);

	if (riid == IID_IMediaSeeking) {
		return GetInterface((IMediaSeeking*)this, ppv);
	}

	return CBaseFilter::NonDelegatingQueryInterface(riid,ppv);
}

int CAMRSplitter::GetPinCount()
{
	// return pin count
	CAutoLock Lock(&lock_filter);
	return ((input ? 1 : 0) + output.GetCount());
}

CBasePin *CAMRSplitter::GetPin(int n)
{
	CAutoLock Lock(&lock_filter);
	if (n == 0) {
		return input;
	}
	n -= 1;
	int l = output.GetCount();

	// return the requested output pin
	if (n >= l) {
		return NULL;
	}
	return output[n];
}

HRESULT CAMRSplitter::CheckConnect(PIN_DIRECTION Dir, IPin *pPin)
{
	return S_OK;
}

HRESULT CAMRSplitter::CheckInputType(const CMediaType* mtIn)
{
	if (mtIn->majortype == MEDIATYPE_Stream) {
		if (mtIn->subtype == MEDIASUBTYPE_None ||
			mtIn->subtype == MEDIASUBTYPE_NULL) {
			return S_OK;
		}
	} else if (mtIn->majortype == MEDIASUBTYPE_NULL) {
		return S_OK;
	}

	// sorry.. nothing else
	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CAMRSplitter::CompleteConnect(PIN_DIRECTION Dir, CBasePin *pCaller, IPin *pReceivePin)
{
	if (Dir == PINDIR_INPUT) {
		// when our input pin gets connected we have to scan
		// the input file if it is really musepack.
		ASSERT(input && input->Reader());
		ASSERT(!reader);
		ASSERT(!file);

		reader	= DNew CAMRReader(input->Reader());
		file	= DNew CAMRFile();

		// try to open the file
		int ret	= file->Open(reader);
		if (ret < 0) {
			delete file;
			file = NULL;
			delete reader;
			reader = NULL;

			return E_FAIL;
		}

		HRESULT hr = NOERROR;
		
		CAMROutputPin *opin = DNew CAMROutputPin(_T("Outpin"), this, &hr, L"Out", 5);
		ConfigureMediaType(opin);
		AddOutputPin(opin);
	}
	return NOERROR;
}

HRESULT CAMRSplitter::RemoveOutputPins()
{
	CAutoLock Lck(&lock_filter);
	if (m_State != State_Stopped) {
		return VFW_E_NOT_STOPPED;
	}

	// we retire all current output pins
	for (int i=0; i<output.GetCount(); i++) {
		CAMROutputPin *pin = output[i];
		if (pin->IsConnected()) {
			pin->GetConnected()->Disconnect();
			pin->Disconnect();
		}
		retired.Add(pin);
	}
	output.RemoveAll();
	return NOERROR;
}

HRESULT CAMRSplitter::ConfigureMediaType(CAMROutputPin *pin)
{
	CMediaType mt;
	mt.majortype	= MEDIATYPE_Audio;
	mt.subtype		= MEDIASUBTYPE_AMR;
	mt.formattype	= FORMAT_WaveFormatEx;
	mt.lSampleSize	= 1*1024;				// should be way enough

	ASSERT(file);

	// let us fill the waveformatex structure
	WAVEFORMATEX *wfx		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfx, 0, sizeof(*wfx));
	wfx->wBitsPerSample		= 0;
	wfx->nChannels			= 1;
	wfx->nSamplesPerSec		= 8000;
	wfx->nBlockAlign		= 1;
	wfx->nAvgBytesPerSec	= 0;
	wfx->wFormatTag			= 0;

	// the one and only type
	pin->mt_types.Add(mt);
	return NOERROR;
}

HRESULT CAMRSplitter::BreakConnect(PIN_DIRECTION Dir, CBasePin *pCaller)
{
	ASSERT(m_State == State_Stopped);

	if (Dir == PINDIR_INPUT) {
		// let's disconnect the output pins
		ev_abort.Set();
		//ev_ready.Set();

		HRESULT hr = RemoveOutputPins();
		if (FAILED(hr)) {
			return hr;
		}

		// destroy input file, reader and update property page
		if (file) {
			delete file;
			file = NULL;
		}
		if (reader) {
			delete reader;
			reader = NULL;
		}

		ev_abort.Reset();
	}
	return NOERROR;
}

// Output pins
HRESULT CAMRSplitter::AddOutputPin(CAMROutputPin *pPin)
{
	CAutoLock lck(&lock_filter);
	output.Add(pPin);
	return NOERROR;
}

// IMediaSeeking

STDMETHODIMP CAMRSplitter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities =	
			AM_SEEKING_CanGetStopPos|AM_SEEKING_CanGetDuration|AM_SEEKING_CanSeekAbsolute|AM_SEEKING_CanSeekForwards|AM_SEEKING_CanSeekBackwards, 
			S_OK : E_POINTER;
}

STDMETHODIMP CAMRSplitter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if (*pCapabilities == 0) {
		return S_OK;
	}

	DWORD caps;
	GetCapabilities(&caps);
	if ((caps&*pCapabilities) == 0) {
		return E_FAIL;
	}
	
	return caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CAMRSplitter::IsFormatSupported(const GUID* pFormat)	{return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CAMRSplitter::QueryPreferredFormat(GUID* pFormat)		{return GetTimeFormat(pFormat);}
STDMETHODIMP CAMRSplitter::GetTimeFormat(GUID* pFormat)				{return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CAMRSplitter::IsUsingTimeFormat(const GUID* pFormat)	{return IsFormatSupported(pFormat);}
STDMETHODIMP CAMRSplitter::SetTimeFormat(const GUID* pFormat)		{return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}

STDMETHODIMP CAMRSplitter::GetStopPosition(LONGLONG* pStop)
{
	if (pStop) {
		*pStop = this->rtStop;
	}
	return NOERROR; 
}

STDMETHODIMP CAMRSplitter::GetCurrentPosition(LONGLONG* pCurrent)	{return E_NOTIMPL;}
STDMETHODIMP CAMRSplitter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}

STDMETHODIMP CAMRSplitter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return SetPositionsInternal(0, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CAMRSplitter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) {
		*pCurrent = rtCurrent;
	}
	if(pStop) {
		*pStop = rtStop;
	}
	return S_OK;
}

STDMETHODIMP CAMRSplitter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest) {
		*pEarliest = 0;
	}
	return GetDuration(pLatest);
}

STDMETHODIMP CAMRSplitter::SetRate(double dRate)			{return dRate > 0 ? rate = dRate, S_OK : E_INVALIDARG;}
STDMETHODIMP CAMRSplitter::GetRate(double* pdRate)			{return pdRate ? *pdRate = rate, S_OK : E_POINTER;}
STDMETHODIMP CAMRSplitter::GetPreroll(LONGLONG* pllPreroll)	{return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}

STDMETHODIMP CAMRSplitter::GetDuration(LONGLONG* pDuration)
{	
	CheckPointer(pDuration, E_POINTER); 
	*pDuration = 0;

	if (file && pDuration) {
		*pDuration = file->duration_10mhz;
	}
	return S_OK;
}

STDMETHODIMP CAMRSplitter::SetPositionsInternal(int iD, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	// only our first pin can seek
	if (iD != 0) {
		return NOERROR;
	}

	CAutoLock cAutoLock(&lock_filter);

	if (!pCurrent && !pStop || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning) {
		return S_OK;
	}

	REFERENCE_TIME rtCurrent = this->rtCurrent, rtStop = this->rtStop;

	if (pCurrent) {
		switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask) {
			case AM_SEEKING_NoPositioning: break;
			case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
			case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
			case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
		}
	}

	if (pStop) {
		switch(dwStopFlags&AM_SEEKING_PositioningBitsMask) {
			case AM_SEEKING_NoPositioning: break;
			case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
			case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
			case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
		}
	}

	if (this->rtCurrent == rtCurrent && this->rtStop == rtStop) {
		//return S_OK;
	}

	this->rtCurrent	= rtCurrent;
	this->rtStop	= rtStop;

	// now there are new valid Current and Stop positions
	HRESULT hr = DoNewSeek();
	return hr;
}

STDMETHODIMP CAMRSplitter::Pause()
{
	CAutoLock lck(&lock_filter);

	if (m_State == State_Stopped) {

		ev_abort.Reset();

		// activate pins
		for (int i=0; i<output.GetCount(); i++) {
			output[i]->Active();
		}

		if (input) {
			input->Active();
		}

		// seekneme na danu poziciu
		DoNewSeek();

		// pustime parser thread
		if (!ThreadExists()) {
			Create();
			CallWorker(CMD_RUN);
		}
	}

	m_State = State_Paused;
	return NOERROR;
}

STDMETHODIMP CAMRSplitter::Stop()
{
	CAutoLock	lock(&lock_filter);
	HRESULT		hr = NOERROR;

	if (m_State != State_Stopped) {

		// set abort
		ev_abort.Set();
		if (reader) {
			reader->BeginFlush();
		}

		// deaktivujeme piny
		if (input) {
			input->Inactive();
		}
		for (int i=0; i<output.GetCount(); i++) {
			output[i]->Inactive();
		}

		// zrusime parser thread
		if (ThreadExists()) {
			CallWorker(CMD_EXIT);
			Close();
		}

		if (reader) {
			reader->EndFlush();
		}

		ev_abort.Reset();
	}

	m_State = State_Stopped;
	return hr;
}

HRESULT CAMRSplitter::DoNewSeek()
{
	CAMROutputPin	*pin = output[0];
	HRESULT			hr;

	if (!pin->IsConnected()) {
		return NOERROR;
	}

	// stop first
	ev_abort.Set();
	if (reader) {
		reader->BeginFlush();
	}

	FILTER_STATE state = m_State;

	// abort
	if (state != State_Stopped) {
		if (pin->ThreadExists()) {
			pin->ev_abort.Set();
			hr = pin->DeliverBeginFlush();
			if (FAILED(hr)) {
				ASSERT(FALSE);
			}
			if (ThreadExists()) {
				CallWorker(CMD_STOP);
			}
			pin->CallWorker(CMD_STOP);

			hr = pin->DeliverEndFlush();
			if (FAILED(hr)) {
				ASSERT(FALSE);
			}
			pin->FlushQueue();
		}
	}

	pin->DoNewSegment(rtCurrent, rtStop, rate);
	if (reader) {
		reader->EndFlush();
	}

	// seek the file
	if (file) {
		int64 sample_pos = (rtCurrent * 8000) / 10000000;
		file->Seek(sample_pos);
	}

	ev_abort.Reset();

	if (state != State_Stopped) {
		// spustime aj jeho thread
		pin->FlushQueue();
		pin->ev_abort.Reset();
		if (pin->ThreadExists()) {
			pin->CallWorker(CMD_RUN);
		}
		if (ThreadExists()) {
			CallWorker(CMD_RUN);
		}
	}

	return NOERROR;
}

DWORD CAMRSplitter::ThreadProc()
{
	DWORD cmd, cmd2;
	while (true) {
		cmd = GetRequest();
		switch (cmd) {
			case CMD_EXIT:	Reply(NOERROR); return 0;
			case CMD_STOP:	
				{
					Reply(NOERROR); 
				}
				break;
			case CMD_RUN:
				{
					Reply(NOERROR);
					if (!file) {
						break;
					}

					CAMRPacket	packet;
					int32		ret = 0;
					bool		eos = false;
					HRESULT		hr;
					int64		current_sample = 0;

					if ((output.GetCount() <= 0) || (output[0]->IsConnected() == FALSE)) {
						break;
					}
					int	delivered = 0;

					do {
						// are we supposed to abort ?
						if (ev_abort.Check()) {
							break; 
						}

						ret = file->ReadAudioPacket(&packet, &current_sample);
						if (ret == -2) {
							// end of stream
							if (!ev_abort.Check()) {
								output[0]->DoEndOfStream();
							}
							break;
						} else if (ret < 0) {
							break;
						} else {
							// compute time stamp
							REFERENCE_TIME	tStart = (current_sample * 10000000) / 8000;
							REFERENCE_TIME	tStop  = ((current_sample + 160) * 10000000) / 8000;

							packet.tStart = tStart - rtCurrent;
							packet.tStop  = tStop  - rtCurrent;

							// deliver packet
							hr = output[0]->DeliverPacket(packet);
							if (FAILED(hr)) {
								break;
							}
	
							delivered++;
						}
					} while (!CheckRequest(&cmd2));
				}
				break;
			default:
				Reply(E_UNEXPECTED);
				break;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
//
//	CAMRInputPin class
//
//-----------------------------------------------------------------------------

CAMRInputPin::CAMRInputPin(TCHAR *pObjectName, CAMRSplitter *pDemux, HRESULT *phr, LPCWSTR pName) :
	CBaseInputPin(pObjectName, pDemux, &pDemux->lock_filter, phr, pName),
	reader(NULL)
{
	// assign the splitter filter
	demux = pDemux;
}

CAMRInputPin::~CAMRInputPin()
{
	if (reader) {
		reader->Release();
		reader = NULL;
	}
}

// this is a pull mode pin - these can not happen

STDMETHODIMP CAMRInputPin::EndOfStream()		{ return E_UNEXPECTED; }
STDMETHODIMP CAMRInputPin::BeginFlush()			{ return E_UNEXPECTED; }
STDMETHODIMP CAMRInputPin::EndFlush()			{ return E_UNEXPECTED; }
STDMETHODIMP CAMRInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate)	{ return E_UNEXPECTED; }

// we don't support this kind of transport
STDMETHODIMP CAMRInputPin::Receive(IMediaSample * pSample) { return E_UNEXPECTED; }

// check for input type
HRESULT CAMRInputPin::CheckConnect(IPin *pPin)
{
	HRESULT hr = demux->CheckConnect(PINDIR_INPUT, pPin);
	if (FAILED(hr)) {
		return hr;
	}

	return CBaseInputPin::CheckConnect(pPin);
}

HRESULT CAMRInputPin::BreakConnect()
{
	// Can't disconnect unless stopped
	ASSERT(IsStopped());

	demux->BreakConnect(PINDIR_INPUT, this);

	// release the reader
	if (reader) {
		reader->Release();
		reader = NULL;
	}

	return CBaseInputPin::BreakConnect();
}

HRESULT CAMRInputPin::CompleteConnect(IPin *pReceivePin)
{
	// make sure there is a pin
	ASSERT(pReceivePin);

	if (reader) {
		reader->Release();
		reader = NULL;
	}

	// and make sure it supports IAsyncReader
	HRESULT hr = pReceivePin->QueryInterface(IID_IAsyncReader, (void **)&reader);
	if (FAILED(hr)) {
		return hr;
	}

	// we're done here
	hr = demux->CompleteConnect(PINDIR_INPUT, this, pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	return CBaseInputPin::CompleteConnect(pReceivePin);
}

HRESULT CAMRInputPin::CheckMediaType(const CMediaType* pmt)
{
	// make sure we have a type
	ASSERT(pmt);

	// ask the splitter
	return demux->CheckInputType(pmt);
}

HRESULT CAMRInputPin::SetMediaType(const CMediaType* pmt)
{
	// let the baseclass know
	if (FAILED(CheckMediaType(pmt))) {
		return E_FAIL;
	}

	HRESULT hr = CBasePin::SetMediaType(pmt);
	if (FAILED(hr)) {
		return hr;
	}

	return NOERROR;
}

HRESULT CAMRInputPin::Inactive()
{
	HRESULT hr = CBaseInputPin::Inactive();
	if (hr == VFW_E_NO_ALLOCATOR) {
		hr = NOERROR;
	}
	if (FAILED(hr)) {
		return hr;
	}

	return hr;
}

//-----------------------------------------------------------------------------
//
//	CAMROutputPin class
//
//-----------------------------------------------------------------------------

CAMROutputPin::CAMROutputPin(TCHAR *pObjectName, CAMRSplitter *pDemux, HRESULT *phr, LPCWSTR pName, int iBuffers) :
	CBaseOutputPin(pObjectName, pDemux, &pDemux->lock_filter, phr, pName),
	CAMThread(),
	demux(pDemux),
	buffers(iBuffers),
	active(false),
	rtStart(0),
	rtStop(0xffffffffffff),
	rate(1.0),
	ev_can_read(TRUE),
	ev_can_write(TRUE),
	ev_abort(TRUE)
{
	discontinuity	= true;
	eos_delivered	= false;

	ev_can_read.Reset();
	ev_can_write.Set();
	ev_abort.Reset();

	// up to 5 seconds
	buffer_time_ms = 5000;
}

CAMROutputPin::~CAMROutputPin()
{
}

STDMETHODIMP CAMROutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv, E_POINTER);
	if (riid == IID_IMediaSeeking) {
		return GetInterface((IMediaSeeking*)this, ppv);
	} else {
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
	}
}

HRESULT CAMROutputPin::CheckMediaType(const CMediaType *mtOut)
{
	for (int i=0; i<mt_types.GetCount(); i++) {
		if (mt_types[i] == *mtOut) {
			return NOERROR;
		}
	}

	// reject type if it's not among ours
	return E_FAIL;
}

HRESULT CAMROutputPin::SetMediaType(const CMediaType *pmt)
{
	// just set internal media type
	if (FAILED(CheckMediaType(pmt))) {
		return E_FAIL;
	}
	return CBaseOutputPin::SetMediaType(pmt);
}

HRESULT CAMROutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
	// enumerate the list
	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition >= mt_types.GetCount()) {
		return VFW_S_NO_MORE_ITEMS;
	}

	*pmt = mt_types[iPosition];
	return NOERROR;
}

HRESULT CAMROutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	ASSERT(pAlloc);
	ASSERT(pProp);
	HRESULT hr = NOERROR;

	pProp->cBuffers = max(buffers, 1);
	pProp->cbBuffer = max(m_mt.lSampleSize, 1);

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT(Actual.cBuffers >= pProp->cBuffers);
	return NOERROR;
}

HRESULT CAMROutputPin::BreakConnect()
{
	ASSERT(IsStopped());
	demux->BreakConnect(PINDIR_OUTPUT, this);
	discontinuity = true;
	return CBaseOutputPin::BreakConnect();
}

HRESULT CAMROutputPin::CompleteConnect(IPin *pReceivePin)
{
	// let the parent know
	HRESULT hr = demux->CompleteConnect(PINDIR_OUTPUT, this, pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	discontinuity = true;
	// okey
	return CBaseOutputPin::CompleteConnect(pReceivePin);
}

STDMETHODIMP CAMROutputPin::Notify(IBaseFilter *pSender, Quality q)
{
	return S_FALSE;
}

HRESULT CAMROutputPin::Inactive()
{
	if (!active) {
		return NOERROR;
	}
	active = FALSE;

	// destroy everything
	ev_abort.Set();
	HRESULT hr = CBaseOutputPin::Inactive();
	ASSERT(SUCCEEDED(hr));

	if (ThreadExists()) {
		CallWorker(CMD_EXIT);
		Close();
	}
	FlushQueue();
	ev_abort.Reset();

	return NOERROR;
}

HRESULT CAMROutputPin::Active()
{
	if (active) {
		return NOERROR;
	}
	
	FlushQueue();
	if (!IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	ev_abort.Reset();

	HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr)) {
		active = FALSE;
		return hr;
	}

	// new segment
	DoNewSegment(rtStart, rtStop, rate);

	// vytvorime novu queue
	if (!ThreadExists()) {
		Create();
		CallWorker(CMD_RUN);
	}

	active = TRUE;
	return hr;
}

HRESULT CAMROutputPin::DoNewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
	// queue the EOS packet
	this->rtStart	= rtStart;
	this->rtStop	= rtStop;
	this->rate		= dRate;
	eos_delivered	= false;

	discontinuity = true;
	return DeliverNewSegment(rtStart, rtStop, rate);
}

// IMediaSeeking
STDMETHODIMP CAMROutputPin::GetCapabilities(DWORD* pCapabilities)					{ return demux->GetCapabilities(pCapabilities); }
STDMETHODIMP CAMROutputPin::CheckCapabilities(DWORD* pCapabilities)					{ return demux->CheckCapabilities(pCapabilities); }
STDMETHODIMP CAMROutputPin::IsFormatSupported(const GUID* pFormat)					{ return demux->IsFormatSupported(pFormat); }
STDMETHODIMP CAMROutputPin::QueryPreferredFormat(GUID* pFormat)						{ return demux->QueryPreferredFormat(pFormat); }
STDMETHODIMP CAMROutputPin::GetTimeFormat(GUID* pFormat)							{ return demux->GetTimeFormat(pFormat); }
STDMETHODIMP CAMROutputPin::IsUsingTimeFormat(const GUID* pFormat)					{ return demux->IsUsingTimeFormat(pFormat); }
STDMETHODIMP CAMROutputPin::SetTimeFormat(const GUID* pFormat)						{ return demux->SetTimeFormat(pFormat); }
STDMETHODIMP CAMROutputPin::GetDuration(LONGLONG* pDuration)						{ return demux->GetDuration(pDuration); }
STDMETHODIMP CAMROutputPin::GetStopPosition(LONGLONG* pStop)						{ return demux->GetStopPosition(pStop); }
STDMETHODIMP CAMROutputPin::GetCurrentPosition(LONGLONG* pCurrent)					{ return demux->GetCurrentPosition(pCurrent); }
STDMETHODIMP CAMROutputPin::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)		{ return demux->GetPositions(pCurrent, pStop); }
STDMETHODIMP CAMROutputPin::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)	{ return demux->GetAvailable(pEarliest, pLatest); }
STDMETHODIMP CAMROutputPin::SetRate(double dRate)									{ return demux->SetRate(dRate); }
STDMETHODIMP CAMROutputPin::GetRate(double* pdRate)									{ return demux->GetRate(pdRate); }
STDMETHODIMP CAMROutputPin::GetPreroll(LONGLONG* pllPreroll)						{ return demux->GetPreroll(pllPreroll); }

STDMETHODIMP CAMROutputPin::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return demux->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CAMROutputPin::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return demux->SetPositionsInternal(0, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

int CAMROutputPin::GetDataPacketAMR(DataPacketAMR **packet)
{
	// this method may blokc
	HANDLE	events[] = { ev_can_write, ev_abort };
	DWORD	ret;

	do {
		// if the abort event is set, quit
		if (ev_abort.Check()) {
			return -1;
		}

		ret = WaitForMultipleObjects(2, events, FALSE, 10);
		if (ret == WAIT_OBJECT_0) {
			// return new packet
			*packet = DNew DataPacketAMR();
			return 0;
		}
	} while (1);

	// unexpected
	return -1;
}

HRESULT CAMROutputPin::DeliverPacket(CAMRPacket &packet)
{
	// we don't accept packets while aborting...
	if (ev_abort.Check()) {
		return E_FAIL;
	}

	// ziskame novy packet na vystup
	DataPacketAMR *outp = NULL;
	int ret = GetDataPacketAMR(&outp);
	if (ret < 0 || !outp) {
		return E_FAIL;
	}

	outp->type = DataPacketAMR::PACKET_TYPE_DATA;

	// spocitame casy
	outp->rtStart	= packet.tStart;
	outp->rtStop	= packet.tStop;
	outp->has_time	= true;
	
	outp->size	= packet.packet_size;
	outp->buf	= (BYTE*)malloc(outp->size);
	memcpy(outp->buf, packet.packet, packet.packet_size);

	// each packet is sync point
	outp->sync_point = TRUE;

	// discontinuity ?
	if (discontinuity) {
		outp->discontinuity	= TRUE;
		discontinuity		= false;
	} else {
		outp->discontinuity	= FALSE;
	}

	{
		// insert into queue
		CAutoLock lck(&lock_queue);
		queue.AddTail(outp);
		ev_can_read.Set();

		// we allow buffering for a few seconds (might be usefull for network playback)
		if (GetBufferTime_MS() >= buffer_time_ms) {
			ev_can_write.Reset();
		}
	}

	return NOERROR;
}

HRESULT CAMROutputPin::DoEndOfStream()
{
	DataPacketAMR *packet = DNew DataPacketAMR();

	// naqueueujeme EOS
	{
		CAutoLock lck(&lock_queue);
		packet->type = DataPacketAMR::PACKET_TYPE_EOS;
		queue.AddTail(packet);
		ev_can_read.Set();
	}
	eos_delivered = true;

	return NOERROR;
}

void CAMROutputPin::FlushQueue()
{
	CAutoLock lck(&lock_queue);
	while (queue.GetCount() > 0) {
		DataPacketAMR *packet = queue.RemoveHead();
		if (packet) {
			delete packet;
		}
	}
	ev_can_read.Reset();
	ev_can_write.Set();
}

HRESULT CAMROutputPin::DeliverDataPacketAMR(DataPacketAMR &packet)
{
	IMediaSample *sample;
	HRESULT hr = GetDeliveryBuffer(&sample, NULL, NULL, 0);
	if (FAILED(hr)) {
		return E_FAIL;
	}

	// we should have enough space in there
	long lsize = sample->GetSize();
	ASSERT(lsize >= packet.size);

	BYTE *buf;
	sample->GetPointer(&buf);

	memcpy(buf, packet.buf, packet.size);
	sample->SetActualDataLength(packet.size);

	// sync point, discontinuity ?
	if (packet.sync_point) {
		sample->SetSyncPoint(TRUE);
	} else {
		sample->SetSyncPoint(FALSE);
	}

	if (packet.discontinuity) { 
		sample->SetDiscontinuity(TRUE); 
	} else { 
		sample->SetDiscontinuity(FALSE); 
	}

	// do we have a time stamp ?
	if (packet.has_time) { 
		sample->SetTime(&packet.rtStart, &packet.rtStop); 
	}

	// dorucime
	hr = Deliver(sample);	
	sample->Release();
	return hr;
}

__int64 CAMROutputPin::GetBufferTime_MS()
{
	CAutoLock lck(&lock_queue);
	if (queue.IsEmpty()) {
		return 0;
	}

	DataPacketAMR	*pfirst;
	DataPacketAMR	*plast;
	__int64		tstart, tstop;
	POSITION	posf, posl;

	tstart	= -1;
	tstop	= -1;

	posf = queue.GetHeadPosition();
	while (posf) {
		pfirst = queue.GetNext(posf);
		if (pfirst->type == DataPacketAMR::PACKET_TYPE_DATA && pfirst->rtStart != -1) {
			tstart = pfirst->rtStart;
			break;
		}
	}

	posl = queue.GetTailPosition();
	while (posl) {
		plast = queue.GetPrev(posl);
		if (plast->type == DataPacketAMR::PACKET_TYPE_DATA && plast->rtStart != -1) {
			tstop = plast->rtStart;
			break;
		}
	}

	if (tstart == -1 || tstop == -1) {
		return 0;
	}

	// calculate time in milliseconds
	return __int64((tstop - tstart) / 10000);
}

// vlakno na output
DWORD CAMROutputPin::ThreadProc()
{
	while (true) {
		DWORD cmd = GetRequest();
		switch (cmd) {
			case CMD_EXIT:		Reply(0); return 0; break;
			case CMD_STOP:
				{
					Reply(0); 
				}
				break;
			case CMD_RUN:
				{
					Reply(0);
					if (!IsConnected()) {
						break;
					}

					// deliver packets
					DWORD	cmd2;
					BOOL	is_first = true;
					DataPacketAMR	*packet;
					HRESULT	hr;

					do {
						if (ev_abort.Check()) {
							break;
						}
						hr = NOERROR;
					
						HANDLE	events[] = { ev_can_read, ev_abort};
						DWORD	ret = WaitForMultipleObjects(2, events, FALSE, 10);

						if (ret == WAIT_OBJECT_0) {
							// look for new packet in queue
							{
								CAutoLock	lck(&lock_queue);
								packet = queue.RemoveHead();
								if (queue.IsEmpty()) {
									ev_can_read.Reset();
								}

								// allow buffering
								if (GetBufferTime_MS() < buffer_time_ms) {
									ev_can_write.Set();
								}
							}

							// bud dorucime End Of Stream, alebo packet
							if (packet->type == DataPacketAMR::PACKET_TYPE_EOS) {
								DeliverEndOfStream();
							} else if (packet->type == DataPacketAMR::PACKET_TYPE_NEW_SEGMENT) {
								hr = DeliverNewSegment(packet->rtStart, packet->rtStop, packet->rate);
							} else if (packet->type == DataPacketAMR::PACKET_TYPE_DATA) {
								hr = DeliverDataPacketAMR(*packet);
							}					

							delete packet;

							if (FAILED(hr)) {
								break;
							}
						}
					} while (!CheckRequest(&cmd2));
				}
				break;
			default:
				Reply(E_UNEXPECTED); 
				break;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
//
//	CAMRReader class
//
//-----------------------------------------------------------------------------
CAMRReader::CAMRReader(IAsyncReader *rd)
{
	ASSERT(rd);

	reader = rd;
	reader->AddRef();
	position = 0;
}

CAMRReader::~CAMRReader()
{
	if (reader) {
		reader->Release();
		reader = NULL;
	}
}

int CAMRReader::GetSize(__int64 *avail, __int64 *total)
{
	// vraciame velkost
	HRESULT hr = reader->Length(total, avail);
	if (FAILED(hr)) {
		return -1;
	}
	return 0;
}

int CAMRReader::GetPosition(__int64 *pos, __int64 *avail)
{
	HRESULT hr;
	__int64	total;
	hr = reader->Length(&total, avail);
	if (FAILED(hr)) {
		return -1;
	}

	// aktualna pozicia
	if (pos) {
		*pos = position;
	}
	return 0;
}

int CAMRReader::Seek(__int64 pos)
{
	__int64	avail, total;
	GetSize(&avail, &total);
	if (pos < 0 || pos >= total) {
		return -1;
	}

	// seekneme
	position = pos;
	return 0;
}

int CAMRReader::Read(void *buf, int size)
{
	__int64	avail, total;
	GetSize(&avail, &total);
	if (position + size > avail) {
		return -1;
	}

	// TODO: Caching here !!!!
	//TRACE("    - read, %I64d, %d\n", position, size);

	HRESULT hr = reader->SyncRead(position, size, (BYTE*)buf);
	if (FAILED(hr)) {
		return -1;
	}

	// update position
	position += size;
	return 0;
}

DataPacketAMR::DataPacketAMR() :
	type(PACKET_TYPE_EOS),
	rtStart(0),
	rtStop(0),
	sync_point(FALSE),
	discontinuity(FALSE),
	buf(NULL),
	size(0)
{
}

DataPacketAMR::~DataPacketAMR()
{
	if (buf) {
		free(buf);
		buf = NULL;
	}
}
