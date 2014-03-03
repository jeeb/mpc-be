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
#include <MMReg.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <dmodshow.h>
#include "MpegSplitter.h"
#include <moreuuids.h>
#include <basestruct.h>
#include <atlpath.h>

#include "../../../apps/mplayerc/SettingsDefines.h"
#include "../../reader/VTSReader/VTSReader.h"

// option names
#define OPT_REGKEY_MPEGSplit  _T("Software\\MPC-BE Filters\\MPEG Splitter")
#define OPT_SECTION_MPEGSplit _T("Filters\\MPEG Splitter")
#define OPT_ForcedSub         _T("ForcedSub")
#define OPT_AudioLangOrder    _T("AudioLanguageOrder")
#define OPT_SubLangOrder      _T("SubtitlesLanguageOrder")
#define OPT_AC3CoreOnly       _T("AC3CoreOnly")
#define OPT_AltDuration       _T("AlternativeDuration")
#define OPT_SubEmptyOutput    _T("SubtitleEmptyOutput")

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PROGRAM},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PVA},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpegSplitterFilter), MpegSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMpegSourceFilter), MpegSourceName, MERIT_UNLIKELY, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMpegSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMpegSourceFilter>, NULL, &sudFilter[1]},
	{L"CMpegSplitterPropertyPage", &__uuidof(CMpegSplitterSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpegSplitterSettingsWnd> >},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".ts"));

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG1System, _T("0,16,FFFFFFFFF100010001800001FFFFFFFF,000001BA2100010001800001000001BB"), NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PROGRAM, _T("0,5,FFFFFFFFC0,000001BA40"), NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PVA, _T("0,8,fffffc00ffe00000,4156000055000000"), NULL);

	CAtlList<CString> chkbytes;
	chkbytes.AddTail(_T("0,1,,47,188,1,,47,376,1,,47"));
	chkbytes.AddTail(_T("4,1,,47,196,1,,47,388,1,,47"));
	chkbytes.AddTail(_T("0,4,,54467263,1660,1,,47")); // TFrc

	chkbytes.AddTail(_T("0,8,,4D504C5330323030")); // MPLS0200
	chkbytes.AddTail(_T("0,8,,4D504C5330313030")); // MPLS0100
	chkbytes.AddTail(_T("0,4,,494D4B48"));			// IMKH

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_TRANSPORT, chkbytes, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

