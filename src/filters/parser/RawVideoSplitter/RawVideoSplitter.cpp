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
#include "RawVideoSplitter.h"
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
	{&__uuidof(CRawVideoSplitterFilter), RawVideoSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRawVideoSourceFilter), RawVideoSourceName, MERIT_NORMAL+1, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CRawVideoSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CRawVideoSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	CAtlList<CString> chkbytes;
	chkbytes.AddTail(_T("0,4,,000001B3"));		// MPEG1/2
	chkbytes.AddTail(_T("0,5,,0000000109"));	// H.264/AVC1
	chkbytes.AddTail(_T("0,4,,0000010F"));		// VC-1
	chkbytes.AddTail(_T("0,4,,0000010D"));		// VC-1
	chkbytes.AddTail(_T("0,5,,0000000140"));	// H.265/HEVC
#if ENABLE_YUV4MPEG2
	chkbytes.AddTail(_T("0,9,,595556344D50454732")); // YUV4MPEG2
#endif

	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_NULL,
		chkbytes,
		_T(".mpeg"), _T(".mpg"), _T(".m2v"), _T(".mpv"),
		_T(".h264"), _T(".264"),
		_T(".vc1"),
		_T(".h265"), _T(".265"), _T(".hm10"), _T(".hevc"),
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
static const BYTE FRAME_[6] = {'F','R','A','M','E',0x0A};
#endif

//
// CRawVideoSplitterFilter
//

CRawVideoSplitterFilter::CRawVideoSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CRawVideoSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_RAWType(RAW_NONE)
	, m_startpos(0)
	, m_framesize(0)
	, m_rtStart(0)
	, m_AvgTimePerFrame(0)
{
}

STDMETHODIMP CRawVideoSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CRawVideoSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, RawVideoSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

bool CRawVideoSplitterFilter::ReadGOP(REFERENCE_TIME& rt)
{
	m_pFile->BitRead(1);
	BYTE hour		= m_pFile->BitRead(5);
	BYTE minute		= m_pFile->BitRead(6);
	m_pFile->BitRead(1);
	BYTE second		= m_pFile->BitRead(6);
	BYTE frame		= m_pFile->BitRead(6);
	BYTE closedGOP	= m_pFile->BitRead(1);
	BYTE brokenGOP	= m_pFile->BitRead(1);
	if (brokenGOP || !frame) {
		return false;
	}

	rt = (((hour * 60) + minute) * 60 + second) * UNITS;

	return true;
}

