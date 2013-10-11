/*
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include "DTSSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CDTSSplitterFilter), DTSSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDTSSplitterFilter>, NULL, &sudFilter[0]},
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

//
// CDTSSplitterFilter
//

CDTSSplitterFilter::CDTSSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CDTSSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

STDMETHODIMP CDTSSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CDTSSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, DTSSplitterName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CDTSSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();

	m_pFile.Attach(DNew CDTSSplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	CAtlArray<CMediaType> mts;
	mts.Add(m_pFile->GetMediaType());

	CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
	AddOutputPin(0, pPinOut);

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->GetDuration();

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CDTSSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	*pDuration = m_pFile->GetDuration();

	return S_OK;
}

bool CDTSSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CDTSSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	return true;
}

void CDTSSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	__int64 startpos = m_pFile->GetStartPos();
	__int64 endpos = m_pFile->GetEndPos();

	if (rt <= 0 || m_pFile->GetDuration() <= 0) {
		m_pFile->Seek(startpos);
		m_rtStart = 0;
	} else {
		m_pFile->Seek(startpos + (__int64)((1.0 * rt / m_pFile->GetDuration()) * (endpos - startpos)));
		m_rtStart = rt;
	}

}

bool CDTSSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	int FrameSize				= 0;
	REFERENCE_TIME rtDuration	= 0;
	m_pFile->GetInfo(FrameSize, rtDuration);

	if (!FrameSize || !rtDuration) {
		return true;
	}

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining()) {
		CAutoPtr<Packet> p(DNew Packet());
		p->SetCount(FrameSize);
		m_pFile->ByteRead(p->GetData(), FrameSize);

		p->TrackNumber = 0;
		p->rtStart = m_rtStart;
		p->rtStop = m_rtStart + rtDuration;
		p->bSyncPoint = TRUE;

		hr = DeliverPacket(p);

		m_rtStart += rtDuration;
	}

	return true;
}