static CString GetMediaTypeDesc(const CMediaType *_pMediaType, const CHdmvClipInfo::Stream *pClipInfo, int _PresentationType, CStringA lang)
{
	const WCHAR *pPresentationDesc = NULL;

	if (pClipInfo) {
		pPresentationDesc = StreamTypeToName(pClipInfo->m_Type);
	} else {
		pPresentationDesc = StreamTypeToName((PES_STREAM_TYPE)_PresentationType);
	}

	CString MajorType;
	CAtlList<CString> Infos;

	if (_pMediaType->majortype == MEDIATYPE_Video) {
		MajorType = "Video";
		if (_pMediaType->subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| _pMediaType->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE
			|| _pMediaType->subtype == MEDIASUBTYPE_CVD_SUBPICTURE) {
			MajorType = "Subtitle";
		}

		if (pClipInfo) {
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty()) {
				Infos.AddTail(name);
			}
		} else {
			if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddTail(name);
				}
			}
		}

		const VIDEOINFOHEADER *pVideoInfo = NULL;
		const VIDEOINFOHEADER2 *pVideoInfo2 = NULL;

		if (_pMediaType->formattype == FORMAT_VideoInfo) {
			pVideoInfo = GetFormatHelper(pVideoInfo, _pMediaType);

			if (pVideoInfo->bmiHeader.biCompression == FCC('drac')) {
				Infos.AddTail(L"DIRAC");
			}

		} else if (_pMediaType->formattype == FORMAT_MPEGVideo) {
			Infos.AddTail(L"MPEG");

			const MPEG1VIDEOINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo = &pInfo->hdr;

		} else if (_pMediaType->formattype == FORMAT_MPEG2_VIDEO) {
			const MPEG2VIDEOINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo2 = &pInfo->hdr;

			bool bIsAVC = false;
			bool bIsMPEG2 = false;

			if (pInfo->hdr.bmiHeader.biCompression == FCC('AVC1') || pInfo->hdr.bmiHeader.biCompression == FCC('H264')) {
				bIsAVC = true;
				Infos.AddTail(L"AVC (H.264)");
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('AMVC')) {
				bIsAVC = true;
				Infos.AddTail(L"MVC (Full)");
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('EMVC')) {
				bIsAVC = true;
				Infos.AddTail(L"MVC (Subset)");
			} else if (pInfo->hdr.bmiHeader.biCompression == 0) {
				Infos.AddTail(L"MPEG2");
				bIsMPEG2 = true;
			} else {
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Temp[1] = (pInfo->hdr.bmiHeader.biCompression >> 8) & 0xFF;
				Temp[2] = (pInfo->hdr.bmiHeader.biCompression >> 16) & 0xFF;
				Temp[3] = (pInfo->hdr.bmiHeader.biCompression >> 24) & 0xFF;
				Infos.AddTail(Temp);
			}

			if (bIsMPEG2) {
				Infos.AddTail(MPEG2_Profile[pInfo->dwProfile]);
			} else if (pInfo->dwProfile) {
				if (bIsAVC) {
					switch (pInfo->dwProfile) {
						case 44:
							Infos.AddTail(L"CAVLC Profile");
							break;
						case 66:
							Infos.AddTail(L"Baseline Profile");
							break;
						case 77:
							Infos.AddTail(L"Main Profile");
							break;
						case 88:
							Infos.AddTail(L"Extended Profile");
							break;
						case 100:
							Infos.AddTail(L"High Profile");
							break;
						case 110:
							Infos.AddTail(L"High 10 Profile");
							break;
						case 118:
							Infos.AddTail(L"Multiview High Profile");
							break;
						case 122:
							Infos.AddTail(L"High 4:2:2 Profile");
							break;
						case 244:
							Infos.AddTail(L"High 4:4:4 Profile");
							break;
						case 128:
							Infos.AddTail(L"Stereo High Profile");
							break;
						default:
							Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
							break;
					}
				} else {
					Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
				}
			}

			if (bIsMPEG2) {
				Infos.AddTail(MPEG2_Level[pInfo->dwLevel]);
			} else if (pInfo->dwLevel) {
				if (bIsAVC) {
					Infos.AddTail(FormatString(L"Level %1.1f", double(pInfo->dwLevel)/10.0));
				} else {
					Infos.AddTail(FormatString(L"Level %d", pInfo->dwLevel));
				}
			}
		} else if (_pMediaType->formattype == FORMAT_VIDEOINFO2) {
			const VIDEOINFOHEADER2 *pInfo = GetFormatHelper(pInfo, _pMediaType);
			pVideoInfo2 = pInfo;

			DWORD CodecType = pInfo->bmiHeader.biCompression;
			if (CodecType == FCC('WVC1')) {
				Infos.AddTail(L"VC-1");
			} else if (CodecType) {
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (CodecType >> 0) & 0xFF;
				Temp[1] = (CodecType >> 8) & 0xFF;
				Temp[2] = (CodecType >> 16) & 0xFF;
				Temp[3] = (CodecType >> 24) & 0xFF;
				Infos.AddTail(Temp);
			}
		} else if (_pMediaType->subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
			Infos.AddTail(L"DVD Sub Picture");
		} else if (_pMediaType->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE) {
			Infos.AddTail(L"SVCD Sub Picture");
		} else if (_pMediaType->subtype == MEDIASUBTYPE_CVD_SUBPICTURE) {
			Infos.AddTail(L"CVD Sub Picture");
		}

		if (pVideoInfo2) {
			if (pVideoInfo2->bmiHeader.biWidth && pVideoInfo2->bmiHeader.biHeight) {
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo2->bmiHeader.biWidth, pVideoInfo2->bmiHeader.biHeight));
			}
			if (pVideoInfo2->AvgTimePerFrame) {
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo2->AvgTimePerFrame)));
			}
			if (pVideoInfo2->dwBitRate) {
				Infos.AddTail(FormatBitrate(pVideoInfo2->dwBitRate));
			}
		} else if (pVideoInfo) {
			if (pVideoInfo->bmiHeader.biWidth && pVideoInfo->bmiHeader.biHeight) {
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight));
			}
			if (pVideoInfo->AvgTimePerFrame) {
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo->AvgTimePerFrame)));
			}
			if (pVideoInfo->dwBitRate) {
				Infos.AddTail(FormatBitrate(pVideoInfo->dwBitRate));
			}
		}

	} else if (_pMediaType->majortype == MEDIATYPE_Audio) {
		MajorType = "Audio";
		if (pClipInfo) {
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty()) {
				Infos.AddTail(name);
			}
		} else {
			if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddTail(name);
				}
			}
		}
		if (_pMediaType->formattype == FORMAT_WaveFormatEx) {
			const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, _pMediaType);

			if (_pMediaType->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
				Infos.AddTail(L"DVD LPCM");
			} else if (_pMediaType->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
				const WAVEFORMATEX_HDMV_LPCM *pInfoHDMV = GetFormatHelper(pInfoHDMV, _pMediaType);
				UNREFERENCED_PARAMETER(pInfoHDMV);
				Infos.AddTail(L"HDMV LPCM");
			}
			if (_pMediaType->subtype == MEDIASUBTYPE_DOLBY_DDPLUS) {
				Infos.AddTail(L"Dolby Digital Plus");
			} else if (_pMediaType->subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
				Infos.AddTail(L"TrueHD");
			} else if (_pMediaType->subtype == MEDIASUBTYPE_MLP) {
				Infos.AddTail(L"MLP");
			} else {
				switch (pInfo->wFormatTag) {
					case WAVE_FORMAT_PS2_PCM: {
						Infos.AddTail(L"PS2 PCM");
					}
					break;
					case WAVE_FORMAT_PS2_ADPCM: {
						Infos.AddTail(L"PS2 ADPCM");
					}
					break;
					case WAVE_FORMAT_DTS2: {
						if (pPresentationDesc) {
							Infos.AddTail(pPresentationDesc);
						} else {
							Infos.AddTail(L"DTS");
						}
					}
					break;
					case WAVE_FORMAT_DOLBY_AC3: {
						if (pPresentationDesc) {
							Infos.AddTail(pPresentationDesc);
						} else {
							Infos.AddTail(L"Dolby Digital");
						}
					}
					break;
					case WAVE_FORMAT_RAW_AAC1: {
						Infos.AddTail(L"AAC");
					}
					break;
					case WAVE_FORMAT_LATM_AAC: {
						Infos.AddTail(L"AAC (LATM)");
					}
					break;
					case WAVE_FORMAT_MPEGLAYER3: {
						Infos.AddTail(L"MP3");
					}
					break;
					case WAVE_FORMAT_MPEG: {
						const MPEG1WAVEFORMAT* pInfoMPEG1 = GetFormatHelper(pInfoMPEG1, _pMediaType);

						int layer = GetHighestBitSet32(pInfoMPEG1->fwHeadLayer) + 1;
						Infos.AddTail(FormatString(L"MPEG1 - Layer %d", layer));
					}
					break;
				}
			}

			if (pClipInfo && (pClipInfo->m_SampleRate == BDVM_SampleRate_48_192 || pClipInfo->m_SampleRate == BDVM_SampleRate_48_96)) {
				switch (pClipInfo->m_SampleRate) {
					case BDVM_SampleRate_48_192:
						Infos.AddTail(FormatString(L"192(48) kHz"));
						break;
					case BDVM_SampleRate_48_96:
						Infos.AddTail(FormatString(L"96(48) kHz"));
						break;
				}
			} else if (pInfo->nSamplesPerSec) {
				Infos.AddTail(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec)/1000.0));
			}
			if (pInfo->nChannels) {
				Infos.AddTail(FormatString(L"%d chn", pInfo->nChannels));
			}
			if (pInfo->wBitsPerSample) {
				Infos.AddTail(FormatString(L"%d bit", pInfo->wBitsPerSample));
			}
			if (pInfo->nAvgBytesPerSec) {
				Infos.AddTail(FormatBitrate(pInfo->nAvgBytesPerSec * 8));
			}

		}
	} else if (_pMediaType->majortype == MEDIATYPE_Subtitle) {
		MajorType = "Subtitle";

		if ((pPresentationDesc == NULL) && ((PES_STREAM_TYPE)_PresentationType == PES_PRIVATE)) {
			pPresentationDesc = L"DVB";
		}

		if (pPresentationDesc) {
			Infos.AddTail(pPresentationDesc);
		}

		if (pClipInfo) {
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty()) {
				Infos.AddHead(name);
			} else if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddHead(name);
				}
			}
		} else if (_pMediaType->cbFormat == sizeof(SUBTITLEINFO)) {
			const SUBTITLEINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);
			CString name = ISO6392ToLanguage(pInfo->IsoLang);

			if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddHead(name);
				}
			} else if (!name.IsEmpty()) {
				Infos.AddHead(name);
			}
			if (pInfo->TrackName[0]) {
				Infos.AddTail(pInfo->TrackName);
			}
		} else if (!lang.IsEmpty()) {
			CString name = ISO6392ToLanguage(lang);
			if (!name.IsEmpty()) {
				Infos.AddHead(name);
			}
		}
	}

	if (!Infos.IsEmpty()) {
		CString Ret;

		Ret += MajorType;
		Ret += " - ";

		bool bFirst = true;

		for (POSITION pos = Infos.GetHeadPosition(); pos; Infos.GetNext(pos)) {
			CString& String = Infos.GetAt(pos);

			if (bFirst) {
				Ret += String;
			} else {
				Ret += L", " + String;
			}

			bFirst = false;
		}

		return Ret;
	}
	return L"";
}