HRESULT CRawVideoSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
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

	CAtlArray<CMediaType> mts;
	CMediaType mt;
	{
		RAWType rawType = RAW_NONE;

#if ENABLE_YUV4MPEG2
		{
			static const BYTE YUV4MPEG2_[10] = {'Y','U','V','4','M','P','E','G','2',0x20};
			BYTE buf[256] = {0};
			if (m_pFile->ByteRead(buf, 255) == S_OK && memcmp(buf, YUV4MPEG2_, sizeof(YUV4MPEG2_)) == 0) {
				CStringA params = CStringA(buf + sizeof(YUV4MPEG2_));
				params.Truncate(params.Find(0x0A));

				int firstframepos = sizeof(YUV4MPEG2_) + params.GetLength() + 1;
				if (firstframepos + sizeof(FRAME_) > 255 || memcmp(buf + firstframepos, FRAME_, sizeof(FRAME_)) != 0) {
					return E_FAIL; // incorrect or unsuppurted YUV4MPEG2 file
				}

				int    width		= 0;
				int    height		= 0;
				int    fpsnum		= 24;
				int    fpsden		= 1;
				int    sar_x		= 1;
				int    sar_y		= 1;
				FOURCC fourcc		= FCC('I420'); // 4:2:0 - I420 by default
				FOURCC fourccRAW	= FCC('I420'); // 4:2:0 - I420 by default
				WORD   bpp			= 12;
				DWORD  interl		= 0; // 

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
							fpsnum = atoi(str.Mid(1, k - 1));
							fpsden = atoi(str.Mid(k + 1));
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
							sar_x = atoi(str.Mid(1, k - 1));
							sar_y = atoi(str.Mid(k + 1));
							if (sar_x <= 0 || sar_y <= 0) { // if 'A0:0' = unknown or bad value
								sar_x = sar_y = 1; // then force 'A1:1' = square pixels
							}
							break;
						case 'C':
							str = str.Mid(1);
							// 4:2:0
							if (str == "420" || str == "420jpeg" || str == "420mpeg2" || str == "420paldv") {
								fourcc		= FCC('I420');
								fourccRAW	= FCC('I420');
								bpp			= 12;
							}
							else if (str == "420p10") {
								fourcc		= FCC('P010');
								fourccRAW	= FCC('I420');
								bpp			= 15;
							}
							else if (str == "420p16") {
								fourcc		= FCC('P016');
								fourccRAW	= FCC('I420');
								bpp			= 24;
							}
							// 4:2:2
							else if (str == "422") {
								fourcc		= FCC('I422');
								fourccRAW	= FCC('422P');
								bpp			= 16;
							}
							else if (str == "422p10") {
								fourcc		= FCC('P210');
								fourccRAW	= FCC('422P');
								bpp			= 20;
							}
							else if (str == "422p16") {
								fourcc		= FCC('P216');
								fourccRAW	= FCC('422P');
								bpp			= 32;
							}
							// 4:4:4
							else if (str == "444") {
								fourcc		= FCC('I444');
								fourccRAW	= FCC('444P');
								bpp			= 24;
							}
							else { // unsuppurted colour space 
								fourcc		= 0;
								fourccRAW	= 0;
								bpp			= 0;
							}
							break;
						//case 'X':
						//	break;
					}
				}

				if (width <= 0 || height <= 0 || fpsnum <= 0 || fpsden <= 0 || fourcc == 0) {
					return E_FAIL; // incorrect or unsuppurted YUV4MPEG2 file
				}

				m_startpos  = firstframepos;
				m_framesize = width * height * bpp >> 3;

				mt.majortype  = MEDIATYPE_Video;
				mt.subtype    = MEDIASUBTYPE_LAV_RAWVIDEO;
				mt.formattype = FORMAT_VIDEOINFO2;

				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

				memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

				vih2->bmiHeader.biSize        = sizeof(vih2->bmiHeader);
				vih2->bmiHeader.biWidth       = width;
				vih2->bmiHeader.biHeight      = height;
				vih2->bmiHeader.biPlanes      = 1;
				vih2->bmiHeader.biBitCount    = bpp;
				vih2->bmiHeader.biCompression = fourccRAW;
				vih2->bmiHeader.biSizeImage   = m_framesize;
				//vih2->rcSource = vih2->rcTarget = CRect(0, 0, width, height);
				//vih2->dwBitRate      = m_framesize * 8 * fpsnum / fpsden;
				vih2->AvgTimePerFrame  = 10000000i64 * fpsden / fpsnum;
				// always tell DirectShow it's interlaced (progressive flags set in IMediaSample struct)
				vih2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOrWeave;

				sar_x *= width;
				sar_y *= height;
				ReduceDim(sar_x, sar_y);
				vih2->dwPictAspectRatioX = sar_x;
				vih2->dwPictAspectRatioY = sar_y;

				m_AvgTimePerFrame = vih2->AvgTimePerFrame;
				m_rtDuration      = (m_pFile->GetLength() - m_startpos) / (sizeof(FRAME_) + m_framesize) * 10000000i64 * fpsden / fpsnum;
				mt.SetSampleSize(m_framesize);
				mts.Add(mt);

				mt.subtype = FOURCCMap(fourcc);
				vih2->bmiHeader.biCompression = fourcc;
				mts.Add(mt);

				m_RAWType   = RAW_Y4M;
			}
		}
