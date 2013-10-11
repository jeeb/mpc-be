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

#pragma once

#include "../../parser/BaseSplitter/BaseSplitter.h"

#include <wavpacklib/wavpack/wputils.h>
#include <wavpacklib/wavpack_common.h>
#include <wavpacklib/wavpack_frame.h>
#include <wavpacklib/wavpack_parser.h>

#define WavPackSplitterName   L"MPC WavPack Splitter"

// B5554304-3C9A-40A1-8E82-8C8CFBED56C0
static const GUID CLSID_WavPackSplitter =
	{ 0xd8cf6a42, 0x3e09, 0x4922, { 0xa4, 0x52, 0x21, 0xdf, 0xf1, 0xb, 0xee, 0xba } };

typedef struct {
	short version;
} wavpack_codec_private_data;

typedef struct {
	stream_reader iocallback;
	IAsyncReader *pReader;
	LONGLONG StreamPos;
	LONGLONG StreamLen;
} IAsyncCallBackWrapper_wv;

IAsyncCallBackWrapper_wv* IAsyncCallBackWrapper_wv_new(IAsyncReader *pReader);

void IAsyncCallBackWrapper_wv_free(IAsyncCallBackWrapper_wv* iacw);

class CWavPackSplitterFilter;

class CWavPackSplitterFilterInputPin : public CBaseInputPin,
									   public CAMThread
{
	friend class CWavPackSplitterFilter;

public:
	CWavPackSplitterFilterInputPin(CWavPackSplitterFilter *pParentFilter, CCritSec *pLock, HRESULT * phr);
	virtual ~CWavPackSplitterFilterInputPin();

	HRESULT CheckMediaType(const CMediaType *pmt);
	CMediaType& CurrentMediaType() { return m_mt; };

	HRESULT CheckConnect(IPin* pPin);
	HRESULT BreakConnect(void);
	HRESULT CompleteConnect(IPin *pReceivePin);

	HRESULT Active();
	HRESULT Inactive();

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();

	HRESULT DoSeeking(REFERENCE_TIME rtStart);

protected:
	enum {CMD_EXIT, CMD_STOP, CMD_PAUSE, CMD_RUN};

	DWORD ThreadProc();
	HRESULT DoProcessingLoop();
	HRESULT DeliverOneFrame(WavPack_parser* wpp);

	CWavPackSplitterFilter *m_pParentFilter;
	IAsyncReader *m_pReader;
	IAsyncCallBackWrapper_wv *m_pIACBW;
	WavPack_parser *m_pWavPackParser;

	BOOL m_bAbort;
	BOOL m_bDiscontinuity;
};

class CWavPackSplitterFilterOutputPin : public CBaseOutputPin,
										public IMediaSeeking
{
	friend class CWavPackSplitterFilter;

public:
	CWavPackSplitterFilterOutputPin(CWavPackSplitterFilter *pParentFilter, CCritSec *pLock, HRESULT * phr);

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {
		if (riid == IID_IMediaSeeking) {
			return GetInterface((IMediaSeeking *)this, ppv);
		}
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
	}

	HRESULT CheckMediaType(const CMediaType *pmt);
	CMediaType& CurrentMediaType() { return m_mt; }
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES *pProp);

	// --- IMediaSeeking ---
	STDMETHODIMP IsFormatSupported(const GUID * pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP SetTimeFormat(const GUID * pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	STDMETHODIMP GetCapabilities(DWORD * pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD * pCapabilities);
	STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
	LONGLONG Source, const GUID * pSourceFormat);
	STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags,
	LONGLONG * pStop, DWORD StopFlags);
	STDMETHODIMP GetPositions(LONGLONG * pCurrent, LONGLONG * pStop);
	STDMETHODIMP GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double * pdRate);
	STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

protected:
	CWavPackSplitterFilter *m_pParentFilter;
};

class __declspec(uuid("B5554304-3C9A-40A1-8E82-8C8CFBED56C0"))
	CWavPackSplitterFilter
	: public CBaseFilter
	, public IDSMResourceBagImpl
	, public IDSMChapterBagImpl
	, public IDSMPropertyBagImpl
	, public IAMMediaContent
{
public :
	DECLARE_IUNKNOWN
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	CWavPackSplitterFilter(LPUNKNOWN lpunk, HRESULT *phr);
	virtual ~CWavPackSplitterFilter();

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter
	int GetPinCount();
	CBasePin *GetPin(int n);
	STDMETHODIMP Stop(void);
	STDMETHODIMP Pause(void);
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);

	HRESULT BeginFlush();
	HRESULT EndFlush();

	// IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {return E_NOTIMPL;}

	// IAMMediaContent
	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating) {return E_NOTIMPL;}
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) {return E_NOTIMPL;}

protected:
	CCritSec m_Lock;

	friend class CWavPackSplitterFilterInputPin;
	friend class CWavPackSplitterFilterOutputPin;

	CWavPackSplitterFilterInputPin* m_pInputPin;
	CWavPackSplitterFilterOutputPin* m_pOutputPin;

	REFERENCE_TIME m_rtStart, m_rtDuration, m_rtStop;
	DWORD m_dwSeekingCaps;
	double m_dRateSeeking;

	void SetDuration(REFERENCE_TIME rtDuration);

	HRESULT DoSeeking();

	WavPack_parser *GetWavPackParser() {
		return m_pInputPin->m_pWavPackParser;
	}
};
