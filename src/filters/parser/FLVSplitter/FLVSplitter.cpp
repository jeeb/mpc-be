/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
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
#include "FLVSplitter.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/VideoParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#define FLV_AUDIODATA     8
#define FLV_VIDEODATA     9
#define FLV_SCRIPTDATA    18

#define FLV_AUDIO_PCM     0 // Linear PCM, platform endian
#define FLV_AUDIO_ADPCM   1 // ADPCM
#define FLV_AUDIO_MP3     2 // MP3
#define FLV_AUDIO_PCMLE   3 // Linear PCM, little endian
#define FLV_AUDIO_NELLY16 4 // Nellymoser 16 kHz mono
#define FLV_AUDIO_NELLY8  5 // Nellymoser 8 kHz mono
#define FLV_AUDIO_NELLY   6 // Nellymoser
// 7 = G.711 A-law logarithmic PCM (reserved)
// 8 = G.711 mu-law logarithmic PCM (reserved)
// 9 = reserved
#define FLV_AUDIO_AAC     10 // AAC
#define FLV_AUDIO_SPEEX   11 // Speex
// 14 = MP3 8 kHz (reserved)
// 15 = Device-specific sound (reserved)

//#define FLV_VIDEO_JPEG    1 // non-standard? need samples
#define FLV_VIDEO_H263    2 // Sorenson H.263
#define FLV_VIDEO_SCREEN  3 // Screen video
#define FLV_VIDEO_VP6     4 // On2 VP6
#define FLV_VIDEO_VP6A    5 // On2 VP6 with alpha channel
#define FLV_VIDEO_SCREEN2 6 // Screen video version 2
#define FLV_VIDEO_AVC     7 // AVC

#define AMF_DATA_TYPE_STRING		0x02
#define AMF_DATA_TYPE_NUMBER		0x00
#define AMF_DATA_TYPE_MIXEDARRAY	0x08


#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_FLV},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut2[] = {
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CFLVSplitterFilter), FlvSplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CFLVSourceFilter), FlvSourceName, MERIT_NORMAL, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CFLVSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CFLVSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".flv"));

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_FLV, _T("0,4,,464C5601"), NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_FLV);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../core/FilterApp.h"

CFilterApp theApp;

#endif

//
// CFLVSplitterFilter
//

CFLVSplitterFilter::CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CFLVSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_TimeStampOffset(0)
	, m_DetectWrongTimeStamp(true)
{
	m_nFlag |= PACKET_PTS_DISCONTINUITY;
}

STDMETHODIMP CFLVSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, FlvSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

bool CFLVSplitterFilter::ReadTag(Tag& t)
{
	if (FAILED(m_pFile->WaitAvailable(1000, 15))) {
		//return false;
	}

	t.PreviousTagSize	= (UINT32)m_pFile->BitRead(32);
	t.TagType			= (BYTE)m_pFile->BitRead(8);
	t.DataSize			= (UINT32)m_pFile->BitRead(24);
	t.TimeStamp			= (UINT32)m_pFile->BitRead(24);
	t.TimeStamp		   |= (UINT32)m_pFile->BitRead(8) << 24;
	t.StreamID			= (UINT32)m_pFile->BitRead(24);

	if (m_DetectWrongTimeStamp && (t.TagType == FLV_AUDIODATA || t.TagType == FLV_VIDEODATA)) {
		if (t.TimeStamp > 0) {
			m_TimeStampOffset = t.TimeStamp;
		}
		m_DetectWrongTimeStamp = false;
	}

	if (m_TimeStampOffset > 0) {
		t.TimeStamp -= m_TimeStampOffset;
		DbgLog((LOG_TRACE, 3, L"CFLVSplitterFilter::ReadTag() : Detect wrong TimeStamp offset, corrected [%d -> %d]",  (t.TimeStamp + m_TimeStampOffset), t.TimeStamp));
	}

	return m_pFile->IsRandomAccess() ? (m_pFile->GetRemaining() >= t.DataSize) : true;
}

