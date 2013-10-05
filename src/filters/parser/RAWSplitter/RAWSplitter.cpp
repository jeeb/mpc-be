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

#ifndef FCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))
#endif

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
	CAtlList<CString> chkbytes;
	chkbytes.AddTail(_T("0,4,,000001B3"));		// MPEG1/2
	chkbytes.AddTail(_T("0,5,,0000000109"));	// H.264
	chkbytes.AddTail(_T("0,4,,0000010F"));		// VC-1
	chkbytes.AddTail(_T("0,4,,0000010D"));		// VC-1
#if ENABLE_YUV4MPEG2
	chkbytes.AddTail(_T("0,9,,595556344D50454732")); // YUV4MPEG2
#endif

	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_NULL,
		chkbytes,
		_T(".mpeg"), _T(".mpg"), _T(".m2v"), _T(".mpv"), _T(".h264"), _T(".264"), _T(".vc1"),
		NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG1Video);
	UnRegisterSourceFilter(MEDIASUBTYPE_NULL);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

#if ENABLE_YUV4MPEG2
const BYTE FRAME_[6] = {'F','R','A','M','E',0x0A};
#endif

//
// CRAWSplitterFilter
//

CRAWSplitterFilter::CRAWSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CRAWSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_RAWType(RAW_NONE)
	, m_startpos(0)
	, m_framelen(0)
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
#if ENABLE_YUV4MPEG2
		{
			const BYTE YUV4MPEG2_[10] = {'Y','U','V','4','M','P','E','G','2',0x20};
			BYTE buf[256];
			if (m_pFile->ByteRead(buf, 10) == S_OK && memcmp(buf, YUV4MPEG2_, 10) == 0 && m_pFile->ByteRead(buf, 255) == S_OK) {
				buf[255] = 0;
				CStringA params = CStringA(buf);
				params.Truncate(params.Find(0x0A));

				int   width  = 0;
				int   height = 0;
				int   fpsnum = 24;
				int   fpsden = 1;
				int   sar_x  = 1;
				int   sar_y  = 1;
				WORD  bpp    = 0;
				DWORD interl = 0;
				FOURCC fourcc;

				int k;

				CAtlList<CStringA> sl;
				Explode(params, sl, 0x20);
				POSITION pos = sl.GetHeadPosition();
				while (pos) {
					CStringA& str = sl.GetNext(pos);
					if (str.GetLength() < 2) {
						continue;
					}
					switch (str[0]) {
						case 'W':
							width = atoi(str.Mid(1));
							break;
						case 'H':
							height = atoi(str.Mid(1));
							break;
						case 'F':
							k = str.Find(':');
							fpsnum = atoi(str.Mid(1, k-1));
							fpsden = atoi(str.Mid(k+1));
							break;
						case 'I':
							if (str.GetLength() == 2) {
								switch (str[1]) {
									case 'p': interl = 0; break;
									case 't': interl = 1; break;
									case 'b': interl = 2; break;
									case 'm': interl = 3; break;
								}
							}
							break;
						case 'A':
							k = str.Find(':');
							sar_x = atoi(str.Mid(1, k-1));
							sar_y = atoi(str.Mid(k+1));
							break;
						case 'C':
							str = str.Mid(1);
							// 4:2:0
							if (str == "420" || "420jpeg" || "420mpeg2" || "420paldv") {
								fourcc = FCC('YV12'); // AV_PIX_FMT_YUV420P
								bpp    = 12;
							}
							else if (str == "420p10") {
								fourcc = FCC('P010'); // AV_PIX_FMT_YUV420P10
								bpp    = 15;
							}
							else if (str == "420p16") {
								fourcc = FCC('P016'); // AV_PIX_FMT_YUV420P16
								bpp    = 24;
							}
							// 4:2:2
							else if (str == "422") {
								fourcc = FCC('YV16'); // AV_PIX_FMT_YUV422P
								bpp    = 16;
							}
							else if (str == "422p10") {
								fourcc = FCC('P210'); // AV_PIX_FMT_YUV422P10
								bpp    = 20;
							}
							else if (str == "422p16") {
								fourcc = FCC('P216'); // AV_PIX_FMT_YUV422P16
								bpp    = 32;
							}
							// 4:4:4
							else if (str == "444") {
								fourcc = FCC('YV24'); // AV_PIX_FMT_YUV444P
								bpp    = 24;
							}
							break;
						//case 'X':
						//	break;
					}
				}

				int frame_length = width * height * bpp / 8;
				if (frame_length && fpsnum && fpsden && sar_x && sar_y) {
					m_RAWType = RAW_Y4M;
					m_startpos = 10 + params.GetLength() + 1;
					m_framelen = frame_length;

					mt.majortype  = MEDIATYPE_Video;
					mt.subtype    = FOURCCMap(fourcc);
					mt.formattype = FORMAT_VIDEOINFO2;

					VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

					memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

					vih2->bmiHeader.biSize        = sizeof(vih2->bmiHeader);
					vih2->bmiHeader.biWidth       = width;
					vih2->bmiHeader.biHeight      = -height;
					vih2->bmiHeader.biPlanes      = 1;
					vih2->bmiHeader.biBitCount    = bpp;
					vih2->bmiHeader.biCompression = fourcc;
					vih2->bmiHeader.biSizeImage   = frame_length;
					vih2->rcSource = vih2->rcTarget = CRect(0, 0, width, height);
					//vih2->dwBitRate      = frame_length * 8 * fpsnum / fpsden;
					vih2->AvgTimePerFrame  = 10000000i64 * fpsden / fpsnum;
					// always tell DirectShow it's interlaced (progressive flags set in IMediaSample struct)
					vih2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOrWeave;

					sar_x *= width;
					sar_y *= height;
					ReduceDim(sar_x, sar_y);
					vih2->dwPictAspectRatioX = sar_x;
					vih2->dwPictAspectRatioY = sar_y;

					m_AvgTimePerFrame = vih2->AvgTimePerFrame;
					m_rtDuration      = (m_pFile->GetLength() - m_startpos) / (6 + m_framelen) * 10000000i64 * fpsden / fpsnum;
					mt.SetSampleSize(m_framelen);
				}
			}
		}
