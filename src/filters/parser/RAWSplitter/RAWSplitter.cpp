/*
 * $Id:
 *
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
#include "RAWSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CRAWSplitterFilter), RAWSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRAWSourceFilter), RAWSourceName, MERIT_NORMAL+1, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CRAWSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CRAWSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_MPEG1Video,
		_T("0,4,,000001B3"),
		_T(".mpeg"), _T(".mpg"), _T(".m2v"), _T(".mpv"),
		NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG1Video);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CRAWSplitterFilter
//

CRAWSplitterFilter::CRAWSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CRAWSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_RAWType(RAW_NONE)
	, m_rtStart(0)
	, m_AvgTimePerFrame(0)
{
}

STDMETHODIMP CRAWSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CRAWSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, RAWSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CRAWSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	CMediaType mt;
	{
		CBaseSplitterFileEx::seqhdr h;
		if (m_pFile->Read(h, min(MEGABYTE, m_pFile->GetLength()), &mt)) {
			m_RAWType			= RAW_MPEG2;
			if (mt.subtype == MEDIASUBTYPE_MPEG1Payload) {
				m_RAWType		= RAW_MPEG1;
			}
			m_AvgTimePerFrame	= h.ifps;

			REFERENCE_TIME rtStart	= INVALID_TIME;
			REFERENCE_TIME rtStop	= INVALID_TIME;
			// find start PTS
			{
				BYTE id = 0x00;
				m_pFile->Seek(0);
				while (m_pFile->GetPos() < min(MEGABYTE, m_pFile->GetLength()) && rtStart == INVALID_TIME) {
					if (!m_pFile->NextMpegStartCode(id)) {
						continue;
					}

					if (id == 0xb8) {	// GOP
						m_pFile->BitRead(1);
						BYTE hour		= m_pFile->BitRead(5);
						BYTE minute		= m_pFile->BitRead(6);
						m_pFile->BitRead(1);
						BYTE second		= m_pFile->BitRead(6);
						BYTE frame		= m_pFile->BitRead(6);
						BYTE closedGOP	= m_pFile->BitRead(1);
						BYTE brokenGOP	= m_pFile->BitRead(1);
						if (closedGOP || brokenGOP || !frame) {
							continue;
						}

						if (rtStart == INVALID_TIME) {
							rtStart = (((hour * 60) + minute) * 60 + second) * UNITS;
						}
					}
				}
			}

			// find end PTS
			{
				BYTE id = 0x00;
				m_pFile->Seek(m_pFile->GetLength() - min(MEGABYTE, m_pFile->GetLength()));
				while (m_pFile->GetPos() < m_pFile->GetLength()) {
					if (!m_pFile->NextMpegStartCode(id)) {
						continue;
					}

					if (id == 0xb8) {	// GOP
						m_pFile->BitRead(1);
						BYTE hour		= m_pFile->BitRead(5);
						BYTE minute		= m_pFile->BitRead(6);
						m_pFile->BitRead(1);
						BYTE second		= m_pFile->BitRead(6);
						BYTE frame		= m_pFile->BitRead(6);
						BYTE closedGOP	= m_pFile->BitRead(1);
						BYTE brokenGOP	= m_pFile->BitRead(1);
						if (closedGOP || brokenGOP || !frame) {
							continue;
						}

						rtStop = (((hour * 60) + minute) * 60 + second) * UNITS;
					}
				}
			}

			if (rtStart != INVALID_TIME && rtStop != INVALID_TIME && rtStop > rtStart) {
				m_rtNewStop = m_rtStop = m_rtDuration = rtStop - rtStart;
			}
		}
	}

	if (mt.subtype != GUID_NULL) {
		CAtlArray<CMediaType> mts;
		mts.Add(mt);
		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"MPEG Video Output", this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CRAWSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CRAWSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	m_pFile->Seek(0);

	return true;
}

void CRAWSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (rt <= 0 || m_rtDuration <= 0) {
		m_pFile->Seek(0);
		m_rtStart = 0;
	} else {
		m_pFile->Seek((__int64)((double)rt / m_rtDuration * m_pFile->GetLength()));
		m_rtStart = rt;
	}
}

bool CRAWSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;
	
	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining()) {
		CAutoPtr<Packet> p(DNew Packet());

		size_t size		= min(1024, m_pFile->GetRemaining());

		p->SetCount(size);
		m_pFile->ByteRead(p->GetData(), size);

		p->TrackNumber	= 0;
		p->rtStart		= INVALID_TIME;
		p->rtStop		= INVALID_TIME;
		p->bSyncPoint	= FALSE;

		hr = DeliverPacket(p);
	}

	return true;
}

//
// CRAWSourceFilter
//

CRAWSourceFilter::CRAWSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRAWSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