bool CFLVSplitterFilter::ReadTag(AudioTag& at)
{
	if (FAILED(m_pFile->WaitAvailable(1000))) {
		//return false;
	}

	at.SoundFormat = (BYTE)m_pFile->BitRead(4);
	at.SoundRate = (BYTE)m_pFile->BitRead(2);
	at.SoundSize = (BYTE)m_pFile->BitRead(1);
	at.SoundType = (BYTE)m_pFile->BitRead(1);

	return true;
}

bool CFLVSplitterFilter::ReadTag(VideoTag& vt)
{
	if (FAILED(m_pFile->WaitAvailable(1000))) {
		//return false;
	}

	vt.FrameType = (BYTE)m_pFile->BitRead(4);
	vt.CodecID = (BYTE)m_pFile->BitRead(4);

	return true;
}

#ifndef NOVIDEOTWEAK
bool CFLVSplitterFilter::ReadTag(VideoTweak& vt)
{
	if (FAILED(m_pFile->WaitAvailable(1000))) {
		//return false;
	}

	vt.x = (BYTE)m_pFile->BitRead(4);
	vt.y = (BYTE)m_pFile->BitRead(4);

	return true;
}
#endif

bool CFLVSplitterFilter::Sync(__int64& pos)
{
	m_pFile->Seek(pos);

	while (m_pFile->GetRemaining(true) >= 15) {
		__int64 limit = m_pFile->GetRemaining(true);
		while (true) {
			BYTE b = (BYTE)m_pFile->BitRead(8);
			if (b == FLV_AUDIODATA || b == FLV_VIDEODATA) {
				break;
			}
			if (--limit < 15) {
				return false;
			}
		}

		pos = m_pFile->GetPos() - 5;
		m_pFile->Seek(pos);

		Tag ct;
		if (ReadTag(ct)) {
			__int64 next = m_pFile->GetPos() + ct.DataSize;
			if (next == m_pFile->GetAvailable() - 4) {
				m_pFile->Seek(pos);
				return true;
			} else if (next <= m_pFile->GetAvailable() - 19) {
				m_pFile->Seek(next);
				Tag nt;
				if (ReadTag(nt) && (nt.TagType == FLV_AUDIODATA || nt.TagType == FLV_VIDEODATA || nt.TagType == FLV_SCRIPTDATA)) {
					if ((nt.PreviousTagSize == ct.DataSize + 11) ||
							(m_IgnorePrevSizes &&
							 nt.TimeStamp >= ct.TimeStamp &&
							 nt.TimeStamp - ct.TimeStamp <= 1000)) {
						m_pFile->Seek(pos);
						return true;
					}
				}
			}
		}

		m_pFile->Seek(pos + 5);
	}

	return false;
}

static double int64toDouble(__int64 value)
{
	union
	{
		__int64	i;
		double	f;
	} intfloat64;
	
	intfloat64.i = value;

	return intfloat64.f;
}