//
// CMpegSplitterFilter
//

CMpegSplitterFilter::CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseSplitterFilter(NAME("CMpegSplitterFilter"), pUnk, phr, clsid)
	, m_pPipoBimbo(false)
	, m_rtPlaylistDuration(0)
	, m_rtMin(0)
	, m_rtMax(0)
	, m_ForcedSub(false)
	, m_AC3CoreOnly(0)
	, m_AlternativeDuration(false)
	, m_SubEmptyPin(false)
{
#ifdef REGISTER_FILTER
	CRegKey key;
	TCHAR buff[256];
	ULONG len;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_MPEGSplit, KEY_READ)) {
		DWORD dw;

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_ForcedSub, dw)) {
			m_ForcedSub = !!dw;
		}

		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_AudioLangOrder, buff, &len)) {
			m_csAudioLanguageOrder = CString(buff);
		}

		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_SubLangOrder, buff, &len)) {
			m_csSubtitlesLanguageOrder = CString(buff);
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_AC3CoreOnly, dw)) {
			m_AC3CoreOnly = dw;
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_AltDuration, dw)) {
			m_AlternativeDuration = !!dw;
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_SubEmptyOutput, dw)) {
			m_SubEmptyPin = !!dw;
		}
	}
#else
	m_ForcedSub					= !!AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_ForcedSub, m_ForcedSub);

	bool UseLangOrder			= !!AfxGetApp()->GetProfileInt(IDS_R_SETTINGS, IDS_RS_INTERNALSELECTTRACKLOGIC, TRUE);
	if (UseLangOrder) {
		m_csSubtitlesLanguageOrder	= AfxGetApp()->GetProfileString(IDS_R_SETTINGS, IDS_RS_SUBTITLESLANGORDER);
		m_csAudioLanguageOrder		= AfxGetApp()->GetProfileString(IDS_R_SETTINGS, IDS_RS_AUDIOSLANGORDER);
	}

	m_AC3CoreOnly				= AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_AC3CoreOnly, m_AC3CoreOnly);
	m_AlternativeDuration		= !!AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_AltDuration, m_AlternativeDuration);
	m_SubEmptyPin				= !!AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_SubEmptyOutput, m_SubEmptyPin);

	m_nFlag					   |= PACKET_PTS_DISCONTINUITY;
#endif
}

STDMETHODIMP CMpegSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IMpegSplitterFilter)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CMpegSplitterFilter::GetClassID(CLSID* pClsID)
{
	CheckPointer (pClsID, E_POINTER);

	if (m_pPipoBimbo) {
		memcpy (pClsID, &CLSID_WMAsfReader, sizeof (GUID));
		return S_OK;
	} else {
		return __super::GetClassID(pClsID);
	}
}

STDMETHODIMP CMpegSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, MpegSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

void CMpegSplitterFilter::ReadClipInfo(LPCOLESTR pszFileName)
{
	if (wcslen (pszFileName) > 0) {
		WCHAR Drive[_MAX_DRIVE];
		WCHAR Dir[_MAX_PATH];
		WCHAR Filename[_MAX_PATH];
		WCHAR Ext[_MAX_EXT];

		if (_wsplitpath_s (pszFileName, Drive, _countof(Drive), Dir, _countof(Dir), Filename, _countof(Filename), Ext, _countof(Ext)) == 0) {
			CString	strClipInfo;

			_wcslwr_s(Ext, _countof(Ext));

			if (wcscmp(Ext, L".ssif") == 0) {
				if (Drive[0]) {
					strClipInfo.Format (_T("%s\\%s\\..\\..\\CLIPINF\\%s.clpi"), Drive, Dir, Filename);
				} else {
					strClipInfo.Format (_T("%s\\..\\..\\CLIPINF\\%s.clpi"), Dir, Filename);
				}
			} else {
				if (Drive[0]) {
					strClipInfo.Format (_T("%s\\%s\\..\\CLIPINF\\%s.clpi"), Drive, Dir, Filename);
				} else {
					strClipInfo.Format (_T("%s\\..\\CLIPINF\\%s.clpi"), Dir, Filename);
				}
			}

			m_ClipInfo.ReadInfo (strClipInfo);
		}
	}
}

STDMETHODIMP CMpegSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	return __super::Load (pszFileName, pmt);
}