#endif

		if (m_RAWType == RAW_NONE) {
			// check sync bytes ...
			BYTE sync[5] = { 0 };

			struct {
				const BYTE		sync[5];
				const BYTE		count;
				const RAWType	rawType;
			} SYNCHDR[] = {
				{ {0x00, 0x00, 0x00, 0x01, 0x40}, 5, RAW_HEVC  },
				{ {0x00, 0x00, 0x01, 0xB3},       4, RAW_MPEG1 },
				{ {0x00, 0x00, 0x00, 0x01},       4, RAW_H264  },
				{ {0x00, 0x00, 0x01, 0x0F},       4, RAW_VC1   },
				{ {0x00, 0x00, 0x01, 0x0D},       4, RAW_VC1   },
			};

			m_pFile->Seek(0);
			if (SUCCEEDED(m_pFile->ByteRead(sync, _countof(sync)))) {
				for (size_t i = 0; i < _countof(SYNCHDR); i++) {
					if (!memcmp(sync, SYNCHDR[i].sync, SYNCHDR[i].count)) {
						rawType = SYNCHDR[i].rawType;
						break;
					}
				}
			}

			if (rawType == RAW_NONE) {
				return E_FAIL;
			}
		}

		if (m_RAWType == RAW_NONE && rawType == RAW_MPEG1) {
			m_pFile->Seek(0);
			CBaseSplitterFileEx::seqhdr h;
			if (m_pFile->Read(h, min(MEGABYTE, m_pFile->GetLength()), &mt, false)) {
				mts.Add(mt);

				m_RAWType			= RAW_MPEG2;
				if (mt.subtype == MEDIASUBTYPE_MPEG1Payload) {
					m_RAWType		= RAW_MPEG1;
				}
				m_AvgTimePerFrame	= h.ifps;

				REFERENCE_TIME rtStart	= INVALID_TIME;
				REFERENCE_TIME rtStop	= INVALID_TIME;

				__int64 posMin			= 0;
				__int64 posMax			= 0;
				// find start PTS
				{
					BYTE id = 0x00;
					m_pFile->Seek(0);
					while (m_pFile->GetPos() < min(MEGABYTE, m_pFile->GetLength()) && rtStart == INVALID_TIME) {
						if (!m_pFile->NextMpegStartCode(id)) {
							continue;
						}

						if (id == 0xb8) {	// GOP
							REFERENCE_TIME rt = 0;
							__int64 pos = m_pFile->GetPos();
							if (!ReadGOP(rt)) {
								continue;
							}

							if (rtStart == INVALID_TIME) {
								rtStart	= rt;
								posMin	= pos;
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
							__int64 pos = m_pFile->GetPos();
							if (ReadGOP(rtStop)) {
								posMax = pos;
							}
						}
					}
				}

				if (rtStart != INVALID_TIME && rtStop != INVALID_TIME && rtStop > rtStart) {
					double rate = (double)(posMax - posMin) / (rtStop - rtStart);
					m_rtNewStop = m_rtStop = m_rtDuration = (REFERENCE_TIME)((double)m_pFile->GetLength() / rate);
				}
			}
		}

		if (m_RAWType == RAW_NONE && rawType == RAW_H264) {
			m_pFile->Seek(0);

			CBaseSplitterFileEx::avchdr h;
			if (m_pFile->Read(h, min(MEGABYTE, m_pFile->GetLength()), &mt)) {
				mts.Add(mt);
				m_RAWType = RAW_H264;
			}
		
		}

		if (m_RAWType == RAW_NONE && rawType == RAW_VC1) {
			BYTE id = 0x00;
			m_pFile->Seek(0);
			while (m_pFile->GetPos() < min(MEGABYTE, m_pFile->GetLength()) && m_RAWType == RAW_NONE) {
				if (!m_pFile->NextMpegStartCode(id)) {
					continue;
				}

				if (id == 0x0F) {	// sequence header
					m_pFile->Seek(m_pFile->GetPos() - 4);

					CBaseSplitterFileEx::vc1hdr h;
					if (m_pFile->Read(h, 1024, &mt)) {
						mts.Add(mt);
						m_RAWType = RAW_VC1;
					}
				}
			}
		}

		if (m_RAWType == RAW_NONE && rawType == RAW_HEVC) {
			m_pFile->Seek(0);

			CBaseSplitterFileEx::hevchdr h;
			if (m_pFile->Read(h, min(MEGABYTE, m_pFile->GetLength()), &mt, false)) {
				// set 25 fps as default value.
				// TODO - detect real fps.
				MPEG2VIDEOINFO* pm2vi		= (MPEG2VIDEOINFO*)mt.pbFormat;
				pm2vi->hdr.AvgTimePerFrame	= 400000;
				
				mts.Add(mt);
				m_RAWType					= RAW_HEVC;
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
				pName = L"H.264/AVC1 Video Output";
				break;
			case RAW_VC1:
				pName = L"VC-1 Video Output";
				break;
			case RAW_HEVC:
				pName = L"H.265/HEVC Video Output";
				break;
#if ENABLE_YUV4MPEG2
			case RAW_Y4M:
				pName = L"YUV4MPEG2 Video Output";
				break;
#endif
		}

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CRawVideoOutputPin(mts, pName, this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CRawVideoSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CRawVideoSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	m_pFile->Seek(0);

	return true;
}

void CRawVideoSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
#if ENABLE_YUV4MPEG2
	if (m_RAWType == RAW_Y4M) {
		if (rt <= 0) {
			m_pFile->Seek(m_startpos);
			m_rtStart = 0;
		} else {
			__int64 framenum = rt / m_AvgTimePerFrame;
			m_pFile->Seek(m_startpos + framenum * (sizeof(FRAME_) + m_framesize));
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

bool CRawVideoSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;
	
	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining()) {
		CAutoPtr<Packet> p(DNew Packet());
		p->TrackNumber	= 0;
		p->bSyncPoint	= FALSE;

#if ENABLE_YUV4MPEG2
		if (m_RAWType == RAW_Y4M) {

			if (m_pFile->GetRemaining() < sizeof(FRAME_) + m_framesize) {
				break;
			}

			BYTE buf[sizeof(FRAME_)];
			if (m_pFile->ByteRead(buf, sizeof(FRAME_)) != S_OK || memcmp(buf, FRAME_, sizeof(FRAME_)) != 0) {
				ASSERT(0);
			}
			__int64 framenum = (m_pFile->GetPos() - m_startpos) / (sizeof(FRAME_) + m_framesize);

			p->SetCount(m_framesize);
			m_pFile->ByteRead(p->GetData(), m_framesize);
			
			p->rtStart     = framenum * m_AvgTimePerFrame;
			p->rtStop      = p->rtStart + m_AvgTimePerFrame;

			hr = DeliverPacket(p);
			continue;
		}
#endif

		size_t size		= min(64 * KILOBYTE, m_pFile->GetRemaining());

		p->SetCount(size);
		m_pFile->ByteRead(p->GetData(), size);

		p->rtStart		= INVALID_TIME;
		p->rtStop		= INVALID_TIME;

		hr = DeliverPacket(p);
	}

	return true;
}

//
// CRawVideoSourceFilter
//

CRawVideoSourceFilter::CRawVideoSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRawVideoSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// CMpegSplitterOutputPin
//

CRawVideoOutputPin::CRawVideoOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
	, m_fHasAccessUnitDelimiters(false)
{
}

CRawVideoOutputPin::~CRawVideoOutputPin()
{
	Flush();
}

HRESULT CRawVideoOutputPin::Flush()
{
	CAutoLock cAutoLock(this);

	m_p.Free();
	m_pl.RemoveAll();

	m_bFlushed = true;

	return S_OK;
}

HRESULT CRawVideoOutputPin::DeliverEndFlush()
{
	CAutoLock cAutoLock(this);

	Flush();

	return __super::DeliverEndFlush();
}

HRESULT CRawVideoOutputPin::DeliverPacket(CAutoPtr<Packet> p)
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


