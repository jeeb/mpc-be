/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include <moreuuids.h>
#include "BaseSplitterParserOutputPin.h"

#include "../../../DSUtil/AudioParser.h"

#define MOVE_TO_H264_START_CODE(b, e)	while(b <= e - 4 && !((*(DWORD*)b == 0x01000000) || ((*(DWORD*)b & 0x00FFFFFF) == 0x00010000))) b++; if((b <= e - 4) && *(DWORD*)b == 0x01000000) b++;
#define MOVE_TO_AC3_START_CODE(b, e)	while(b <= e - 8 && (*(WORD*)b != 0x770b)) b++;
#define MOVE_TO_AAC_START_CODE(b, e)	while(b <= e - 9 && ((*(WORD*)b & 0xf0ff) != 0xf0ff)) b++;

#ifndef BSWAP32
#define BSWAP32(x)	((x >> 24) & 0x000000ff) | \
					((x >>  8) & 0x0000ff00) | \
					((x <<  8) & 0x00ff0000) | \
					((x << 24) & 0xff000000);
#endif

//
// CBaseSplitterParserOutputPin
//

CBaseSplitterParserOutputPin::CBaseSplitterParserOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int QueueMaxPackets)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr, 0, QueueMaxPackets)
	, m_fHasAccessUnitDelimiters(false)
	, m_bFlushed(false)
	, m_truehd_framelength(0)
	, m_nChannels(0)
	, m_nSamplesPerSec(0)
{
}

CBaseSplitterParserOutputPin::~CBaseSplitterParserOutputPin()
{
	Flush();
}

void CBaseSplitterParserOutputPin::InitAudioParams()
{
	if (m_mt.pbFormat) {
		const WAVEFORMATEX *wfe	= GetFormatHelper(wfe, &m_mt);
		m_nChannels				= wfe->nChannels;
		m_nSamplesPerSec		= wfe->nSamplesPerSec;
	}
}