#endif

		if (m_RAWType == RAW_NONE) {
			m_pFile->Seek(0);
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

		if (m_RAWType == RAW_NONE) {
			m_pFile->Seek(0);

			CBaseSplitterFileEx::avchdr h;
			if (m_pFile->Read(h, min(5 * MEGABYTE, m_pFile->GetLength()), &mt)) {
				m_RAWType = RAW_H264;
			}
		
		}

		if (m_RAWType == RAW_NONE) {
			BYTE id = 0x00;
			m_pFile->Seek(0);
			while (m_pFile->GetPos() < min(5 * MEGABYTE, m_pFile->GetLength()) && m_RAWType == RAW_NONE) {
				if (!m_pFile->NextMpegStartCode(id)) {
					continue;
				}

				if (id == 0x0F) {	// sequence header
					m_pFile->Seek(m_pFile->GetPos() - 4);

					CBaseSplitterFileEx::vc1hdr h;
					if (m_pFile->Read(h, 1024, &mt)) {
						m_RAWType = RAW_VC1;
					}
				}
			}
		}
	}

	if (m_RAWType != RAW_NONE) {
		CString pName;
		switch (m_RAWType) {
			case RAW_MPEG1:
				pName = L"MPEG1 Video Output";
				break;
			case RAW_MPEG2:
				pName = L"MPEG2 Video Output";
				break;
			case RAW_H264:
				pName = L"H.264 Video Output";
				break;
			case RAW_VC1:
				pName = L"VC-1 Video Output";
				break;
		}

		CAtlArray<CMediaType> mts;
		mts.Add(mt);
		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CRAWSplitterOutputPin(mts, pName, this, this, &hr));
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
#if ENABLE_YUV4MPEG2
	if (m_RAWType == RAW_Y4M) {
		if (rt <= 0) {
			m_pFile->Seek(m_startpos);
			m_rtStart = 0;
		} else {
			__int64 framenum = m_rtDuration / m_AvgTimePerFrame;
			m_pFile->Seek(m_startpos + framenum * (6 + m_framelen));
			m_rtStart = framenum * m_AvgTimePerFrame;
		}
		return;
	}
#endif

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

#if ENABLE_YUV4MPEG2
		if (m_RAWType == RAW_Y4M) {
			BYTE buf[6];
			if (m_pFile->ByteRead(buf, 6) != S_OK || memcmp(buf, FRAME_, 6) != 0) {
				ASSERT(0);
			}
			__int64 framenum = (m_pFile->GetPos() - m_startpos) / (6 + m_framelen);

			p->SetCount(m_framelen);
			m_pFile->ByteRead(p->GetData(), m_framelen);
			
			p->TrackNumber = 0;
			p->rtStart     = framenum * m_AvgTimePerFrame;
			p->rtStop      = p->rtStart + m_AvgTimePerFrame;
			p->bSyncPoint  = FALSE;

			hr = DeliverPacket(p);
			continue;
		}
#endif

		size_t size		= min(64 * KILOBYTE, m_pFile->GetRemaining());

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

//
// CMpegSplitterOutputPin
//

CRAWSplitterOutputPin::CRAWSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
	, m_fHasAccessUnitDelimiters(false)
{
}

