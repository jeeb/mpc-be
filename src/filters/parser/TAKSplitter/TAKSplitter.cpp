/*
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
#include "TAKSplitter.h"
#include <moreuuids.h>
#include "../DSUtil/ApeTag.h"

enum TAKMetaDataType {
	TAK_METADATA_END = 0,
	TAK_METADATA_STREAMINFO,
	TAK_METADATA_SEEKTABLE,
	TAK_METADATA_SIMPLE_WAVE_DATA,
	TAK_METADATA_ENCODER,
	TAK_METADATA_PADDING,
	TAK_METADATA_MD5,
	TAK_METADATA_LAST_FRAME,
};

enum TAKFrameType {
	TAK_FRAME_94ms = 0,
	TAK_FRAME_125ms,
	TAK_FRAME_188ms,
	TAK_FRAME_250ms,
	TAK_FRAME_4096,
	TAK_FRAME_8192,
	TAK_FRAME_16384,
	TAK_FRAME_512,
	TAK_FRAME_1024,
	TAK_FRAME_2048,
};

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
	, m_startpos(0)
	, m_endpos(0)
	, m_nAvgBytesPerSec(0)
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

#define BSWAP24(x) { x = (((x & 0xff) << 16) | (x >> 16) | (x & 0xff00)); }
#define BSWAP32(x) { x = (((x & 0xff00ff) << 8) | ((x >> 8) & 0xff00ff)); \
					 x = (x << 16) | (x >> 16); }

#define CRC24_INIT 0xb704ceL
#define CRC24_POLY 0x1864cfbL

inline long crc_octets(BYTE *octets, size_t len)
{
	long crc = CRC24_INIT;

	while (len--) {
		crc ^= (*octets++) << 16;
		for (int i = 0; i < 8; i++) {
			crc <<= 1;
			if (crc & 0x1000000) {
				crc ^= CRC24_POLY;
			}
		}
	}

	return crc & 0xffffffL;
}

HRESULT CTAKSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
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

	if (m_pFile->BitRead(32) != 'tBaK') {
		return E_FAIL;
	}

	BOOL bLastFrame	= FALSE;
	BOOL bIsEnd		= FALSE;
	while ((m_pFile->GetRemaining() >= 4) && !bIsEnd) {
		TAKMetaDataType type	= (TAKMetaDataType)(m_pFile->BitRead(8) & 0x7f);
		int size				= m_pFile->BitRead(24);
		BSWAP24(size);

		BYTE* buf				= NULL;

		switch (type) {
			case TAK_METADATA_STREAMINFO:
			case TAK_METADATA_LAST_FRAME:
				{
					buf = DNew BYTE[size];
					if (!buf) {
						return E_FAIL;
					}
					if (FAILED(m_pFile->ByteRead(buf, size))) {
						delete [] buf;
						return E_FAIL;
					}

					// check crc
					m_pFile->Seek(m_pFile->GetPos() - 3);
					int crc		= m_pFile->BitRead(24);
					BSWAP24(crc);
					if (crc != crc_octets(buf, size - 3)) {
						delete [] buf;
						return E_FAIL;
					}
				}
				break;
			case TAK_METADATA_END:
				{
					m_endpos += m_pFile->GetPos();

					// parse APE Tag Header
					BYTE buf[APE_TAG_FOOTER_BYTES];
					memset(buf, 0, sizeof(buf));
					__int64 cur_pos = m_pFile->GetPos();
					__int64 file_size = m_pFile->GetLength();

					CAtlArray<BYTE>		CoverData;
					CString				CoverMime;
					CString				CoverFileName;

					if (cur_pos + APE_TAG_FOOTER_BYTES <= file_size) {
						m_pFile->Seek(file_size - APE_TAG_FOOTER_BYTES);
						if (SUCCEEDED(m_pFile->ByteRead(buf, APE_TAG_FOOTER_BYTES))) {
							CAPETag* APETag = DNew CAPETag;
							if (APETag->ReadFooter(buf, APE_TAG_FOOTER_BYTES) && APETag->GetTagSize()) {
								size_t tag_size = APETag->GetTagSize();
								m_pFile->Seek(file_size - tag_size);
								BYTE *p = DNew BYTE[tag_size];
								if (SUCCEEDED(m_pFile->ByteRead(p, tag_size)) && APETag->ReadTags(p, tag_size)) {
									POSITION pos = APETag->TagItems.GetHeadPosition();
									while (pos) {
										CApeTagItem* item = APETag->TagItems.GetAt(pos);
										CString TagKey = item->GetKey();
										TagKey.MakeLower();

										if (item->GetType() == CApeTagItem::APE_TYPE_BINARY) {
											if (!TagKey.IsEmpty()) {
												CString ext = TagKey.Mid(TagKey.ReverseFind('.')+1);
												if (ext == _T("jpeg") || ext == _T("jpg")) {
													CoverMime = _T("image/jpeg");
												} else if (ext == _T("png")) {
													CoverMime = _T("image/png");
												}
											}

											if (!CoverData.GetCount() && !CoverMime.IsEmpty()) {
												CoverFileName = TagKey;
												CoverData.SetCount(tag_size);
												memcpy(CoverData.GetData(), item->GetData(), item->GetDataLen());
											}

											ResAppend(CoverFileName, _T("cover"), CoverMime, CoverData.GetData(), (DWORD)CoverData.GetCount());

										} else {
											CString TagValue = item->GetValue();
											if (TagKey == _T("cuesheet")) {
												CAtlList<Chapters> ChaptersList;
												if (ParseCUESheet(TagValue, ChaptersList)) {
													ChapRemoveAll();
													while (ChaptersList.GetCount()) {
														Chapters cp = ChaptersList.RemoveHead();
														ChapAppend(cp.rt, cp.name);
													}
												}
											}

											if (TagKey == _T("artist")) {
												SetProperty(L"AUTH", TagValue);
											} else if (TagKey == _T("comment")) {
												SetProperty(L"DESC", TagValue);
											} else if (TagKey == _T("title")) {
												SetProperty(L"TITL", TagValue);
											} else if (TagKey == _T("year")) {
												SetProperty(L"YEAR", TagValue);
											} else if (TagKey == _T("album")) {
												SetProperty(L"ALBUM", TagValue);
											}
										}

										APETag->TagItems.GetNext(pos);
									}
								}

								delete [] p;
							}

							delete APETag;
						}

						m_pFile->Seek(cur_pos);
					}

					bIsEnd = TRUE;
				}
				break;
			default:
				m_pFile->Seek(m_pFile->GetPos() + size);
				m_startpos = m_pFile->GetPos() + 4;
		}

		if (type == TAK_METADATA_STREAMINFO) {
			TAKStreamInfo si;
			ParseTAKStreamInfo(buf, size - 3, si);

			if (si.data_type || !si.channels || !si.frame_size) {
				delete [] buf;
				return E_FAIL;
			}

			CMediaType mt;
			mt.majortype			= MEDIATYPE_Audio;
			mt.formattype			= FORMAT_WaveFormatEx;
			mt.subtype				= MEDIASUBTYPE_TAK;
			WAVEFORMATEX* wfe		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + size);
			memset(wfe, 0, sizeof(WAVEFORMATEX));
			wfe->nSamplesPerSec		= si.sample_rate;
			wfe->wBitsPerSample		= si.bps;
			wfe->nChannels			= si.channels;

			wfe->nBlockAlign		= wfe->nChannels * wfe->wBitsPerSample >> 3;
			wfe->nAvgBytesPerSec	= wfe->nSamplesPerSec * wfe->nBlockAlign;
			wfe->cbSize				= size;
			memcpy(wfe + 1, buf, size);

			mt.SetSampleSize(256000);

			if (mt.subtype != GUID_NULL) {
				CAtlArray<CMediaType> mts;
				mts.Add(mt);
				CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Tak Audio Output", this, this, &hr));
				EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
			}

			m_nAvgBytesPerSec		= wfe->nAvgBytesPerSec;
			m_rtDuration			= si.samples / si.sample_rate * UNITS;
		} else if (type == TAK_METADATA_LAST_FRAME) {
			bLastFrame				= TRUE;
			uint64_t LastFramePos	= *(uint64_t*)buf & 0xFFFFFFFFFF;
			uint64_t LastFrameSize	= *(uint32_t*)&buf[4] >> 8;

			m_endpos				= LastFramePos + LastFrameSize;
		}

		SAFE_DELETE_ARRAY(buf);
	}

	if (!bLastFrame) {
		m_endpos = m_pFile->GetLength();
	}

	if (m_startpos > m_endpos) {
		ASSERT(0);
		m_startpos = m_endpos;
	}

	m_pFile->Seek(m_startpos);

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
	if (rt <= 0 || m_rtDuration <= 0) {
		m_pFile->Seek(m_startpos);
		m_rtStart = 0;
	} else {
		m_pFile->Seek((__int64)((1.0 * rt / m_rtDuration) * (m_endpos - m_startpos)));
		m_rtStart = rt;
	}
}

bool CTAKSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetPos() < m_endpos) {

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

void CTAKSplitterFilter::ParseTAKStreamInfo(BYTE* buf, int size, TAKStreamInfo& si)
{
	static const uint16_t frame_duration_type_quants[] = { 3, 4, 6, 8, 4096, 8192, 16384, 512, 1024, 2048 };
	static const DWORD tak_channels[] = {
		0,
		SPEAKER_FRONT_LEFT,
		SPEAKER_FRONT_RIGHT,
		SPEAKER_FRONT_CENTER,
		SPEAKER_LOW_FREQUENCY,
		SPEAKER_BACK_LEFT,
		SPEAKER_BACK_RIGHT,
		SPEAKER_FRONT_LEFT_OF_CENTER,
		SPEAKER_FRONT_RIGHT_OF_CENTER,
		SPEAKER_BACK_CENTER,
		SPEAKER_SIDE_LEFT,
		SPEAKER_SIDE_RIGHT,
		SPEAKER_TOP_CENTER,
		SPEAKER_TOP_FRONT_LEFT,
		SPEAKER_TOP_FRONT_CENTER,
		SPEAKER_TOP_FRONT_RIGHT,
		SPEAKER_TOP_BACK_LEFT,
		SPEAKER_TOP_BACK_CENTER,
		SPEAKER_TOP_BACK_RIGHT,
	};

	if (size >= 10) {
		uint8_t  Codec          = buf[0] & 0x3F;
		uint16_t Profile        = *(uint16_t*)&buf[0] >> 6 & 0xF;

		uint8_t  FrameSizeType  = buf[1] >> 2 & 0xF;
		uint64_t SampleNum      = *(uint64_t*)&buf[1] >> 6 & 0x1FFFFFFFFF;

		uint8_t  DataType       = (buf[6] >> 1 & 0x7); // 0 - PCM
		uint32_t SampleRate     = (*(uint32_t*)&buf[6] >> 4 & 0x3FFFF) + 6000;
		uint16_t SampleBits     = (*(uint16_t*)&buf[8] >> 6 & 0x1F) + 8;
		uint8_t  ChannelNum     = (buf[9] >> 3 & 0xF) + 1;
		uint8_t  HasExtension   = buf[9] >> 7;

		uint8_t ValidBitsPerSample      = 0;
		uint8_t HasSpeakerAssignment    = 0;
		DWORD   ChannelMask             = 0;
		if (HasExtension && size >= 11) {
			ValidBitsPerSample      = (buf[10] & 0x1F) + 1;
			HasSpeakerAssignment    = (buf[10] >> 5 & 0x1);
			if (HasSpeakerAssignment && size >= (80 + ChannelNum * 6 + 7) / 8) {
				int ch = 1;
				int bytepos = 10;
				do {
					ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[*(uint16_t*)&buf[bytepos + 0] >> 6 & 0x3F] : 0;
					ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[*(uint16_t*)&buf[bytepos + 1] >> 4 & 0x3F] : 0;
					ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[buf[bytepos + 2] >> 2 & 0x3F]              : 0;
					ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[buf[bytepos + 3] & 0x3F]                   : 0;
					bytepos += 3;
				} while (ch <= ChannelNum);
			}
		}

		int nb_samples, max_nb_samples;
		if (FrameSizeType <= TAK_FRAME_250ms) {
			nb_samples     = SampleRate * frame_duration_type_quants[FrameSizeType] >> 5;
			max_nb_samples = 16384;
		} else if (FrameSizeType < _countof(frame_duration_type_quants)) {
			nb_samples     = frame_duration_type_quants[FrameSizeType];
			max_nb_samples = SampleRate * frame_duration_type_quants[TAK_FRAME_250ms] >> 5;
		} else {
			nb_samples     = 0;
			max_nb_samples = 0;
		}

		si.codec_type   = (TAKCodecType)Codec;
		si.frame_size   = nb_samples;
		si.data_type    = DataType;
		si.sample_rate  = SampleRate;
		si.channels     = ChannelNum;
		si.bps          = SampleBits;
		si.ch_layout    = ChannelMask;
		si.samples      = SampleNum;
	}
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