HRESULT CMpegSplitterFilter::DemuxNextPacket(REFERENCE_TIME rtStartOffset)
{
	if (m_pFile->IsRandomAccess() && !m_pFile->GetRemaining()) {
		return E_FAIL;
	}

	HRESULT hr;
	BYTE b;

	if (m_pFile->m_type == mpeg_ps || m_pFile->m_type == mpeg_es) {
		if (!m_pFile->NextMpegStartCode(b)) {
			return S_FALSE;
		}

		if (b == 0xba) { // program stream header
			CMpegSplitterFile::pshdr h;
			if (!m_pFile->Read(h)) {
				return S_FALSE;
			}
		} else if (b == 0xbb) { // program stream system header
			CMpegSplitterFile::pssyshdr h;
			if (!m_pFile->Read(h)) {
				return S_FALSE;
			}
		}
		else if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) { // pes packet
			CMpegSplitterFile::peshdr h;

			if (!m_pFile->Read(h, b) || !h.len) {
				return S_FALSE;
			}

			if (h.type == CMpegSplitterFile::mpeg2 && h.scrambling) {
				ASSERT(0);
				return E_FAIL;
			}

			__int64 pos = m_pFile->GetPos();

			DWORD TrackNumber = m_pFile->AddStream(0, b, h.id_ext, h.len);

			if (GetOutputPin(TrackNumber)) {
				CAutoPtr<Packet> p(DNew Packet());

				p->TrackNumber	= TrackNumber;
				p->bSyncPoint	= !!h.fpts;
				p->bAppendable	= !h.fpts;
				p->rtStart		= h.fpts ? (h.pts - rtStartOffset) : INVALID_TIME;
				p->rtStop		= p->rtStart+1;

				p->SetCount(h.len - (size_t)(m_pFile->GetPos() - pos));
				m_pFile->ByteRead(p->GetData(), h.len - (m_pFile->GetPos() - pos));

				hr = DeliverPacket(p);
			}
			m_pFile->Seek(pos + h.len);
		}
	} else if (m_pFile->m_type == mpeg_ts) {
		CMpegSplitterFile::trhdr h;
		if (m_pFile->Read(h) == -1) {
			return S_FALSE;
		}

		__int64 pos = m_pFile->GetPos();

		m_pFile->UpdatePrograms(h, false);

		if (h.payload && ISVALIDPID(h.pid)) {
			DWORD TrackNumber = h.pid;

			CMpegSplitterFile::peshdr h2;

			if (h.payloadstart && m_pFile->NextMpegStartCode(b, 4)) { // pes packet
				if (m_pFile->Read(h2, b)) {
					if (h2.type == CMpegSplitterFile::mpeg2 && h2.scrambling) {
						ASSERT(0);
						return E_FAIL;
					}
					TrackNumber = m_pFile->AddStream(h.pid, b, 0, h.bytes - (DWORD)(m_pFile->GetPos() - pos));
				}
			}

			if (GetOutputPin(TrackNumber) && (h.bytes > (m_pFile->GetPos() - pos))) {
				CAutoPtr<Packet> p(DNew Packet());

				if (h.fPCR) {
					CRefTime rtNow;
					StreamTime(rtNow);
				}

				p->TrackNumber	= TrackNumber;
				p->bAppendable	= !h2.fpts;
				p->rtStart		= h2.fpts ? (h2.pts - rtStartOffset) : INVALID_TIME;
				if (m_rtDuration && p->rtStart > m_rtDuration) {
					p->rtStart	= INVALID_TIME;
				}
				p->rtStop		= (p->rtStart == INVALID_TIME) ? INVALID_TIME : p->rtStart+1;
				p->bSyncPoint	= !!h2.fpts && (p->rtStart != INVALID_TIME);
#if (DEBUG) && 0
				if (h2.fpts) {
					TRACE(_T("h.pid = %d, m_rtPTSOffset = [%10I64d], h2.pts = %ws [%10I64d] ==> %ws [%10I64d]\n"), h.pid, rtStartOffset, ReftimeToString(h2.pts), h2.pts, ReftimeToString(p->rtStart), p->rtStart);
				}
#endif

				size_t nBytes = h.bytes - (m_pFile->GetPos() - pos);
				p->SetCount(nBytes);
				m_pFile->ByteRead(p->GetData(), nBytes);

				hr = DeliverPacket(p);
			}
		}

		m_pFile->Seek(h.next);
	} else if (m_pFile->m_type == mpeg_pva) {
		CMpegSplitterFile::pvahdr h;
		if (!m_pFile->Read(h)) {
			return S_FALSE;
		}

		DWORD TrackNumber = h.streamid;

		__int64 pos = m_pFile->GetPos();

		if (GetOutputPin(TrackNumber)) {
			CAutoPtr<Packet> p(DNew Packet());

			p->TrackNumber	= TrackNumber;
			p->bSyncPoint	= !!h.fpts;
			p->bAppendable	= !h.fpts;
			p->rtStart		= h.fpts ? (h.pts - rtStartOffset) : INVALID_TIME;
			p->rtStop		= p->rtStart+1;
			
			p->SetCount(h.length);
			m_pFile->ByteRead(p->GetData(), h.length);

			hr = DeliverPacket(p);
		}

		m_pFile->Seek(pos + h.length);
	}

	return S_OK;
}

#define ReadBEdw(var) \
	f.Read(&((BYTE*)&var)[3], 1); \
	f.Read(&((BYTE*)&var)[2], 1); \
	f.Read(&((BYTE*)&var)[1], 1); \
	f.Read(&((BYTE*)&var)[0], 1); \

