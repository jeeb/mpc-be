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

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
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

		BYTE* buf = NULL;
		CGolombBuffer gb(NULL, 0);

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

					gb.Reset(buf, size - 3);

					//HexDump(L"", buf, size - 3);
				}
				break;
			case TAK_METADATA_END:
				{
					//m_endpos += m_pFile->GetPos();

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
								if (SUCCEEDED(m_pFile->ByteRead(p, tag_size))) {
									if (APETag->ReadTags(p, tag_size)) {
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
		}

		if (type == TAK_METADATA_STREAMINFO) {
			TAKStreamInfo si = {0};
			ParseTAKStreamInfo(gb, si);

			// fake ... for test, work on STEREO
			CMediaType mt;
			mt.majortype			= MEDIATYPE_Audio;
			mt.formattype			= FORMAT_WaveFormatEx;
			mt.subtype				= MEDIASUBTYPE_TAK;
			WAVEFORMATEX* wfe		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + size);
			memset(wfe, 0, sizeof(WAVEFORMATEX));
			wfe->nSamplesPerSec		= si.sample_rate;
			wfe->wBitsPerSample		= 16;
			wfe->nChannels			= si.channels;

			wfe->nBlockAlign		= wfe->nChannels * wfe->wBitsPerSample / 8;
			wfe->nAvgBytesPerSec	= wfe->nSamplesPerSec * wfe->nBlockAlign;
			mt.SetSampleSize(/*wfe->wBitsPerSample * wfe->nChannels / 8*/256000);

			wfe->cbSize				= size;
			memcpy(wfe + 1, buf, size);

			if (mt.subtype != GUID_NULL) {
				CAtlArray<CMediaType> mts;
				mts.Add(mt);
				CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Tak Audio Output", this, this, &hr));
				EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
			}

			m_nAvgBytesPerSec		= wfe->nAvgBytesPerSec;
			m_rtDuration			= si.samples;
		} else if (type == TAK_METADATA_LAST_FRAME) {
			//bLastFrame	= TRUE;
			//m_endpos	= gb.BitRead(TAK_LAST_FRAME_POS_BITS) + gb.BitRead(TAK_LAST_FRAME_SIZE_BITS);
		}

		SAFE_DELETE_ARRAY(buf);
	}

	if (!bLastFrame) {
		m_endpos = m_pFile->GetLength();
	}

	m_pFile->Seek(0);

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
		m_pFile->Seek(0);
		m_rtStart = 0;
	} else {
		m_pFile->Seek((__int64)((1.0 * rt / m_rtDuration) * (m_endpos)));
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

static int tak_get_nb_samples(int sample_rate, enum TAKFrameSizeType type)
{
    int nb_samples, max_nb_samples;

    if (type <= TAK_FST_250ms) {
        nb_samples     = sample_rate * frame_duration_type_quants[type] >>
                         TAK_FRAME_DURATION_QUANT_SHIFT;
        max_nb_samples = 16384;
    } else if (type < _countof(frame_duration_type_quants)) {
        nb_samples     = frame_duration_type_quants[type];
        max_nb_samples = sample_rate *
                         frame_duration_type_quants[TAK_FST_250ms] >>
                         TAK_FRAME_DURATION_QUANT_SHIFT;
    } else {
        return 0;
    }

    if (nb_samples <= 0 || nb_samples > max_nb_samples)
        return 0;

    return nb_samples;
}

void CTAKSplitterFilter::ParseTAKStreamInfo(CGolombBuffer gb, TAKStreamInfo& si)
{
	// this code work but only mono/stereo information
	gb.BitRead(8);

	int num_samples_lo		= gb.BitRead(2);
	int framesizecode		= gb.BitRead(3);
	gb.BitRead(2);

	gb.BitByteAlign();
	UINT32 num_samples_hi	= gb.BitRead(32);
	BSWAP32(num_samples_hi);
	UINT32 sample_rate		= gb.BitRead(24);
	BSWAP24(sample_rate);

	gb.BitByteAlign();
	gb.BitRead(4);
	int channels			= gb.BitRead(1);
	int samplesize			= gb.BitRead(2);
	gb.BitRead(1);

	gb.BitByteAlign();
	gb.BitRead(24);

	UINT64 Samples			= ((UINT64)num_samples_hi)<<2 | num_samples_lo;
    UINT SamplingRate		= (sample_rate >> 4) + 6000;

	si.channels				= channels ? 2 : 1;
	si.sample_rate			= SamplingRate;
	si.samples				= (Samples / SamplingRate) * UNITS;

	/*
	UINT64 channel_mask = 0;

	si.codec = (TAKCodecType)gb.BitRead(TAK_ENCODER_CODEC_BITS);
	gb.BitRead(TAK_ENCODER_PROFILE_BITS);

	enum TAKFrameSizeType frame_type = (TAKFrameSizeType)gb.BitRead(TAK_SIZE_FRAME_DURATION_BITS);
	si.samples = gb.BitRead(TAK_SIZE_SAMPLES_NUM_BITS);

	si.data_type = gb.BitRead(TAK_FORMAT_DATA_TYPE_BITS);
	si.sample_rate = gb.BitRead(TAK_FORMAT_SAMPLE_RATE_BITS) + TAK_SAMPLE_RATE_MIN;
	si.bps = gb.BitRead(TAK_FORMAT_BPS_BITS) + TAK_BPS_MIN;
	si.channels = gb.BitRead(TAK_FORMAT_CHANNEL_BITS) + TAK_CHANNELS_MIN;

	if (gb.BitRead(1)) {
		gb.BitRead(TAK_FORMAT_VALID_BITS);
		if (gb.BitRead(1)) {
			for (int i = 0; i < si.channels; i++) {
				int value = gb.BitRead(TAK_FORMAT_CH_LAYOUT_BITS);

				if (value < _countof(tak_channel_layouts)) {
					channel_mask |= tak_channel_layouts[value];
				}
			}
		}
	}

    si.ch_layout     = channel_mask;
    si.frame_samples = tak_get_nb_samples(si.sample_rate, frame_type);
	*/
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