HRESULT CFLVSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr, false, true, true));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	if (m_pFile->BitRead(24) != 'FLV' || m_pFile->BitRead(8) != 1) {
		return E_FAIL;
	}

	EXECUTE_ASSERT(m_pFile->BitRead(5) == 0); // TypeFlagsReserved
	bool fTypeFlagsAudio = !!m_pFile->BitRead(1);
	EXECUTE_ASSERT(m_pFile->BitRead(1) == 0); // TypeFlagsReserved
	bool fTypeFlagsVideo = !!m_pFile->BitRead(1);
	m_DataOffset = (UINT32)m_pFile->BitRead(32);

	// doh, these flags aren't always telling the truth
	fTypeFlagsAudio = fTypeFlagsVideo = true;

	Tag t;
	AudioTag at;
	VideoTag vt;

	UINT32 prevTagSize = 0;
	m_IgnorePrevSizes = false;

	m_pFile->Seek(m_DataOffset);

	REFERENCE_TIME AvgTimePerFrame	= 0;
	REFERENCE_TIME metaDataDuration	= 0;

	for (int i = 0; ReadTag(t) && (fTypeFlagsVideo || fTypeFlagsAudio); i++) {
		if (!t.DataSize) continue; // skip empty Tag

		UINT64 next = m_pFile->GetPos() + t.DataSize;

		CStringW name;

		CMediaType mt;
		mt.subtype = GUID_NULL;

		if (i != 0 && t.PreviousTagSize != prevTagSize) {
			m_IgnorePrevSizes = true;
		}
		prevTagSize = t.DataSize + 11;

		if (t.TagType == FLV_SCRIPTDATA && t.DataSize) {
			BYTE type = m_pFile->BitRead(8);
			SHORT length = m_pFile->BitRead(16);
			if (type == AMF_DATA_TYPE_STRING && length <= 11) {
				char name[256];
				memset(name, 0, 256);
				m_pFile->ByteRead((BYTE*)name, length);
				if (!strncmp(name, "onTextData", length) || (!strncmp(name, "onMetaData", length))) {

					BYTE amf_type = m_pFile->BitRead(8);
					if (amf_type == AMF_DATA_TYPE_MIXEDARRAY) {
						m_pFile->BitRead(32); // skip 32-bit max array index

						length = m_pFile->BitRead(16);
						memset(name, 0, 256);
						m_pFile->ByteRead((BYTE*)name, length);
						if (!strncmp(name, "duration", length)) {
							amf_type = m_pFile->BitRead(8);
							if (amf_type == AMF_DATA_TYPE_NUMBER) {
								metaDataDuration = UNITS * (REFERENCE_TIME)int64toDouble(m_pFile->BitRead(64));
							}
						}
					}
				}
			}
		} else if (t.TagType == FLV_AUDIODATA && t.DataSize != 0 && fTypeFlagsAudio) {
			UNREFERENCED_PARAMETER(at);
			AudioTag at;
			name = L"Audio";

			if (ReadTag(at)) {
				int dataSize = t.DataSize - 1;

				fTypeFlagsAudio = false;

				mt.majortype			= MEDIATYPE_Audio;
				mt.formattype			= FORMAT_WaveFormatEx;
				WAVEFORMATEX* wfe		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(wfe, 0, sizeof(WAVEFORMATEX));
				wfe->nSamplesPerSec		= 44100*(1<<at.SoundRate)/8;
				wfe->wBitsPerSample		= 8*(at.SoundSize+1);
				wfe->nChannels			= at.SoundType+1;

				switch (at.SoundFormat) {
					case FLV_AUDIO_PCM:
					case FLV_AUDIO_PCMLE:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_PCM);
						name += L" PCM";
						break;
					case FLV_AUDIO_ADPCM:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_ADPCM_SWF);
						name += L" ADPCM";
						break;
					case FLV_AUDIO_MP3:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MPEGLAYER3);
						name += L" MP3";
						{
							CBaseSplitterFileEx::mpahdr h;
							CMediaType mt2;
							if (m_pFile->Read(h, 4, false, &mt2)) {
								mt = mt2;
							}
						}
						break;
					case FLV_AUDIO_NELLY16:
						mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
						wfe->nSamplesPerSec = 16000;
						name += L" Nellimoser";
						break;
					case FLV_AUDIO_NELLY8:
						mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
						wfe->nSamplesPerSec = 8000;
						name += L" Nellimoser";
						break;
					case FLV_AUDIO_NELLY:
						mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
						name += L" Nellimoser";
						break;
					case FLV_AUDIO_AAC: {
						if (dataSize < 1 || m_pFile->BitRead(8) != 0) { // packet type 0 == aac header
							fTypeFlagsAudio = true;
							break;
						}
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_AAC);
						name += L" AAC";

						__int64 configOffset = m_pFile->GetPos();
						UINT32 configSize = dataSize - 1;
						if (configSize < 2) {
							break;
						}

						// Might break depending on the AAC profile, see ff_mpeg4audio_get_config in ffmpeg's mpeg4audio.c
						m_pFile->BitRead(5);
						int iSampleRate = (int)m_pFile->BitRead(4);
						int iChannels   = (int)m_pFile->BitRead(4);
						if (iSampleRate > 12 || iChannels > 7) {
							break;
						}

						const int sampleRates[] = {
							96000, 88200, 64000, 48000, 44100, 32000, 24000,
							22050, 16000, 12000, 11025, 8000, 7350
						};
						const int channels[] = {
							0, 1, 2, 3, 4, 5, 6, 8
						};

						wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + configSize);
						memset(wfe, 0, mt.FormatLength());
						wfe->wFormatTag		= WAVE_FORMAT_AAC;
						wfe->nSamplesPerSec = sampleRates[iSampleRate];
						wfe->wBitsPerSample = 16;
						wfe->nChannels      = channels[iChannels];
						wfe->cbSize         = configSize;

						m_pFile->Seek(configOffset);
						m_pFile->ByteRead((BYTE*)(wfe+1), configSize);
					}
				}
				wfe->nBlockAlign		= wfe->nChannels * wfe->wBitsPerSample / 8;
				wfe->nAvgBytesPerSec	= wfe->nSamplesPerSec * wfe->nBlockAlign;

				mt.SetSampleSize(wfe->wBitsPerSample * wfe->nChannels / 8);
			}
		} else if (t.TagType == FLV_VIDEODATA && t.DataSize != 0 && fTypeFlagsVideo) {
			UNREFERENCED_PARAMETER(vt);
			VideoTag vt;
			if (ReadTag(vt) && vt.FrameType == 1) {
				int dataSize = t.DataSize - 1;

				fTypeFlagsVideo = false;
				name = L"Video";

				mt.majortype = MEDIATYPE_Video;
				mt.formattype = FORMAT_VideoInfo;
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
				memset(vih, 0, sizeof(VIDEOINFOHEADER));

				BITMAPINFOHEADER* bih = &vih->bmiHeader;

				int w, h, arx, ary;

				// calculate video fps
				if (!AvgTimePerFrame) {
					__int64 pos = m_pFile->GetPos();
					__int64 sync_pos = m_DataOffset;
					if (Sync(sync_pos)) {
						Tag tag;
						VideoTag vtag;
						UINT32 first_ts, current_ts;
						first_ts = current_ts = 0;
						int frame_cnt = 0;

						while ((frame_cnt < 30) && ReadTag(tag) && !CheckRequest(NULL) && m_pFile->GetRemaining(true)) {
							__int64 _next = m_pFile->GetPos() + tag.DataSize;

							if ((tag.DataSize > 0) && (tag.TagType == FLV_VIDEODATA && ReadTag(vtag))) {

								if (tag.TimeStamp != current_ts) {
									frame_cnt++;
								}

								current_ts = tag.TimeStamp;

								if (!frame_cnt) {
									first_ts = current_ts;
								}
							}
							m_pFile->Seek(_next);
						}

						AvgTimePerFrame = 10000 * (current_ts - first_ts)/frame_cnt;
					}

					m_pFile->Seek(pos);
				}

				vih->AvgTimePerFrame = AvgTimePerFrame;

				switch (vt.CodecID) {
					case FLV_VIDEO_H263:   // H.263
						if (m_pFile->BitRead(17) != 1) {
							break;
						}

						m_pFile->BitRead(13); // Version (5), TemporalReference (8)

						switch (BYTE PictureSize = (BYTE)m_pFile->BitRead(3)) { // w00t
							case 0:
							case 1:
								vih->bmiHeader.biWidth = (WORD)m_pFile->BitRead(8*(PictureSize+1));
								vih->bmiHeader.biHeight = (WORD)m_pFile->BitRead(8*(PictureSize+1));
								break;
							case 2:
							case 3:
							case 4:
								vih->bmiHeader.biWidth = 704 / PictureSize;
								vih->bmiHeader.biHeight = 576 / PictureSize;
								break;
							case 5:
							case 6:
								PictureSize -= 3;
								vih->bmiHeader.biWidth = 640 / PictureSize;
								vih->bmiHeader.biHeight = 480 / PictureSize;
								break;
						}

						if (!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight) {
							break;
						}

						mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = '1VLF');
						name += L" H.263";

						break;
					case FLV_VIDEO_SCREEN: {
						m_pFile->BitRead(4);
						vih->bmiHeader.biWidth  = (LONG)m_pFile->BitRead(12);
						m_pFile->BitRead(4);
						vih->bmiHeader.biHeight = (LONG)m_pFile->BitRead(12);

						if (!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight) {
							break;
						}

						vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
						vih->bmiHeader.biPlanes = 1;
						vih->bmiHeader.biBitCount = 24;
						vih->bmiHeader.biSizeImage = vih->bmiHeader.biWidth * vih->bmiHeader.biHeight * 3;

						mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = '1VSF');
						name += L" Screen";

						break;
					}
					case FLV_VIDEO_VP6A:  // VP6 with alpha
						m_pFile->BitRead(24);
					case FLV_VIDEO_VP6: { // VP6
#ifdef NOVIDEOTWEAK
						m_pFile->BitRead(8);
#else
						VideoTweak fudge;
						ReadTag(fudge);
#endif

						if (m_pFile->BitRead(1)) {
							// Delta (inter) frame
							fTypeFlagsVideo = true;
							break;
						}
						m_pFile->BitRead(6);
						bool fSeparatedCoeff = !!m_pFile->BitRead(1);
						m_pFile->BitRead(5);
						int filterHeader = (int)m_pFile->BitRead(2);
						m_pFile->BitRead(1);
						if (fSeparatedCoeff || !filterHeader) {
							m_pFile->BitRead(16);
						}

						h = (int)m_pFile->BitRead(8) * 16;
						w = (int)m_pFile->BitRead(8) * 16;

						ary = (int)m_pFile->BitRead(8) * 16;
						arx = (int)m_pFile->BitRead(8) * 16;

						if (arx && arx != w || ary && ary != h) {
							VIDEOINFOHEADER2* vih2		= (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
							memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
							vih2->dwPictAspectRatioX	= arx;
							vih2->dwPictAspectRatioY	= ary;
							vih2->AvgTimePerFrame		= AvgTimePerFrame;
							bih = &vih2->bmiHeader;
							mt.formattype = FORMAT_VideoInfo2;
							vih = (VIDEOINFOHEADER *)vih2;
						}

						bih->biWidth = w;
						bih->biHeight = h;
#ifndef NOVIDEOTWEAK
						SetRect(&vih->rcSource, 0, 0, w - fudge.x, h - fudge.y);
						SetRect(&vih->rcTarget, 0, 0, w - fudge.x, h - fudge.y);
#endif

						mt.subtype = FOURCCMap(bih->biCompression = '4VLF');
						name += L" VP6";

						break;
					}
					case FLV_VIDEO_AVC: { // H.264
						if (dataSize < 4 || m_pFile->BitRead(8) != 0) { // packet type 0 == avc header
							fTypeFlagsVideo = true;
							break;
						}
						m_pFile->BitRead(24); // composition time

						__int64 headerOffset = m_pFile->GetPos();
						UINT32 headerSize = dataSize - 4;
						BYTE *headerData = DNew BYTE[headerSize];

						m_pFile->ByteRead(headerData, headerSize);

						CGolombBuffer gb(headerData + 9, headerSize - 9);
						avc_hdr h;
						if (!ParseAVCHeader(gb, h)) {
							break;
						}

						BITMAPINFOHEADER pbmi;
						memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
						pbmi.biSize			= sizeof(pbmi);
						pbmi.biWidth		= h.width;
						pbmi.biHeight		= h.height;
						pbmi.biCompression	= '1CVA';
						pbmi.biPlanes		= 1;
						pbmi.biBitCount		= 24;

						CSize aspect(h.width * h.sar.num, h.height * h.sar.den);
						int lnko = LNKO(aspect.cx, aspect.cy);
						if (lnko > 1) {
							aspect.cx /= lnko, aspect.cy /= lnko;
						}

						CreateMPEG2VIfromAVC(&mt, &pbmi, AvgTimePerFrame, aspect, headerData, headerSize); 

						delete[] headerData;

						name += L" H.264";

						break;
					}
					default:
						fTypeFlagsVideo = true;
				}
			}
		}

		if (mt.subtype != GUID_NULL) {
			CAtlArray<CMediaType> mts;
			mts.Add(mt);
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, name, this, this, &hr));
			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(t.TagType, pPinOut)));
		}

		m_pFile->Seek(next);
	}

	m_rtDuration = metaDataDuration;
	m_rtNewStop = m_rtStop = m_rtDuration;

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CFLVSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CFLVSplitterFilter");

	if (m_pFile->IsRandomAccess()) {
		__int64 pos = max(m_DataOffset, m_pFile->GetAvailable() - 256 * 1024);
		REFERENCE_TIME rtDuration = 0;
		
		if (Sync(pos)) {
			Tag t;
			AudioTag at;
			VideoTag vt;

			while (ReadTag(t) && m_pFile->GetRemaining()) {
				UINT64 next = m_pFile->GetPos() + t.DataSize;

				if ((t.TagType == FLV_AUDIODATA && ReadTag(at)) || (t.TagType == FLV_VIDEODATA && ReadTag(vt))) {
					rtDuration = max(rtDuration, 10000i64 * t.TimeStamp);
				}

				m_pFile->Seek(next);
			}
		}

		if (rtDuration) {
			m_rtDuration = rtDuration;
		}
	}

	return true;
}

void CFLVSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (!m_rtDuration || rt <= 0) {
		m_pFile->Seek(m_DataOffset);
	} else if (!m_IgnorePrevSizes) {
		NormalSeek(rt);
	} else {
		AlternateSeek(rt);
	}
}

void CFLVSplitterFilter::NormalSeek(REFERENCE_TIME rt)
{
	bool fAudio = !!GetOutputPin(FLV_AUDIODATA);
	bool fVideo = !!GetOutputPin(FLV_VIDEODATA);

	__int64 pos = m_DataOffset + (__int64)(double(m_pFile->GetLength() - m_DataOffset) * rt / m_rtDuration);

	if (pos > m_pFile->GetAvailable()) {
		return;
	}

	if (!Sync(pos)) {
		m_pFile->Seek(m_DataOffset);
		return;
	}

	Tag t;
	AudioTag at;
	VideoTag vt;

	while (ReadTag(t)) {
		pos = m_pFile->GetPos() + t.DataSize;

		CBaseSplitterOutputPin* pOutPin = dynamic_cast<CBaseSplitterOutputPin*>(GetOutputPin(t.TagType));
		if (!pOutPin) {
			m_pFile->Seek(pos);
			continue;
		}

		t.TimeStamp += (pOutPin->GetOffset() / 10000i64);

		if (10000i64 * t.TimeStamp >= rt) {
			m_pFile->Seek(m_pFile->GetPos() - 15);
			break;
		}

		m_pFile->Seek(pos);
	}

	while (m_pFile->GetPos() >= m_DataOffset && (fAudio || fVideo) && ReadTag(t)) {
		UINT64 prev = m_pFile->GetPos() - 15 - t.PreviousTagSize - 4;

		CBaseSplitterOutputPin* pOutPin = dynamic_cast<CBaseSplitterOutputPin*>(GetOutputPin(t.TagType));
		if (!pOutPin) {
			m_pFile->Seek(prev);
			continue;
		}

		t.TimeStamp += (pOutPin->GetOffset() / 10000i64);

		if (10000i64 * t.TimeStamp <= rt) {
			if (t.TagType == FLV_AUDIODATA && ReadTag(at)) {
				fAudio = false;
			} else if (t.TagType == FLV_VIDEODATA && ReadTag(vt) && vt.FrameType == 1) {
				fVideo = false;
			}
		}

		m_pFile->Seek(prev);
	}

	if (fAudio || fVideo) {
		m_pFile->Seek(m_DataOffset);
	}
}

