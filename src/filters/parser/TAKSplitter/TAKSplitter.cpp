/*
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include <MMReg.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include "TAKSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_TAK_Stream},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_TAK},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CTAKSplitterFilter), TAKSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CTAKSourceFilter), TAKSourceName, MERIT_NORMAL+1, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CTAKSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CTAKSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_TAK_Stream,
		_T("0,4,,7442614B"), // tBaK
		_T(".tak"), NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_TAK_Stream);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CTAKSplitterFilter
//

CTAKSplitterFilter::CTAKSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CTAKSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_nAvgBytesPerSec(0)
	, m_rtStart(0)
{
}

STDMETHODIMP CTAKSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CTAKSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, TAKSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CTAKSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CBaseSplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	if (m_pFile->BitRead(32) != 'tBaK') {
		return E_FAIL;
	}

	if (FAILED(m_TAKFile.Open(m_pFile))) {
		return E_FAIL;
	}

	CMediaType mt;
	if (m_TAKFile.SetMediaType(mt)) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
		m_nAvgBytesPerSec		= wfe->nAvgBytesPerSec;
		m_rtDuration			= m_TAKFile.GetDuration();

		CAtlArray<CMediaType> mts;
		mts.Add(mt);
		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Tak Audio Output", this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));


	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CTAKSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CTAKSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	return true;
}

void CTAKSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_rtStart = m_TAKFile.Seek(rt);
}

bool CTAKSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetPos() < m_TAKFile.GetEndPos()) {

		CAutoPtr<Packet> p(DNew Packet());

		size_t size		= min(1024, m_pFile->GetRemaining());

		p->SetCount(size);
		m_pFile->ByteRead(p->GetData(), size);

		p->TrackNumber	= 0;
		p->rtStart		= m_rtStart;
		p->rtStop		= m_rtStart + m_nAvgBytesPerSec;
		p->bSyncPoint	= TRUE;

		hr = DeliverPacket(p);

		m_rtStart += m_nAvgBytesPerSec;
	}

	return true;
}

//
// CTAKSourceFilter
//

CTAKSourceFilter::CTAKSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CTAKSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