void CMpegSplitterFilter::HandleStream(CMpegSplitterFile::stream& s, CString fName, DWORD dwPictAspectRatioX, DWORD dwPictAspectRatioY)
{
	CMediaType mt = s.mt;

	// correct Aspect Ratio for DVD structure.
	if (s.mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO && dwPictAspectRatioX && dwPictAspectRatioY) {
		VIDEOINFOHEADER2& vih2 = *(VIDEOINFOHEADER2*)s.mt.pbFormat;
		vih2.dwPictAspectRatioX = dwPictAspectRatioX;
		vih2.dwPictAspectRatioY = dwPictAspectRatioY;
	}

	// Add addition GUID for compatible with Cyberlink & Arcsoft VC1 Decoder
	if (mt.subtype == MEDIASUBTYPE_WVC1) {
		mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
		s.mts.push_back(mt);
		mt.subtype = MEDIASUBTYPE_WVC1_ARCSOFT;
		s.mts.push_back(mt);
	}

	// Add addition VobSub type
	if (mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
		CStringA palette;

		for (;;) {
			if (::PathFileExists(fName)) {
				CPath fname(fName);
				fname.StripPath();
				if (!CString(fname).Find(_T("VTS_"))) {
					fName = fName.Left(fName.ReverseFind('.') + 1);
					fName.TrimRight(_T(".0123456789")) += _T("0.ifo");

					if (::PathFileExists(fName)) {
						// read palette from .ifo file, code from CVobSubFile::ReadIfo()
						CFile f;
						if (!f.Open(fName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
							break;
						}

						/* PGC1 */
						f.Seek(0xc0 + 0x0c, SEEK_SET);

						DWORD pos;
						ReadBEdw(pos);

						f.Seek(pos * 0x800 + 0x0c, CFile::begin);

						DWORD offset;
						ReadBEdw(offset);

						/* Subpic palette */
						f.Seek(pos * 0x800 + offset + 0xa4, CFile::begin);

						CAtlList<CStringA> sl;
						for (size_t i = 0; i < 16; i++) {
							BYTE y, u, v, tmp;

							f.Read(&tmp, 1);
							f.Read(&y, 1);
							f.Read(&u, 1);
							f.Read(&v, 1);

							y = (y - 16) * 255 / 219;

							BYTE r = (BYTE)min(max(1.0*y + 1.4022*(v - 128), 0), 255);
							BYTE g = (BYTE)min(max(1.0*y - 0.3456*(u - 128) - 0.7145*(v - 128), 0), 255);
							BYTE b = (BYTE)min(max(1.0*y + 1.7710*(u - 128), 0), 255);

							CStringA str;
							str.Format("%02x%02x%02x", r, g, b);
							sl.AddTail(str);
						}
						palette = Implode(sl, ',');

						f.Close();
					}
				}
			}

			break;
		}

		// Get resolution for first video track
		int vid_width = 0;
		int vid_height = 0;
		POSITION pos = m_pFile->m_streams[CMpegSplitterFile::stream_type::video].GetHeadPosition();
		if (pos) {
			CMpegSplitterFile::stream& sVideo = m_pFile->m_streams[CMpegSplitterFile::stream_type::video].GetHead();
			int arx, ary;
			ExtractDim(&sVideo.mt, vid_width, vid_height, arx, ary);
		}

		CStringA hdr		= VobSubDefHeader(vid_width ? vid_width : 720, vid_height ? vid_height : 576, palette);

		mt.majortype		= MEDIATYPE_Subtitle;
		mt.subtype			= MEDIASUBTYPE_VOBSUB;
		mt.formattype		= FORMAT_SubtitleInfo;
		SUBTITLEINFO* si	= (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.GetLength());
		memset(si, 0, mt.FormatLength());
		si->dwOffset		= sizeof(SUBTITLEINFO);
		strncpy_s(si->IsoLang, pTI ? CStringA(pTI->GetTrackName(s.ps1id)) : "eng", _countof(si->IsoLang)-1);

		memcpy(si + 1, (LPCSTR)hdr, hdr.GetLength());
		s.mts.push_back(mt);

		s.mt.majortype = GUID_NULL;
	}

	if (mt.subtype == MEDIASUBTYPE_H264 && SUCCEEDED(CreateAVCfromH264(&mt))) {
		s.mts.push_back(mt);
	}

	if (s.mt.majortype != GUID_NULL) {
		s.mts.push_back(s.mt);
	}
}

//
HRESULT CMpegSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();

	ReadClipInfo(GetPartFilename(pAsyncReader));
	m_pFile.Attach(DNew CMpegSplitterFile(pAsyncReader, hr, m_ClipInfo, m_rtPlaylistDuration > 0, m_ForcedSub, m_AC3CoreOnly, m_AlternativeDuration, m_SubEmptyPin));

	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}

	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	if (m_rtMin && m_rtMax && m_rtMax > m_rtMin) {
		m_pFile->m_rtMin = m_rtMin;
		m_pFile->m_rtMax = m_rtMax;
	}

	REFERENCE_TIME	rt_IfoDuration	= 0;
	AV_Rational		IfoASpect		= {0, 0};
	if (m_pFile->m_type == mpeg_ps) {
		if (m_pInput && m_pInput->IsConnected() && (GetCLSID(m_pInput->GetConnected()) == __uuidof(CVTSReader))) { // MPC VTS Reader
			pTI = GetFilterFromPin(m_pInput->GetConnected());

			if (CComPtr<IBaseFilter> pFilter = GetFilterFromPin(m_pInput->GetConnected()) ) {
				if (CComQIPtr<IVTSReader> VTSREader = pFilter) {
					rt_IfoDuration	= VTSREader->GetDuration();
					IfoASpect		= VTSREader->GetAspect();
				}
			}
		}
	}

	CString cs_audioProgram, cs_subpicProgram;

	if (m_ClipInfo.IsHdmv()) {
		for (size_t i = 0; i < m_ClipInfo.GetStreamCount(); i++) {
			CHdmvClipInfo::Stream* stream = m_ClipInfo.GetStreamByIndex (i);
			if (stream->m_Type == PRESENTATION_GRAPHICS_STREAM) {
				m_pFile->AddHdmvPGStream(stream->m_PID, stream->m_LanguageCode);
			}
		}
	}

	CString lang;
	CAtlList<CString> lang_list_audio;
	CAtlList<CString> lang_list_subpic;
	int	Idx_audio  = 99;
	int	Idx_subpic = 99;

	if (!m_csAudioLanguageOrder.IsEmpty()) {
		int tPos = 0;
		lang = m_csAudioLanguageOrder.Tokenize(_T(",; "), tPos);
		while (tPos != -1) {
			if (!lang.IsEmpty()) lang_list_audio.AddTail(lang);
			lang = m_csAudioLanguageOrder.Tokenize(_T(",; "), tPos);
		}
		if (!lang.IsEmpty()) lang_list_audio.AddTail(lang);
	}

	if (!m_csSubtitlesLanguageOrder.IsEmpty()) {
		int tPos = 0;
		lang = m_csSubtitlesLanguageOrder.Tokenize(_T(",; "), tPos);
		while (tPos != -1) {
			if (!lang.IsEmpty()) lang_list_subpic.AddTail(lang);
			lang = m_csSubtitlesLanguageOrder.Tokenize(_T(",; "), tPos);
		}
		if (!lang.IsEmpty()) lang_list_subpic.AddTail(lang);
	}

	for (int type = CMpegSplitterFile::stream_type::video; type < _countof(m_pFile->m_streams); type++) {
		POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
		while (pos) {
			CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);

			CStringW name = CMpegSplitterFile::CStreamList::ToString(type);
			CStringW str;

			if (type == CMpegSplitterFile::stream_type::subpic && s.pid == NO_SUBTITLE_PID) {
				str	= NO_SUBTITLE_NAME;
				continue;
			} else {
				int iProgram = -1;
				const CHdmvClipInfo::Stream *pClipInfo;
				const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, iProgram, pClipInfo);
				const wchar_t *pStreamName	= NULL;
				int StreamType				= pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[iProgram].type : 0;
				pStreamName					= StreamTypeToName((PES_STREAM_TYPE)StreamType);

				CStringA lang_name	= s.lang;
				lang_name			= pTI ? CStringA(pTI->GetTrackName(s.ps1id)) : lang_name;

				CString FormatDesc = GetMediaTypeDesc(&s.mt, pClipInfo, StreamType, lang_name);

				if (!FormatDesc.IsEmpty()) {
					str.Format(L"%s (%04x,%02x,%02x)", FormatDesc.GetString(), s.pid, s.pesid, s.ps1id);
				} else if (pStreamName) {
					str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id);
				} else {
					str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id);
				}
			}
			CString str_tmp = str;
			str_tmp.MakeLower();
			if (type == CMpegSplitterFile::stream_type::audio) {
				if (lang_list_audio.GetCount() > 0) {
					int idx = 0;
					POSITION pos = lang_list_audio.GetHeadPosition();
					while (pos) {
						lang = lang_list_audio.GetNext(pos).MakeLower();
						if (-1 != str_tmp.Find(lang)) {
							if (idx<Idx_audio) {
								cs_audioProgram	= str;
								Idx_audio		= idx;
								break;
							}
						}
						idx++;
					}
				}
				if (!Idx_audio && !cs_audioProgram.IsEmpty()) break;
			}
			if (type == CMpegSplitterFile::stream_type::subpic) {
				if (lang_list_subpic.GetCount() > 0) {
					int idx = 0;
					POSITION pos = lang_list_subpic.GetHeadPosition();
					while (pos) {
						lang = lang_list_subpic.GetNext(pos).MakeLower();
						if (-1 != str_tmp.Find(lang)) {
							if (idx<Idx_subpic) {
								cs_subpicProgram	= str;
								Idx_subpic			= idx;
								break;
							}
						}
						idx++;
					}
				}
				if (!Idx_subpic && !cs_subpicProgram.IsEmpty()) break;
			}
		}
	}

	CString fullName = GetPartFilename(pAsyncReader);
	if (fullName.IsEmpty()) {
		// trying to get file name from FileSource
		BeginEnumFilters(m_pGraph, pEF, pBF) {
			CComQIPtr<IFileSourceFilter> pFSF = pBF;
			if (pFSF) {
				LPOLESTR pFN = NULL;
				AM_MEDIA_TYPE mt;
				if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
					fullName = CString(pFN);
					CoTaskMemFree(pFN);
				}
				break;
			}
		}
		EndEnumFilters
	}

	for (int type = CMpegSplitterFile::stream_type::video; type < _countof(m_pFile->m_streams); type++) {
		POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
		while (pos) {
			CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);
			HandleStream(s, fullName, IfoASpect.num, IfoASpect.den);
		}
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	for (int type = CMpegSplitterFile::stream_type::video; type < _countof(m_pFile->m_streams); type++) {
		POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
		while (pos) {
			CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);

			CStringW name = CMpegSplitterFile::CStreamList::ToString(type);
			CStringW str;

			if (type == CMpegSplitterFile::stream_type::subpic && s.pid == NO_SUBTITLE_PID) {
				str	= NO_SUBTITLE_NAME;
			} else {
				int iProgram = -1;
				const CHdmvClipInfo::Stream *pClipInfo;
				const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, iProgram, pClipInfo);
				const wchar_t *pStreamName	= NULL;
				int StreamType				= pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[iProgram].type : 0;
				pStreamName					= StreamTypeToName((PES_STREAM_TYPE)StreamType);

				CStringA lang_name	= s.lang;
				lang_name			= pTI ? CStringA(pTI->GetTrackName(s.ps1id)) : lang_name;

				CString FormatDesc = GetMediaTypeDesc(&s.mt, pClipInfo, StreamType, lang_name);

				if (!FormatDesc.IsEmpty()) {
					str.Format(L"%s (%04x,%02x,%02x)", FormatDesc.GetString(), s.pid, s.pesid, s.ps1id);
				} else if (pStreamName) {
					str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id);
				} else {
					str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id);
				}
			}

			CAtlArray<CMediaType> mts;
			for(size_t i = 0; i < s.mts.size(); i++) {
				mts.Add(s.mts[i]);
			}
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CMpegSplitterOutputPin(mts, str, this, this, &hr, (type != CMpegSplitterFile::stream_type::video) ? 20 : 1));

			if (type == CMpegSplitterFile::stream_type::audio) {
				if (!cs_audioProgram.IsEmpty()) {
					if ((!cs_audioProgram.Compare(str)) && (S_OK == AddOutputPin(s, pPinOut))) {
						break;
					}
				} else {
					if (S_OK == AddOutputPin(s, pPinOut)) {
						break;
					}
				}
			}
			else if (type == CMpegSplitterFile::stream_type::subpic) {
				if (!cs_subpicProgram.IsEmpty()) {
					if ((!cs_subpicProgram.Compare(str)) && (S_OK == AddOutputPin(s, pPinOut))) {
						break;
					}
				} else {
					if ((m_pFile->m_streams[CMpegSplitterFile::stream_type::subpic].GetCount() == 1) && (S_OK == AddOutputPin(s, pPinOut))) {
						break;
					} else if ((s.pid != NO_SUBTITLE_PID) && (S_OK == AddOutputPin(s, pPinOut))) {
						break;
					}
				}
			} else {
				if (S_OK == AddOutputPin(s, pPinOut)) {
					break;
				}
			}
		}
	}

	if (m_rtPlaylistDuration) {
		m_rtNewStop = m_rtStop = m_rtDuration = m_rtPlaylistDuration;
	} else if (m_pFile->IsRandomAccess() && m_pFile->m_rate) {
		m_rtNewStop = m_rtStop = m_rtDuration = rt_IfoDuration ? rt_IfoDuration : (UNITS * m_pFile->GetLength() / m_pFile->m_rate);
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMpegSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CMpegSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	m_rtStartOffset = 0;

	return true;
}