void CFLVSplitterFilter::AlternateSeek(REFERENCE_TIME rt)
{
	bool hasAudio = !!GetOutputPin(FLV_AUDIODATA);
	bool hasVideo = !!GetOutputPin(FLV_VIDEODATA);

	__int64 estimPos = m_DataOffset + (__int64)(double(m_pFile->GetAvailable() - m_DataOffset) * rt / m_rtDuration);

	while (true) {
		estimPos -= 256 * 1024;
		if (estimPos < m_DataOffset) estimPos = m_DataOffset;

		bool foundAudio = !hasAudio;
		bool foundVideo = !hasVideo;
		__int64 bestPos = estimPos;

		if (Sync(bestPos)) {
			Tag t;
			AudioTag at;
			VideoTag vt;

			while (ReadTag(t) && t.TimeStamp * 10000i64 < rt) {
				__int64 cur = m_pFile->GetPos() - 15;
				__int64 next = cur + 15 + t.DataSize;

				if (hasAudio && t.TagType == FLV_AUDIODATA && ReadTag(at)) {
					foundAudio = true;
					if (!hasVideo) {
						bestPos = cur;
					}
				} else if (hasVideo && t.TagType == FLV_VIDEODATA && ReadTag(vt) && vt.FrameType == 1) {
					foundVideo = true;
					bestPos = cur;
				}

				m_pFile->Seek(next);
			}
		}

		if (foundAudio && foundVideo) {
			m_pFile->Seek(bestPos);
			return;
		} else if (estimPos == m_DataOffset) {
			m_pFile->Seek(m_DataOffset);
			return;
		}
	}
}