HRESULT CBaseSplitterParserOutputPin::Flush()
{
	CAutoLock cAutoLock(this);

	m_p.Free();
	m_pl.RemoveAll();

	m_bFlushed				= true;
	m_truehd_framelength	= 0;

	m_hdmvLPCM.Clear();

	InitAudioParams();

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::DeliverEndFlush()
{
	CAutoLock cAutoLock(this);

	Flush();

	return __super::DeliverEndFlush();
}

void CBaseSplitterParserOutputPin::InitPacket(Packet* pSource)
{
	m_p.Attach(DNew Packet());
	m_p->TrackNumber		= pSource->TrackNumber;
	m_p->bDiscontinuity		= pSource->bDiscontinuity;
	pSource->bDiscontinuity	= FALSE;

	m_p->bSyncPoint			= pSource->bSyncPoint;
	pSource->bSyncPoint		= FALSE;

	m_p->rtStart			= pSource->rtStart;
	pSource->rtStart		= INVALID_TIME;

	m_p->rtStop				= pSource->rtStop;
	pSource->rtStop			= INVALID_TIME;
}

#define HandlePacket \
	CAutoPtr<Packet> p2(DNew Packet());			\
	p2->TrackNumber		= m_p->TrackNumber;		\
	p2->bDiscontinuity	= m_p->bDiscontinuity;	\
	m_p->bDiscontinuity	= FALSE;				\
	\
	p2->bSyncPoint	= m_p->bSyncPoint;			\
	m_p->bSyncPoint	= FALSE;					\
	\
	p2->rtStart		= m_p->rtStart;				\
	m_p->rtStart	= INVALID_TIME;				\
	\
	p2->rtStop		= m_p->rtStop;				\
	m_p->rtStop		= INVALID_TIME;				\
	\
	p2->pmt		= m_p->pmt;						\
	m_p->pmt	= NULL;							\
	\
	p2->SetData(start, size);					\
	\
	if (!p2->pmt && m_bFlushed) {				\
		p2->pmt		= CreateMediaType(&m_mt);	\
		m_bFlushed	= false;					\
	}											\
	\
	HRESULT hr = __super::DeliverPacket(p2);	\
	if (hr != S_OK) {							\
		return hr;								\
	}											\
	\
	if (p->rtStart != INVALID_TIME) {			\
		m_p->rtStart	= p->rtStart;			\
		m_p->rtStop		= p->rtStop;			\
		p->rtStart		= INVALID_TIME;			\
	}											\
	if (p->bDiscontinuity) {					\
		m_p->bDiscontinuity	= p->bDiscontinuity;\
		p->bDiscontinuity	= FALSE;			\
	}											\
	if (p->bSyncPoint) {						\
		m_p->bSyncPoint	= p->bSyncPoint;		\
		p->bSyncPoint	= FALSE;				\
	}											\
	if (m_p->pmt) {								\
		DeleteMediaType(m_p->pmt);				\
	}											\
	\
	m_p->pmt	= p->pmt;						\
	p->pmt		= NULL;							\


HRESULT CBaseSplitterParserOutputPin::DeliverPacket(CAutoPtr<Packet> p)
{
	CAutoLock cAutoLock(this);

	if (p->pmt) {
		if (*((CMediaType*)p->pmt) != m_mt) {
			SetMediaType((CMediaType*)p->pmt);
			Flush();
		}
	}

	if (m_mt.majortype == MEDIATYPE_Audio && (!m_nChannels || !m_nSamplesPerSec)) {
		InitAudioParams();
	}

	if (m_mt.subtype == MEDIASUBTYPE_RAW_AAC1) { // special code for aac, the currently available decoders only like whole frame samples
		// AAC
		return ParseAAC(p);
	} else if (m_mt.subtype == FOURCCMap('1CVA') || m_mt.subtype == FOURCCMap('1cva') || m_mt.subtype == FOURCCMap('CVMA') || m_mt.subtype == FOURCCMap('CVME')) {
		// H.264/AVC/CVMA/CVME
		return ParseAnnexB(p);
	} else if (m_mt.subtype == FOURCCMap('1CVW') || m_mt.subtype == FOURCCMap('1cvw') || m_mt.subtype == MEDIASUBTYPE_WVC1_CYBERLINK || m_mt.subtype == MEDIASUBTYPE_WVC1_ARCSOFT) { // just like aac, this has to be starting nalus, more can be packed together
		// VC1
		return ParseVC1(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
		// HDMV LPCM
		return ParseHDMVLPCM(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DOLBY_AC3) {
		// Dolby AC3 - core only
		return ParseAC3(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
		// TrueHD only - skip AC3 core
		return ParseTrueHD(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DRAC) {
		// Dirac
		return ParseDirac(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE || m_mt.subtype == MEDIASUBTYPE_VOBSUB) {
		// DVD(VobSub) Subtitle
		return ParseVobSub(p);
	} else {
		m_p.Free();
		m_pl.RemoveAll();
	}

	return __super::DeliverPacket(p);
}

HRESULT CBaseSplitterParserOutputPin::ParseAAC(CAutoPtr<Packet> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	m_p->Append(*p);

	if (m_p->GetCount() < 9) {
		return S_OK;	// Should be invalid packet
	}

	BYTE* start	= m_p->GetData();
	BYTE* end	= start + m_p->GetCount();

	for(;;) {
		MOVE_TO_AAC_START_CODE(start, end);
		if (start <= end - 9) {
			audioframe_t aframe;
			int size = ParseADTSAACHeader(start, &aframe);
			if (size == 0) {
				start++;
				continue;
			}
			if (start + size > end) {
				break;
			}

			if (start + size + 9 <= end) {
				int size2 = ParseADTSAACHeader(start + size, &aframe);
				if (size2 == 0) {
					start++;
					continue;
				}
			}

			HandlePacket;
							
			start += size;
		} else {
			break;
		}
	}

	if (start > m_p->GetData()) {
		m_p->RemoveAt(0, start - m_p->GetData());
	}

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseAnnexB(CAutoPtr<Packet> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	m_p->Append(*p);

	BYTE* start	= m_p->GetData();
	BYTE* end	= start + m_p->GetCount();

	MOVE_TO_H264_START_CODE(start, end);

	while (start <= end - 4) {
		BYTE* next = start + 1;

		MOVE_TO_H264_START_CODE(next, end);

		if (next >= end - 4) {
			break;
		}

		int size = next - start;

		CH264Nalu Nalu;
		Nalu.SetBuffer(start, size, 0);

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

		if ((pData[4] & 0x1f) == 0x09) {
			m_fHasAccessUnitDelimiters = true;
		}

		if ((pData[4] & 0x1f) == 0x09 || (!m_fHasAccessUnitDelimiters && pPacket->rtStart != INVALID_TIME)) {
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

HRESULT CBaseSplitterParserOutputPin::ParseVC1(CAutoPtr<Packet> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	m_p->Append(*p);

	BYTE* start = m_p->GetData();
	BYTE* end = start + m_p->GetCount();

	bool bSeqFound = false;
	while (start <= end - 4) {
		if (*(DWORD*)start == 0x0D010000) {
			bSeqFound = true;
			break;
		} else if (*(DWORD*)start == 0x0F010000) {
			break;
		}
		start++;
	}

	while (start <= end - 4) {
		BYTE* next = start + 1;

		while (next <= end - 4) {
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

		if (next >= end - 4) {
			break;
		}

		int size = next - start;

		HandlePacket;

		start		= next;
		bSeqFound	= (*(DWORD*)start == 0x0D010000);
	}

	if (start > m_p->GetData()) {
		m_p->RemoveAt(0, start - m_p->GetData());
	}

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseHDMVLPCM(CAutoPtr<Packet> p)
{
	if (!m_p) {
		m_p.Attach(DNew Packet());
	}
	m_p->Append(*p);

	if (m_p->GetCount() < 4) {
		m_p.Free();
		return S_OK;	// Should be invalid packet
	}

	BYTE* start = m_p->GetData();
	audioframe_t aframe;
	size_t packet_size = ParseHdmvLPCMHeader(start, &aframe);

	if (!packet_size || packet_size > m_p->GetCount()) {
		if (!packet_size) {
			m_p.Free();
		}
		return S_OK;
	}

	if (!m_hdmvLPCM.samplerate) {
		m_hdmvLPCM.samplerate	= aframe.samplerate;
		m_hdmvLPCM.channels		= aframe.channels;
		m_hdmvLPCM.packetsize	= packet_size;
	}

	if (m_hdmvLPCM.samplerate != aframe.samplerate
		|| m_hdmvLPCM.channels != aframe.channels
		|| m_hdmvLPCM.packetsize != packet_size) {
		m_p.Free();
		return S_OK;
	}

	if (!p->pmt && m_bFlushed) {
		p->pmt = CreateMediaType(&m_mt);
		m_bFlushed = false;
	}
	p->SetData(start + 4, m_p->GetCount() - 4);
	m_p.Free();

	return __super::DeliverPacket(p);
}

HRESULT CBaseSplitterParserOutputPin::ParseAC3(CAutoPtr<Packet> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	m_p->Append(*p);

	if (m_p->GetCount() < 8) {
		return S_OK;	// Should be invalid packet
	}

	BYTE* start	= m_p->GetData();
	BYTE* end	= start + m_p->GetCount();

	for(;;) {
		MOVE_TO_AC3_START_CODE(start, end);
		if (start <= end - 8) {
			audioframe_t aframe;
			int size = ParseAC3Header(start, &aframe);
			if (size == 0) {
				start++;
				continue;
			}

			if (m_nChannels != aframe.channels || m_nSamplesPerSec != aframe.samplerate) {
				start++;
				continue;
			}
			if (start + size > end) {
				break;
			}

			HandlePacket;
							
			start += size;
		} else {
			break;
		}
	}

	if (start > m_p->GetData()) {
		m_p->RemoveAt(0, start - m_p->GetData());
	}

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseTrueHD(CAutoPtr<Packet> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	m_p->Append(*p);

	if (m_p->GetCount() < 16) {
		return S_OK;	// Should be invalid packet
	}

	BYTE* start	= m_p->GetData();
	BYTE* end	= start + m_p->GetCount();

	while (start + 16 <= end) {
		audioframe_t aframe;
		int size = ParseMLPHeader(start, &aframe);
		if (size > 0) {
			// sync frame
			m_truehd_framelength = aframe.samples;
		} else {
			int ac3size = ParseAC3Header(start);
			if (ac3size == 0) {
				ac3size = ParseEAC3Header(start);
			}
			if (ac3size > 0) {
				if (start + ac3size > end) {
					break;
				}
				start += ac3size;
				continue; // skip ac3 frames
			}
		}

		if (size == 0 && m_truehd_framelength > 0) {
			// get not sync frame size
			size = ((start[0] << 8 | start[1]) & 0xfff) * 2;
		}

		if (size < 8) {
			start++;
			continue;
		}
		if (start + size > end) {
			break;
		}

		HandlePacket;
							
		start += size;
	}

	if (start > m_p->GetData()) {
		m_p->RemoveAt(0, start - m_p->GetData());
	}

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseDirac(CAutoPtr<Packet> p)
{
	if (p->GetCount() < 4) {
		return S_OK;    // Should be invalid packet
	}

	BYTE* start = p->GetData();
	if (*(DWORD*)start != 0x44434242) { // not found Dirac SYNC 'BBCD'
		if (!m_p) {
			return S_OK;
		}

		start = m_p->GetData();
		if (*(DWORD*)start != 0x44434242) { // not found Dirac SYNC 'BBCD'
			return S_OK;
		}

		m_p->Append(*p);
		return S_OK;
	}

	HRESULT hr = S_OK;

	if (m_p) {
		CAutoPtr<Packet> p2(DNew Packet());
		p2->TrackNumber		= m_p->TrackNumber;
		p2->bDiscontinuity	= m_p->bDiscontinuity;
		p2->bSyncPoint		= m_p->bSyncPoint;
		p2->rtStart			= m_p->rtStart;
		p2->rtStop			= m_p->rtStop;
		p2->pmt				= m_p->pmt;
		p2->SetData(m_p->GetData(), m_p->GetCount());
		m_p.Free();

		if (!p2->pmt && m_bFlushed) {
			p2->pmt = CreateMediaType(&m_mt);
			m_bFlushed = false;
		}
		
		hr = __super::DeliverPacket(p2);
	}

	m_p.Attach(DNew Packet());
	m_p->TrackNumber	= p->TrackNumber;
	m_p->bDiscontinuity	= p->bDiscontinuity;
	m_p->bSyncPoint		= p->bSyncPoint;
	m_p->rtStart		= p->rtStart;
	m_p->rtStop			= p->rtStop;
	m_p->pmt			= p->pmt;
	m_p->Append(*p);

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseVobSub(CAutoPtr<Packet> p)
{
	HRESULT hr = S_OK;

	if (!m_p) {
		InitPacket(p);
	}

	m_p->Append(*p);

	BYTE* pData = m_p->GetData();
	int len = (pData[0] << 8) | pData[1];

	if (!len) {
		if (m_p) {
			m_p.Free();
		}
		return hr;

	}

	if (m_p->GetCount() <= 4 || (len > (int)m_p->GetCount())) {
		return hr;
	}

	if (m_p) {
		CAutoPtr<Packet> p2(DNew Packet());
		p2->TrackNumber		= m_p->TrackNumber;
		p2->bDiscontinuity	= m_p->bDiscontinuity;
		p2->bSyncPoint		= m_p->bSyncPoint;
		p2->rtStart			= m_p->rtStart;
		p2->rtStop			= m_p->rtStop;
		p2->pmt				= m_p->pmt;
		p2->SetData(m_p->GetData(), m_p->GetCount());
		m_p.Free();

		if (!p2->pmt && m_bFlushed) {
			p2->pmt = CreateMediaType(&m_mt);
			m_bFlushed = false;
		}
		
		hr = __super::DeliverPacket(p2);
	}

	return hr;
}