CRAWSplitterOutputPin::~CRAWSplitterOutputPin()
{
	Flush();
}

HRESULT CRAWSplitterOutputPin::Flush()
{
	CAutoLock cAutoLock(this);

	m_p.Free();
	m_pl.RemoveAll();

	m_bFlushed = true;

	return S_OK;
}

HRESULT CRAWSplitterOutputPin::DeliverEndFlush()
{
	CAutoLock cAutoLock(this);

	Flush();

	return __super::DeliverEndFlush();
}

HRESULT CRAWSplitterOutputPin::DeliverPacket(CAutoPtr<Packet> p)
{
	CAutoLock cAutoLock(this);

	if (p->pmt) {
		if (*((CMediaType *)p->pmt) != m_mt) {
			SetMediaType ((CMediaType*)p->pmt);
			Flush();
		}
	}

	// H.264/AVC/CVMA/CVME
	if (m_mt.subtype == FOURCCMap('1CVA') || m_mt.subtype == FOURCCMap('1cva') || m_mt.subtype == FOURCCMap('CVMA') || m_mt.subtype == FOURCCMap('CVME')) {
		if (!m_p) {
			m_p.Attach(DNew Packet());
			m_p->TrackNumber	= p->TrackNumber;
			m_p->bDiscontinuity	= p->bDiscontinuity;
			p->bDiscontinuity	= FALSE;

			m_p->bSyncPoint	= p->bSyncPoint;
			p->bSyncPoint	= FALSE;

			m_p->rtStart	= p->rtStart;
			p->rtStart		= INVALID_TIME;

			m_p->rtStop	= p->rtStop;
			p->rtStop	= INVALID_TIME;
		}

		m_p->Append(*p);

		BYTE* start	= m_p->GetData();
		BYTE* end	= start + m_p->GetCount();

		MOVE_TO_H264_START_CODE(start, end);

		while (start <= end-4) {
			BYTE* next = start+1;

			MOVE_TO_H264_START_CODE(next, end);

			if (next >= end-4) {
				break;
			}

			int size = next - start;

			CH264Nalu Nalu;
			Nalu.SetBuffer (start, size, 0);

			CAutoPtr<Packet> p2;

			while (Nalu.ReadNext()) {
				DWORD dwNalLength = Nalu.GetDataLength();
				dwNalLength = BSWAP32(dwNalLength);

				CAutoPtr<Packet> p3(DNew Packet());

				p3->SetCount(Nalu.GetDataLength() + sizeof(dwNalLength));
				memcpy(p3->GetData(), &dwNalLength, sizeof(dwNalLength));
				memcpy(p3->GetData() + sizeof(dwNalLength), Nalu.GetDataBuffer(), Nalu.GetDataLength());

				if (p2 == NULL) {
					p2 = p3;
				} else {
					p2->Append(*p3);
				}
			}
			start = next;

			if (!p2) {
				continue;
			}

			p2->TrackNumber		= m_p->TrackNumber;
			p2->bDiscontinuity	= m_p->bDiscontinuity;
			m_p->bDiscontinuity	= FALSE;

			p2->bSyncPoint	= m_p->bSyncPoint;
			m_p->bSyncPoint	= FALSE;

			p2->rtStart		= m_p->rtStart;
			m_p->rtStart	= INVALID_TIME;
			p2->rtStop		= m_p->rtStop;
			m_p->rtStop		= INVALID_TIME;

			p2->pmt		= m_p->pmt;
			m_p->pmt	= NULL;

			m_pl.AddTail(p2);

			if (p->rtStart != INVALID_TIME) {
				m_p->rtStart	= p->rtStart;
				m_p->rtStop		= p->rtStop;
				p->rtStart		= INVALID_TIME;
			}
			if (p->bDiscontinuity) {
				m_p->bDiscontinuity	= p->bDiscontinuity;
				p->bDiscontinuity	= FALSE;
			}
			if (p->bSyncPoint) {
				m_p->bSyncPoint	= p->bSyncPoint;
				p->bSyncPoint	= FALSE;
			}
			if (m_p->pmt) {
				DeleteMediaType(m_p->pmt);
			}

			m_p->pmt = p->pmt;
			p->pmt = NULL;
		}
		if (start > m_p->GetData()) {
			m_p->RemoveAt(0, start - m_p->GetData());
		}

		REFERENCE_TIME rtStart = INVALID_TIME, rtStop = INVALID_TIME;

		for (POSITION pos = m_pl.GetHeadPosition(); pos; m_pl.GetNext(pos)) {
			if (pos == m_pl.GetHeadPosition()) {
				continue;
			}

			Packet* pPacket = m_pl.GetAt(pos);
			BYTE* pData = pPacket->GetData();

			/*
			if ((pData[4] & 0x1f) == 0x09) {
				m_fHasAccessUnitDelimiters = true;
			}
			*/

			if ((pData[4] & 0x1f) == 0x09/* || (!m_fHasAccessUnitDelimiters && pPacket->rtStart != INVALID_TIME)*/) {
				if (pPacket->rtStart == INVALID_TIME && rtStart != INVALID_TIME) {
					pPacket->rtStart	= rtStart;
					pPacket->rtStop		= rtStop;
				}

				p = m_pl.RemoveHead();

				while (pos != m_pl.GetHeadPosition()) {
					CAutoPtr<Packet> p2 = m_pl.RemoveHead();
					p->Append(*p2);
				}

				if (!p->pmt && m_bFlushed) {
					p->pmt = CreateMediaType(&m_mt);
					m_bFlushed = false;
				}

				HRESULT hr = __super::DeliverPacket(p);
				if (hr != S_OK) {
					return hr;
				}
			} else if (rtStart == INVALID_TIME) {
				rtStart	= pPacket->rtStart;
				rtStop	= pPacket->rtStop;
			}
		}

		return S_OK;
	}
	// VC1
	else if (m_mt.subtype == FOURCCMap('1CVW')) {
		if (!m_p) {
			m_p.Attach(DNew Packet());
			m_p->TrackNumber	= p->TrackNumber;
			m_p->bDiscontinuity	= p->bDiscontinuity;
			p->bDiscontinuity	= FALSE;

			m_p->bSyncPoint	= p->bSyncPoint;
			p->bSyncPoint	= FALSE;

			m_p->rtStart	= p->rtStart;
			p->rtStart		= INVALID_TIME;

			m_p->rtStop	= p->rtStop;
			p->rtStop	= INVALID_TIME;
		}

		m_p->Append(*p);

		BYTE* start = m_p->GetData();
		BYTE* end = start + m_p->GetCount();

		bool bSeqFound = false;
		while (start <= end-4) {
			if (*(DWORD*)start == 0x0D010000) {
				bSeqFound = true;
				break;
			} else if (*(DWORD*)start == 0x0F010000) {
				break;
			}
			start++;
		}

		while (start <= end-4) {
			BYTE* next = start+1;

			while (next <= end-4) {
				if (*(DWORD*)next == 0x0D010000) {
					if (bSeqFound) {
						break;
					}
					bSeqFound = true;
				} else if (*(DWORD*)next == 0x0F010000) {
					break;
				}
				next++;
			}

			if (next >= end-4) {
				break;
			}

			int size = next - start - 4;
			UNREFERENCED_PARAMETER(size);

			CAutoPtr<Packet> p2(DNew Packet());
			p2->TrackNumber		= m_p->TrackNumber;
			p2->bDiscontinuity	= m_p->bDiscontinuity;
			m_p->bDiscontinuity	= FALSE;

			p2->bSyncPoint	= m_p->bSyncPoint;
			m_p->bSyncPoint	= FALSE;

			p2->rtStart		= m_p->rtStart;
			m_p->rtStart	= INVALID_TIME;

			p2->rtStop		= m_p->rtStop;
			m_p->rtStop		= INVALID_TIME;

			p2->pmt		= m_p->pmt;
			m_p->pmt	= NULL;

			p2->SetData(start, next - start);

			if (!p2->pmt && m_bFlushed) {
				p2->pmt = CreateMediaType(&m_mt);
				m_bFlushed = false;
			}

			HRESULT hr = __super::DeliverPacket(p2);
			if (hr != S_OK) {
				return hr;
			}

			if (p->rtStart != INVALID_TIME) {
				m_p->rtStart = p->rtStart;
				m_p->rtStop	= p->rtStop;
				p->rtStart	= INVALID_TIME;
			}
			if (p->bDiscontinuity) {
				m_p->bDiscontinuity	= p->bDiscontinuity;
				p->bDiscontinuity	= FALSE;
			}
			if (p->bSyncPoint) {
				m_p->bSyncPoint	= p->bSyncPoint;
				p->bSyncPoint	= FALSE;
			}
			if (m_p->pmt) {
				DeleteMediaType(m_p->pmt);
			}

			m_p->pmt	= p->pmt;
			p->pmt		= NULL;

			start		= next;
			bSeqFound	= (*(DWORD*)start == 0x0D010000);
		}

		if (start > m_p->GetData()) {
			m_p->RemoveAt(0, start - m_p->GetData());
		}

		return S_OK;
	} else {
		m_p.Free();
		m_pl.RemoveAll();
	}

	return __super::DeliverPacket(p);
}