bool CFLVSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	CAutoPtr<Packet> p;

	Tag t;
	AudioTag at = {};
	VideoTag vt = {};

	while (SUCCEEDED(hr) && !CheckRequest(NULL)) {

		if (!ReadTag(t)) {
			break;
		}

		if (!m_pFile->GetRemaining(m_pFile->IsStreaming())) {
			m_pFile->WaitAvailable(1500, t.DataSize);

			if (!m_pFile->GetRemaining(m_pFile->IsStreaming())) {
				break;
			}
		}

		__int64 next = m_pFile->GetPos() + t.DataSize;

		if ((t.DataSize > 0) && (t.TagType == FLV_AUDIODATA && ReadTag(at) || t.TagType == FLV_VIDEODATA && ReadTag(vt))) {
			UINT32 tsOffset = 0;
			if (t.TagType == FLV_VIDEODATA) {
				if (vt.FrameType == 5) {
					goto NextTag;    // video info/command frame
				}
				if (vt.CodecID == FLV_VIDEO_VP6) {
					m_pFile->BitRead(8);
				} else if (vt.CodecID == FLV_VIDEO_VP6A) {
					m_pFile->BitRead(32);
				} else if (vt.CodecID == FLV_VIDEO_AVC) {
					if (m_pFile->BitRead(8) != 1) {
						goto NextTag;
					}
					// Tag timestamps specify decode time, this is the display time offset
					tsOffset = (UINT32)m_pFile->BitRead(24);
					tsOffset = (tsOffset + 0xff800000) ^ 0xff800000; // sign extension
				}
			}
			if (t.TagType == FLV_AUDIODATA && at.SoundFormat == FLV_AUDIO_AAC) {
				if (m_pFile->BitRead(8) != 1) {
					goto NextTag;
				}
			}
			__int64 dataSize = next - m_pFile->GetPos();
			if (dataSize <= 0) {
				goto NextTag;
			}

			m_pFile->WaitAvailable(1500, dataSize);
			
			p.Attach(DNew Packet());
			p->TrackNumber	= t.TagType;
			p->rtStart		= 10000i64 * (t.TimeStamp + tsOffset);
			p->rtStop		= p->rtStart + 1;
			p->bSyncPoint	= t.TagType == FLV_VIDEODATA ? vt.FrameType == 1 : true;

			p->SetCount((size_t)dataSize);
			m_pFile->ByteRead(p->GetData(), p->GetCount());

			hr = DeliverPacket(p);
		}
NextTag:
		m_pFile->Seek(next);
	}

	return true;
}

//
// CFLVSourceFilter
//

CFLVSourceFilter::CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CFLVSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
