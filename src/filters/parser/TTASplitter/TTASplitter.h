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

#pragma once

#include <mmreg.h>
#include "tta_file.h"

#define TTASplitterName	L"MPC TTA Splitter"

// {F60BAD6D-2101-491a-9265-9F7CB67C6BBB}
static const GUID CLSID_TTASplitter = 
	{ 0xf60bad6d, 0x2101, 0x491a, { 0x92, 0x65, 0x9f, 0x7c, 0xb6, 0x7c, 0x6b, 0xbb } };

typedef struct {
	TTA_io_callback iocallback;
	IAsyncReader *pReader;
	LONGLONG StreamPos;
	LONGLONG StreamLen;
} IAsyncCallBackWrapper_tta;

IAsyncCallBackWrapper_tta* IAsyncCallBackWrapper_tta_new(IAsyncReader *pReader);

void IAsyncCallBackWrapper_tta_free(IAsyncCallBackWrapper_tta **iacw);

class CTTASplitter;

class CTTASplitterInputPin : public CBaseInputPin,
	public CAMThread                             
{
	friend class CTTASplitter;

public:
	CTTASplitterInputPin(CTTASplitter *pParentFilter, CCritSec *pLock, HRESULT *phr);
	virtual ~CTTASplitterInputPin();

	HRESULT CheckMediaType(const CMediaType *pmt);
	CMediaType& CurrentMediaType() { return m_mt; };

	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect(void);
	HRESULT CompleteConnect(IPin *pReceivePin); 

	HRESULT Active();
	HRESULT Inactive();

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();

	HRESULT DoSeeking(REFERENCE_TIME rtStart);

protected:
	enum {CMD_PAUSE, CMD_RUN, CMD_STOP, CMD_EXIT};

	DWORD ThreadProc();
	HRESULT DoProcessingLoop();

	CTTASplitter *m_pParentFilter;
	IAsyncReader *m_pReader;
	IAsyncCallBackWrapper_tta *m_pIACBW;
	TTA_parser *m_pTTAParser;

	BOOL m_bAbort;
	BOOL m_bDiscontinuity;
};

class CTTASplitterOutputPin : public CBaseOutputPin,
	public IMediaSeeking
{
	friend class CTTASplitter;

public:
	CTTASplitterOutputPin(CTTASplitter *pParentFilter, CCritSec *pLock, HRESULT *phr);

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {

		if (riid == IID_IMediaSeeking) {
			return GetInterface((IMediaSeeking*)this, ppv);
		}

		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
	}

	HRESULT CheckMediaType(const CMediaType *pmt);
	CMediaType& CurrentMediaType() { return m_mt; }
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);

	STDMETHODIMP IsFormatSupported(const GUID *pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP SetTimeFormat(const GUID *pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities);
	STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat);   
	STDMETHODIMP SetPositions( LONGLONG *pCurrent, DWORD CurrentFlags, LONGLONG *pStop, DWORD StopFlags);
	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double *pdRate);
	STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

protected:
	CTTASplitter *m_pParentFilter;    
};

class __declspec(uuid("F60BAD6D-2101-491a-9265-9F7CB67C6BBB"))
	CTTASplitter :
	public CBaseFilter
{
public :
	DECLARE_IUNKNOWN
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr); 

	CTTASplitter(LPUNKNOWN lpunk, HRESULT *phr);
	virtual ~CTTASplitter();

	int GetPinCount();
	CBasePin *GetPin(int n);
	STDMETHODIMP Stop(void);
	STDMETHODIMP Pause(void);
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	HRESULT BeginFlush();
	HRESULT EndFlush();

protected:
	CCritSec m_Lock;

	friend class CTTASplitterInputPin;
	friend class CTTASplitterOutputPin;
	CTTASplitterInputPin *m_pInputPin;
	CTTASplitterOutputPin *m_pOutputPin;

	REFERENCE_TIME m_rtStart, m_rtDuration, m_rtStop;
	DWORD m_dwSeekingCaps;
	double m_dRateSeeking;

	void SetDuration(REFERENCE_TIME rtDuration);
	HRESULT DoSeeking();

	TTA_header *GetTTAHeader() {
		return m_pInputPin->m_pTTAParser ? &m_pInputPin->m_pTTAParser->TTAHeader : NULL;
	}

	TTA_parser *GetTTAParser() {
		return m_pInputPin->m_pTTAParser ? m_pInputPin->m_pTTAParser : NULL;
	}

	unsigned long GetMaxFrameLenBytes() { return m_pInputPin->m_pTTAParser->MaxFrameLenBytes; }
};