#define SimpleSeek														\
	REFERENCE_TIME rtPTS = m_pFile->NextPTS(pMasterStream->GetHead());	\
	if (rtPTS != INVALID_TIME) {										\
		m_rtStartOffset = m_pFile->m_rtMin + rtPTS - rt;				\
	}																	\
	if (m_rtStartOffset > m_pFile->m_rtMax)  {							\
		m_rtStartOffset = 0;											\
	}																	\

#define SeekPos(t) (__int64)(1.0 * t / m_rtDuration * len)

void CMpegSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	CAtlList<CMpegSplitterFile::stream>* pMasterStream = m_pFile->GetMasterStream();
	if (!pMasterStream) {
		ASSERT(0);
		return;
	}

	if (m_pFile->IsStreaming()) {
		m_pFile->Seek(max(0, m_pFile->GetLength() - 256 * KILOBYTE));
		m_rtStartOffset = m_pFile->m_rtMin + m_pFile->NextPTS(pMasterStream->GetHead());
		return;
	}

	if (rt <= UNITS || m_rtDuration <= 0) {
		m_pFile->Seek(0);
		if (m_rtDuration && m_pFile->bIsBadPacked) {
			SimpleSeek;
		}
	} else {
		__int64 pos = SeekBD(rt);
		if (pos >= 0) {
			if (pos < m_pFile->GetLength()) {
				m_pFile->Seek(pos + 4);

				CMpegSplitterFile::stream stream = pMasterStream->GetHead();
				REFERENCE_TIME rtmax = rt - UNITS;
				REFERENCE_TIME rtPTS = m_pFile->NextPTS(stream);

				while (rtPTS <= rtmax && rtPTS != INVALID_TIME && (m_pFile->GetPos() + 192) < m_pFile->GetLength()) {
					m_pFile->Seek(m_pFile->GetPos() + 192);
					rtPTS = m_pFile->NextPTS(stream);
				}
			}

			return;
		}

		__int64 len				= m_pFile->GetLength();
		__int64 seekpos			= SeekPos(rt);
		__int64 minseekpos		= _I64_MIN;

		REFERENCE_TIME rtmax	= rt - UNITS;
		REFERENCE_TIME rtmin	= rtmax - UNITS/2;

		if (!m_pFile->bIsBadPacked) {
			POSITION pos = pMasterStream->GetHeadPosition();
			while (pos) {
				DWORD TrackNum = pMasterStream->GetNext(pos);

				CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNum);
				if (pPin && pPin->IsConnected()) {
					m_pFile->Seek(seekpos);
					__int64 curpos = seekpos;

					if (m_pFile->m_type == mpeg_ts) {
						double div = 1.0;
						for (;;) {
							REFERENCE_TIME rt2 = m_pFile->NextPTS(TrackNum);
							if (rt2 == INVALID_TIME) {
								break;
							}

							if (rtmin <= rt2 && rt2 <= rtmax) {
								//minseekpos = curpos;
								minseekpos = m_pFile->GetPos();
								break;
							}

							REFERENCE_TIME dt = rt2 - rtmax;
							if (rt2 < 0) {
								dt = 20*UNITS / div;
							}
							dt /= div;
							div += 0.05;

							if (div >= 5.0) {
								break;
							}

							curpos -= SeekPos(dt);
							m_pFile->Seek(curpos);
						}
					} else {
						for (int j = 0; j < 10; j++) {
							REFERENCE_TIME rt2 = m_pFile->NextPTS(TrackNum);
							if (rt2 < 0) {
								break;
							}

							if (rtmin <= rt2 && rt2 <= rtmax) {
								//minseekpos = curpos;
								minseekpos = m_pFile->GetPos();
								break;
							}

							REFERENCE_TIME dt = rt2 - rtmax;
							curpos -= SeekPos(dt);
							m_pFile->Seek(curpos);
						}
					}
				}
			}
		}

		if (minseekpos != _I64_MIN) {
			seekpos			= minseekpos;
			m_rtStartOffset	= 0;
		} else {
			// simple seek by bitrate

			seekpos	= SeekPos(rt);
			m_pFile->Seek(seekpos);

			SimpleSeek;
			if (rtPTS != INVALID_TIME) {
				seekpos = m_pFile->GetPos();
			}
		}

		m_pFile->Seek(seekpos);
	}
}

bool CMpegSplitterFilter::DemuxLoop()
{
	REFERENCE_TIME rtStartOffset = m_rtStartOffset ? m_rtStartOffset : m_pFile->m_rtMin;

	CAtlList<CMpegSplitterFile::stream>* pMasterStream = m_pFile->GetMasterStream();
	if (!pMasterStream) {
		ASSERT(0);
		return false;
	}

	const CMpegSplitterFile::stream st = pMasterStream->GetHead();
	BOOL bMainIsVideo	= (st.mt.majortype == MEDIATYPE_Video);
	__int64 AvailBytes	= bMainIsVideo ? 256 * KILOBYTE : 16 * KILOBYTE;

	HRESULT hr = S_OK;
	while (SUCCEEDED(hr) && !CheckRequest(NULL)) {
		if (m_pFile->IsStreaming()) {
			m_pFile->WaitAvailable(2000, AvailBytes);
		}

		hr = DemuxNextPacket(rtStartOffset);
	}

	return true;
}

bool CMpegSplitterFilter::BuildPlaylist(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& Items)
{
	m_rtPlaylistDuration = 0;

	bool res = SUCCEEDED (m_ClipInfo.ReadPlaylist (pszFileName, m_rtPlaylistDuration, Items, TRUE));
	if (res) {
		m_rtMin = Items.GetHead()->m_rtIn;
		REFERENCE_TIME rtDur = 0;

		POSITION pos = Items.GetHeadPosition();
		while (pos) {
			rtDur += Items.GetNext(pos)->Duration();
		}

		m_rtMax = m_rtMin + rtDur;
	}
	
	return res;
}

bool CMpegSplitterFilter::BuildChapters(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& PlaylistItems, CHdmvClipInfo::CPlaylistChapter& Items)
{
	return SUCCEEDED (m_ClipInfo.ReadChapters (pszFileName, PlaylistItems, Items)) ? true : false;
}

// IAMStreamSelect

STDMETHODIMP CMpegSplitterFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	*pcStreams = 0;

	for (int i = 0; i < _countof(m_pFile->m_streams); i++) {
		(*pcStreams) += m_pFile->m_streams[i].GetCount();
	}

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::Enable(long lIndex, DWORD dwFlags)
{
	if (!(dwFlags & AMSTREAMSELECTENABLE_ENABLE)) {
		return E_NOTIMPL;
	}

	for (int type = CMpegSplitterFile::stream_type::video, j = 0; type < _countof(m_pFile->m_streams); type++) {
		int cnt = m_pFile->m_streams[type].GetCount();

		if (lIndex >= j && lIndex < j+cnt) {
			lIndex -= j;

			POSITION pos = m_pFile->m_streams[type].FindIndex(lIndex);
			if (!pos) {
				return E_UNEXPECTED;
			}

			CMpegSplitterFile::stream& to = m_pFile->m_streams[type].GetAt(pos);

			pos = m_pFile->m_streams[type].GetHeadPosition();
			while (pos) {
				CMpegSplitterFile::stream& from = m_pFile->m_streams[type].GetNext(pos);
				if (!GetOutputPin(from)) {
					continue;
				}

				if (from == to) {
					return S_OK;
				}

				CComQIPtr<IMediaControl> pMC(m_pGraph);
				OAFilterState fs = -1;
				if(pMC) {
					pMC->GetState(100, &fs);
					pMC->Stop();
				}
				Lock();

				HRESULT hr = RenameOutputPin(from, to, to.mts, type == CMpegSplitterFile::stream_type::subpic);

				Unlock();
				if (pMC) {
					if(fs == State_Running) {
						pMC->Run();
					} else if (fs == State_Paused) {
						pMC->Pause();
					}
				}

				if (FAILED(hr)) {
					return hr;
				}

				return S_OK;
			}
		}

		j += cnt;
	}

	return S_FALSE;
}

STDMETHODIMP CMpegSplitterFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	for (int type = CMpegSplitterFile::stream_type::video, j = 0; type < _countof(m_pFile->m_streams); type++) {
		int cnt = m_pFile->m_streams[type].GetCount();

		if (lIndex >= j && lIndex < j+cnt) {
			lIndex -= j;

			POSITION pos = m_pFile->m_streams[type].FindIndex(lIndex);
			if (!pos) {
				return E_UNEXPECTED;
			}

			CMpegSplitterFile::stream& s	= m_pFile->m_streams[type].GetAt(pos);
			CHdmvClipInfo::Stream* pStream	= m_ClipInfo.FindStream(s.pid);

			if (ppmt) {
				*ppmt = CreateMediaType(&s.mts[0]);
			}
			if (pdwFlags) {
				*pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
			}
			if (plcid) {
				CStringA lang_name	= s.lang;
				LCID lcid			= !lang_name.IsEmpty() ? ISO6392ToLcid(lang_name) : 0;

				*plcid = pStream ? pStream->m_LCID : lcid;
			}
			if (pdwGroup) {
				*pdwGroup = type;
			}
			if (ppObject) {
				*ppObject = NULL;
			}
			if (ppUnk) {
				*ppUnk = NULL;
			}

			if (ppszName) {
				CString name = CMpegSplitterFile::CStreamList::ToString(type);

				CString str;

				if (type == CMpegSplitterFile::stream_type::subpic && s.pid == NO_SUBTITLE_PID) {
					str = NO_SUBTITLE_NAME;
					if (plcid) {
						*plcid = (LCID)LCID_NOSUBTITLES;
					}
				} else {
					int iProgram;
					const CHdmvClipInfo::Stream *pClipInfo;
					const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, iProgram, pClipInfo);
					const wchar_t *pStreamName	= NULL;
					int StreamType				= pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[iProgram].type : 0;
					pStreamName					= StreamTypeToName((PES_STREAM_TYPE)StreamType);

					CStringA lang_name	= s.lang;
					lang_name			= pTI ? CStringA(pTI->GetTrackName(s.ps1id)) : lang_name;

					CString FormatDesc = GetMediaTypeDesc(&s.mt, pClipInfo, StreamType, lang_name);

					if (!FormatDesc.IsEmpty()) {
						str.Format(L"%s (%04x,%02x,%02x)", FormatDesc.GetString(), s.pid, s.pesid, s.ps1id);
					} else if (pStreamName) {
						str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id);
					} else {
						str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id);
					}
				}

				*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength() + 1) * sizeof(WCHAR));
				if (*ppszName == NULL) {
					return E_OUTOFMEMORY;
				}

				wcscpy_s(*ppszName, str.GetLength() + 1, str);
			}

			return S_OK;
		}

		j += cnt;
	}

	return S_FALSE;
}

// ISpecifyPropertyPages2

STDMETHODIMP CMpegSplitterFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CMpegSplitterSettingsWnd);

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpegSplitterSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpegSplitterSettingsWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IMpegSplitterFilter
STDMETHODIMP CMpegSplitterFilter::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_MPEGSplit)) {
		key.SetDWORDValue(OPT_ForcedSub, m_ForcedSub);
		key.SetStringValue(OPT_AudioLangOrder, m_csAudioLanguageOrder);
		key.SetStringValue(OPT_SubLangOrder, m_csSubtitlesLanguageOrder);
		key.SetDWORDValue(OPT_AC3CoreOnly, m_AC3CoreOnly);
		key.SetDWORDValue(OPT_AltDuration, m_AlternativeDuration);
		key.SetDWORDValue(OPT_SubEmptyOutput, m_SubEmptyPin);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_ForcedSub, m_ForcedSub);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_AC3CoreOnly, m_AC3CoreOnly);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_AltDuration, m_AlternativeDuration);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_SubEmptyOutput, m_SubEmptyPin);
#endif

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::SetForcedSub(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_ForcedSub = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpegSplitterFilter::GetForcedSub()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_ForcedSub;
}

STDMETHODIMP CMpegSplitterFilter::SetAudioLanguageOrder(WCHAR *nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_csAudioLanguageOrder = nValue;
	return S_OK;
}

STDMETHODIMP_(WCHAR *) CMpegSplitterFilter::GetAudioLanguageOrder()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_csAudioLanguageOrder.GetBuffer();
}

STDMETHODIMP CMpegSplitterFilter::SetSubtitlesLanguageOrder(WCHAR *nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_csSubtitlesLanguageOrder = nValue;
	return S_OK;
}

STDMETHODIMP_(WCHAR *) CMpegSplitterFilter::GetSubtitlesLanguageOrder()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_csSubtitlesLanguageOrder.GetBuffer();
}

STDMETHODIMP CMpegSplitterFilter::SetTrueHD(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_AC3CoreOnly = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMpegSplitterFilter::GetTrueHD()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_AC3CoreOnly;
}

STDMETHODIMP CMpegSplitterFilter::SetAlternativeDuration(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_AlternativeDuration = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpegSplitterFilter::GetAlternativeDuration()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_AlternativeDuration;
}

STDMETHODIMP CMpegSplitterFilter::SetSubEmptyPin(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_SubEmptyPin = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpegSplitterFilter::GetSubEmptyPin()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_SubEmptyPin;
}

STDMETHODIMP_(int) CMpegSplitterFilter::GetMPEGType()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_pFile->m_type;
}
//
// CMpegSourceFilter
//

CMpegSourceFilter::CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CMpegSplitterFilter(pUnk, phr, clsid)
{
	m_pInput.Free();
}

//
// CMpegSplitterOutputPin
//

CMpegSplitterOutputPin::CMpegSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int QueueMaxPackets)
	: CBaseSplitterParserOutputPin(mts, pName, pFilter, pLock, phr, QueueMaxPackets)
{
}

STDMETHODIMP CMpegSplitterOutputPin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	HRESULT		hr;
	PIN_INFO	PinInfo;
	GUID		FilterClsid;

	if (SUCCEEDED(pReceivePin->QueryPinInfo(&PinInfo))) {
		if (SUCCEEDED(PinInfo.pFilter->GetClassID(&FilterClsid))) {
			if (FilterClsid == CLSID_DMOWrapperFilter) {
				(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(true);
			}
		}
		PinInfo.pFilter->Release();
	}

	hr = __super::Connect (pReceivePin, pmt);
	(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(false);
	return hr;
}
