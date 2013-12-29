/*
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
#include "PPageInternalFilters.h"
#include "ComPropertyPage.h"
#include "../../filters/filters.h"


static filter_t s_filters[] = {

	// Source filters
	{_T("AMR"),                   SOURCE_FILTER,  SOURCE, SRC_AMR,        0},
	{_T("AVI"),                   SOURCE_FILTER,  SOURCE, SRC_AVI,        IDS_SRC_AVI},
	{_T("CDDA (Audio CD)"),       SOURCE_FILTER,  SOURCE, SRC_CDDA,       IDS_SRC_CDDA},
	{_T("CDXA (VCD/SVCD/XCD)"),   SOURCE_FILTER,  SOURCE, SRC_CDXA,       0},
	{_T("DirectShow Media"),      SOURCE_FILTER,  SOURCE, SRC_DSM,        0},
	{_T("DTS AudioCD"),           SOURCE_FILTER,  SOURCE, SRC_DTS,        IDS_SRC_DTS},
	{_T("DTS/AC3"),               SOURCE_FILTER,  SOURCE, SRC_DTSAC3,     0},
	{_T("DVD Video Title Set"),   SOURCE_FILTER,  SOURCE, SRC_VTS,        IDS_SRC_VTS},
	{_T("FLI/FLC"),               SOURCE_FILTER,  SOURCE, SRC_FLIC,       0},
	{_T("FLAC"),                  SOURCE_FILTER,  SOURCE, SRC_FLAC,       0},
	{_T("FLV"),                   SOURCE_FILTER,  SOURCE, SRC_FLV,        0},
	{_T("Matroska"),              SOURCE_FILTER,  SOURCE, SRC_MATROSKA,   0},
	{_T("MP4/MOV"),               SOURCE_FILTER,  SOURCE, SRC_MP4,        0},
	{_T("MPEG Audio"),            SOURCE_FILTER,  SOURCE, SRC_MPA,        IDS_SRC_MPA},
	{_T("MPEG PS/TS/PVA"),        SOURCE_FILTER,  SOURCE, SRC_MPEG,       0},
	{_T("MusePack"),              SOURCE_FILTER,  SOURCE, SRC_MPAC,       0},
	{_T("Ogg/Opus/Speex"),        SOURCE_FILTER,  SOURCE, SRC_OGG,        0},
	{_T("RAW Video"),             SOURCE_FILTER,  SOURCE, SRC_RAWVIDEO,   0},
	{_T("RealMedia"),             SOURCE_FILTER,  SOURCE, SRC_REALMEDIA,  IDS_SRC_REALMEDIA},
	{_T("RoQ"),                   SOURCE_FILTER,  SOURCE, SRC_ROQ,        IDS_SRC_ROQ},
	{_T("SHOUTcast"),             SOURCE_FILTER,  SOURCE, SRC_SHOUTCAST,  0},
	{_T("TAK"),                   SOURCE_FILTER,  SOURCE, SRC_TAK,        0},
	{_T("TTA"),                   SOURCE_FILTER,  SOURCE, SRC_TTA,        0},
	{_T("WavPack"),               SOURCE_FILTER,  SOURCE, SRC_WPAC,       0},
	{_T("UDP/HTTP"),              SOURCE_FILTER,  SOURCE, SRC_UDP,        0},

	// Audio decoder
	{_T("AAC"),                   FFMPEG_DECODER, AUDIO,  FFM_AAC,        IDS_TRA_FFMPEG,},
	{_T("AC3/E-AC3/TrueHD/MLP"),  DECODER,        AUDIO,  TRA_AC3,        IDS_TRA_FFMPEG,},
	{_T("ALAC"),                  FFMPEG_DECODER, AUDIO,  FFM_ALAC,       IDS_TRA_FFMPEG,},
	{_T("ALS"),                   FFMPEG_DECODER, AUDIO,  FFM_ALS,        IDS_TRA_FFMPEG,},
	{_T("AMR"),                   FFMPEG_DECODER, AUDIO,  FFM_AMR,        IDS_TRA_FFMPEG,},
	{_T("APE"),                   FFMPEG_DECODER, AUDIO,  FFM_APE,        IDS_TRA_FFMPEG,},
	{_T("Bink Audio"),            FFMPEG_DECODER, AUDIO,  FFM_BINKA,      IDS_TRA_FFMPEG,},
	{_T("DSP Group TrueSpeech"),  FFMPEG_DECODER, AUDIO,  FFM_TRUESPEECH, IDS_TRA_FFMPEG,},
	{_T("DTS"),                   DECODER,        AUDIO,  TRA_DTS,        IDS_TRA_FFMPEG,},
	{_T("FLAC"),                  FFMPEG_DECODER, AUDIO,  FFM_FLAC,       IDS_TRA_FFMPEG},
	{_T("Indeo Audio"),           FFMPEG_DECODER, AUDIO,  FFM_IAC,        IDS_TRA_FFMPEG,},
	{_T("LPCM"),                  DECODER,        AUDIO,  TRA_LPCM,       IDS_TRA_LPCM,},
	{_T("MPEG Audio"),            FFMPEG_DECODER, AUDIO,  FFM_MPA,        IDS_TRA_FFMPEG,},
	{_T("MusePack SV7/SV8"),      FFMPEG_DECODER, AUDIO,  FFM_MPAC,       IDS_TRA_FFMPEG,},
	{_T("Nellymoser"),            FFMPEG_DECODER, AUDIO,  FFM_NELLY,      IDS_TRA_FFMPEG,},
	{_T("Opus"),                  FFMPEG_DECODER, AUDIO,  FFM_OPUS,       IDS_TRA_FFMPEG,},
	{_T("PS2 Audio (PCM/ADPCM)"), DECODER,        AUDIO,  TRA_PS2AUD,     IDS_TRA_PS2AUD,},
	{_T("QDesign Music Codec 2"), FFMPEG_DECODER, AUDIO,  FFM_QDM2,       IDS_TRA_FFMPEG,},
	{_T("RealAudio"),             DECODER,        AUDIO,  TRA_RA,         IDS_TRA_RA},
	{_T("Speex"),                 FFMPEG_DECODER, AUDIO,  FFM_SPEEX,      IDS_TRA_FFMPEG,},
	{_T("TAK"),                   FFMPEG_DECODER, AUDIO,  FFM_TAK,        IDS_TRA_FFMPEG,},
	{_T("TTA"),                   FFMPEG_DECODER, AUDIO,  FFM_TTA,        IDS_TRA_FFMPEG,},
	{_T("Vorbis"),                FFMPEG_DECODER, AUDIO,  FFM_VORBIS,     IDS_TRA_FFMPEG,},
	{_T("Voxware MetaSound"),     FFMPEG_DECODER, AUDIO,  FFM_VOXWARE,    IDS_TRA_FFMPEG,},
	{_T("WavPack lossless audio"),FFMPEG_DECODER, AUDIO,  FFM_WPAC,       IDS_TRA_FFMPEG,},
	{_T("WMA v.1/v.2"),           FFMPEG_DECODER, AUDIO,  FFM_WMA2,       IDS_TRA_FFMPEG,},
	{_T("WMA v.9 Professional"),  FFMPEG_DECODER, AUDIO,  FFM_WMAPRO,     IDS_TRA_FFMPEG,},
	{_T("WMA Lossless"),          FFMPEG_DECODER, AUDIO,  FFM_WMALOSS,    IDS_TRA_FFMPEG,},
	{_T("WMA Voice"),             FFMPEG_DECODER, AUDIO,  FFM_WMAVOICE,   IDS_TRA_FFMPEG,},
	{_T("Other PCM/ADPCM"),       DECODER,        AUDIO,  TRA_PCM,        IDS_TRA_FFMPEG,},

	// DXVA decoder
	{_T("H264/AVC (DXVA)"),       DXVA_DECODER,   VIDEO,  TRA_DXVA_H264,  IDS_TRA_FFMPEG},
	{_T("MPEG-2 Video (DXVA)"),   DXVA_DECODER,   VIDEO,  TRA_DXVA_MPEG2, IDS_TRA_FFMPEG},
	{_T("VC1 (DXVA)"),            DXVA_DECODER,   VIDEO,  TRA_DXVA_VC1,   IDS_TRA_FFMPEG},
	{_T("WMV3 (DXVA)"),           DXVA_DECODER,   VIDEO,  TRA_DXVA_WMV3,  IDS_TRA_FFMPEG},

	// Video Decoder
	{_T("AMV Video"),             FFMPEG_DECODER, VIDEO,  FFM_AMVV,       IDS_TRA_FFMPEG},
	{_T("Apple ProRes"),          FFMPEG_DECODER, VIDEO,  FFM_PRORES,     IDS_TRA_FFMPEG},
	{_T("Avid DNxHD"),            FFMPEG_DECODER, VIDEO,  FFM_DNXHD,      IDS_TRA_FFMPEG},
	{_T("Bink Video"),            FFMPEG_DECODER, VIDEO,  FFM_BINKV,      IDS_TRA_FFMPEG},
	{_T("Canopus Lossless"),      FFMPEG_DECODER, VIDEO,  FFM_CLLC,       IDS_TRA_FFMPEG},
	{_T("Dirac"),                 FFMPEG_DECODER, VIDEO,  FFM_DIRAC,      IDS_TRA_FFMPEG},
	{_T("DivX"),                  FFMPEG_DECODER, VIDEO,  FFM_DIVX,       IDS_TRA_FFMPEG},
	{_T("DV Video"),              FFMPEG_DECODER, VIDEO,  FFM_DV,         IDS_TRA_FFMPEG},
	{_T("FLV1/4"),                FFMPEG_DECODER, VIDEO,  FFM_FLV4,       IDS_TRA_FFMPEG},
	{_T("H263"),                  FFMPEG_DECODER, VIDEO,  FFM_H263,       IDS_TRA_FFMPEG},
	{_T("H264/AVC (FFmpeg)"),     FFMPEG_DECODER, VIDEO,  FFM_H264,       IDS_TRA_FFMPEG},
	{_T("HEVC (experimental)"),   FFMPEG_DECODER, VIDEO,  FFM_HEVC,       IDS_TRA_FFMPEG},
	{_T("Indeo 3/4/5"),           FFMPEG_DECODER, VIDEO,  FFM_INDEO,      IDS_TRA_FFMPEG},
	{_T("Lossless video (huffyuv, Lagarith, FFV1)"), FFMPEG_DECODER, VIDEO,  FFM_LOSSLESS,   IDS_TRA_FFMPEG},
	{_T("MJPEG"),                 FFMPEG_DECODER, VIDEO,  FFM_MJPEG,      IDS_TRA_FFMPEG},
	{_T("MPEG-1 Video (FFmpeg)"), FFMPEG_DECODER, VIDEO,  FFM_MPEG1,      IDS_TRA_FFMPEG},
	{_T("MPEG-2 Video (FFmpeg)"), FFMPEG_DECODER, VIDEO,  FFM_MPEG2,      IDS_TRA_FFMPEG},
	{_T("MPEG-1 Video (libmpeg2)"),     DECODER,        VIDEO,  TRA_MPEG1,      IDS_TRA_MPEG2},
	{_T("MPEG-2/DVD Video (libmpeg2)"), DECODER,        VIDEO,  TRA_MPEG2,      IDS_TRA_MPEG2},
	{_T("MS MPEG-4"),             FFMPEG_DECODER, VIDEO,  FFM_MSMPEG4,    IDS_TRA_FFMPEG},
	{_T("PNG"),                   FFMPEG_DECODER, VIDEO,  FFM_PNG,        IDS_TRA_FFMPEG},
	{_T("Screen Recorder (Cinepak/CSCD/QTRle/MS/TSCC/VMnc)"), FFMPEG_DECODER, VIDEO, FFM_SCREC, IDS_TRA_FFMPEG},
	{_T("SVQ1/3"),                FFMPEG_DECODER, VIDEO,  FFM_SVQ3,       IDS_TRA_FFMPEG},
	{_T("Theora"),                FFMPEG_DECODER, VIDEO,  FFM_THEORA,     IDS_TRA_FFMPEG},
	{_T("Ut Video"),              FFMPEG_DECODER, VIDEO,  FFM_UTVD,       IDS_TRA_FFMPEG},
	{_T("VC1 (FFmpeg)"),          FFMPEG_DECODER, VIDEO,  FFM_VC1,        IDS_TRA_FFMPEG},
	{_T("VP3/5/6"),               FFMPEG_DECODER, VIDEO,  FFM_VP356,      IDS_TRA_FFMPEG},
	{_T("VP8/9"),                 FFMPEG_DECODER, VIDEO,  FFM_VP8,        IDS_TRA_FFMPEG},
	{_T("WMV1/2/3"),              FFMPEG_DECODER, VIDEO,  FFM_WMV,        IDS_TRA_FFMPEG},
	{_T("Xvid/MPEG-4"),           FFMPEG_DECODER, VIDEO,  FFM_XVID,       IDS_TRA_FFMPEG},
	{_T("RealVideo"),             FFMPEG_DECODER, VIDEO,  FFM_RV,         IDS_TRA_RV},
	{_T("Uncompressed video (v210, V410, Y800, I420, ...)"),   FFMPEG_DECODER, VIDEO, FFM_UNCOMPRESSED, IDS_TRA_FFMPEG},

	// End
	{NULL, 0, 0, 0, NULL}
};

IMPLEMENT_DYNAMIC(CPPageInternalFiltersListBox, CCheckListBox)
CPPageInternalFiltersListBox::CPPageInternalFiltersListBox(int n)
	: CCheckListBox()
	, m_n(n)
{
	for (int i = 0; i < FILTER_TYPE_NB; i++) {
		m_nbFiltersPerType[i] = m_nbChecked[i] = 0;
	}
}

void CPPageInternalFiltersListBox::PreSubclassWindow()
{
	__super::PreSubclassWindow();
	EnableToolTips(TRUE);
}

INT_PTR CPPageInternalFiltersListBox::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	BOOL b = FALSE;
	int row = ItemFromPoint(point, b);

	if (row < 0) {
		return -1;
	}

	CRect r;
	GetItemRect(row, r);
	pTI->rect = r;
	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)row;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->uFlags |= TTF_ALWAYSTIP;

	return pTI->uId;
}

BEGIN_MESSAGE_MAP(CPPageInternalFiltersListBox, CCheckListBox)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CPPageInternalFiltersListBox::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	filter_t* f = (filter_t*)GetItemDataPtr(static_cast<int>(pNMHDR->idFrom));

	if (f->nHintID == 0) {
		return FALSE;
	}

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

	static CString m_strTipText;

	m_strTipText = ResStr(f->nHintID);

	pTTT->lpszText = m_strTipText.GetBuffer();

	*pResult = 0;

	return TRUE;
}

int CPPageInternalFiltersListBox::AddFilter(filter_t* filter, bool checked)
{
	int index = AddString(filter->label);
	// SetItemDataPtr must be called before SetCheck
	SetItemDataPtr(index, filter);
	SetCheck(index, checked);

	return index;
}

void CPPageInternalFiltersListBox::UpdateCheckState()
{
	for (int i = 0; i < FILTER_TYPE_NB; i++) {
		m_nbFiltersPerType[i] = m_nbChecked[i] = 0;
	}

	for (int i = 0; i < GetCount(); i++) {
		filter_t* filter = (filter_t*) GetItemDataPtr(i);

		m_nbFiltersPerType[filter->type]++;

		if (GetCheck(i)) {
			m_nbChecked[filter->type]++;
		}
	}
}

void CPPageInternalFiltersListBox::OnRButtonDown(UINT nFlags, CPoint point)
{
	CCheckListBox::OnRButtonDown(nFlags, point);

	CMenu m;
	m.CreatePopupMenu();

	enum {
		ENABLE_ALL = 1,
		DISABLE_ALL,
		ENABLE_FFMPEG,
		DISABLE_FFMPEG,
		ENABLE_DXVA,
		DISABLE_DXVA,
	};

	int totalFilters = 0, totalChecked = 0;

	for (int i = 0; i < FILTER_TYPE_NB; i++) {
		totalFilters += m_nbFiltersPerType[i];
		totalChecked += m_nbChecked[i];
	}

	UINT state = (totalChecked != totalFilters) ? MF_ENABLED : MF_GRAYED;
	m.AppendMenu(MF_STRING | state, ENABLE_ALL, ResStr(IDS_ENABLE_ALL_FILTERS));
	state = (totalChecked != 0) ? MF_ENABLED : MF_GRAYED;
	m.AppendMenu(MF_STRING | state, DISABLE_ALL, ResStr(IDS_DISABLE_ALL_FILTERS));

	if (m_n) {
		m.AppendMenu(MF_SEPARATOR);
		state = (m_nbChecked[FFMPEG_DECODER] != m_nbFiltersPerType[FFMPEG_DECODER]) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, ENABLE_FFMPEG, ResStr(IDS_ENABLE_FFMPEG_FILTERS));
		state = (m_nbChecked[FFMPEG_DECODER] != 0) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, DISABLE_FFMPEG, ResStr(IDS_DISABLE_FFMPEG_FILTERS));
	}

	if (m_n == VIDEO) {
		m.AppendMenu(MF_SEPARATOR);
		state = (m_nbChecked[DXVA_DECODER] != m_nbFiltersPerType[DXVA_DECODER]) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, ENABLE_DXVA, ResStr(IDS_ENABLE_DXVA_FILTERS));
		state = (m_nbChecked[DXVA_DECODER] != 0) ? MF_ENABLED : MF_GRAYED;
		m.AppendMenu(MF_STRING | state, DISABLE_DXVA, ResStr(IDS_DISABLE_DXVA_FILTERS));
	}

	CPoint p = point;
	::MapWindowPoints(m_hWnd, HWND_DESKTOP, &p, 1);

	UINT id = m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this);

	if (id == 0) {
		return;
	}

	int index = 0;

	for (int i = 0; i < _countof(s_filters); i++) {
		switch (s_filters[i].type) {
			case SOURCE_FILTER:
				if (!(m_n == SOURCE)) {
					continue;
				}
				if (s_filters[i].filter_type != SOURCE) {
					continue;
				}
				break;
			case DECODER:
			case FFMPEG_DECODER:
				if (m_n == SOURCE) {
					continue;
				}
				if (m_n == VIDEO && s_filters[i].filter_type != VIDEO) {
					continue;
				}
				if (m_n == AUDIO && s_filters[i].filter_type != AUDIO) {
					continue;
				}
				break;
			case DXVA_DECODER:
				if (m_n == SOURCE || m_n == AUDIO) {
					continue;
				}
				if (s_filters[i].filter_type != VIDEO) {
					continue;
				}
				break;
			default:
				continue;
		}

		switch (id) {
			case ENABLE_ALL:
				SetCheck(index, TRUE);
				break;
			case DISABLE_ALL:
				SetCheck(index, FALSE);
				break;
			case ENABLE_FFMPEG:
				if (s_filters[i].type == FFMPEG_DECODER) {
					SetCheck(index, TRUE);
				}
				break;
			case DISABLE_FFMPEG:
				if (s_filters[i].type == FFMPEG_DECODER) {
					SetCheck(index, FALSE);
				}
				break;
			case ENABLE_DXVA:
				if (s_filters[i].type == DXVA_DECODER) {
					SetCheck(index, TRUE);
				}
				break;
			case DISABLE_DXVA:
				if (s_filters[i].type == DXVA_DECODER) {
					SetCheck(index, FALSE);
				}
				break;
		}

		index++;
	}

	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CLBN_CHKCHANGE), (LPARAM)m_hWnd);
}

// CPPageInternalFilters dialog

IMPLEMENT_DYNAMIC(CPPageInternalFilters, CPPageBase)
CPPageInternalFilters::CPPageInternalFilters()
	: CPPageBase(CPPageInternalFilters::IDD, CPPageInternalFilters::IDD)
	, m_listSrc(SOURCE)
	, m_listVideo(VIDEO)
	, m_listAudio(AUDIO)
{
}

CPPageInternalFilters::~CPPageInternalFilters()
{
}

void CPPageInternalFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_listSrc);
	DDX_Control(pDX, IDC_LIST2, m_listVideo);
	DDX_Control(pDX, IDC_LIST3, m_listAudio);
	DDX_Control(pDX, IDC_TAB1, m_Tab);
	DDX_Control(pDX, IDC_BUTTON5, m_btnAviCfg);
	DDX_Control(pDX, IDC_BUTTON1, m_btnMpegCfg);
	DDX_Control(pDX, IDC_BUTTON6, m_btnMatroskaCfg);
	DDX_Control(pDX, IDC_BUTTON2, m_btnVideoCfg);
	DDX_Control(pDX, IDC_BUTTON3, m_btnMPEG2Cfg);
	DDX_Control(pDX, IDC_BUTTON4, m_btnAudioCfg);
}

BEGIN_MESSAGE_MAP(CPPageInternalFilters, CPPageBase)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelChange)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelChange)
	ON_LBN_SELCHANGE(IDC_LIST3, OnSelChange)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckBoxChange)
	ON_CLBN_CHKCHANGE(IDC_LIST2, OnCheckBoxChange)
	ON_CLBN_CHKCHANGE(IDC_LIST3, OnCheckBoxChange)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CPPageInternalFilters::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON5, OnAviSplitterConfig)
	ON_BN_CLICKED(IDC_BUTTON1, OnMpegSplitterConfig)
	ON_BN_CLICKED(IDC_BUTTON6, OnMatroskaSplitterConfig)
	ON_BN_CLICKED(IDC_BUTTON2, OnVideoDecConfig)
	ON_BN_CLICKED(IDC_BUTTON3, OnMPEG2DecConfig)
	ON_BN_CLICKED(IDC_BUTTON4, OnAudioDecConfig)
END_MESSAGE_MAP()

// CPPageInternalFilters message handlers

BOOL CPPageInternalFilters::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	for (int i = 0; i < _countof(s_filters)-1; i++) {
		CPPageInternalFiltersListBox* l;
		bool checked = false;

		switch (s_filters[i].type) {
			case SOURCE_FILTER:
				l		= &m_listSrc;
				checked	= s.SrcFilters[s_filters[i].flag];
				break;
			case DECODER:
				l		= (s_filters[i].filter_type == AUDIO) ? &m_listAudio : &m_listVideo;
				checked	= s.TraFilters[s_filters[i].flag];
				break;
			case DXVA_DECODER:
				l		= &m_listVideo;
				checked	= s.DXVAFilters[s_filters[i].flag];
				break;
			case FFMPEG_DECODER:
				l		= (s_filters[i].filter_type == AUDIO) ? &m_listAudio : &m_listVideo;
				checked	= s.FFmpegFilters[s_filters[i].flag];
				break;
			default:
				l		= NULL;
				checked	= false;
		}

		if (l) {
			l->AddFilter(&s_filters[i], checked);
		}
	}

	m_listSrc.UpdateCheckState();
	m_listVideo.UpdateCheckState();
	m_listAudio.UpdateCheckState();

	TC_ITEM tci;
	memset(&tci, 0, sizeof(tci));
	tci.mask = TCIF_TEXT;

	CString TabName	= ResStr(IDS_FILTERS_SOURCE);
	tci.pszText		= TabName.GetBuffer();
	tci.cchTextMax	= TabName.GetLength();
	m_Tab.InsertItem(0, &tci);

	TabName			= ResStr(IDS_FILTERS_VIDEO);
	tci.pszText		= TabName.GetBuffer();
	tci.cchTextMax	= TabName.GetLength();
	m_Tab.InsertItem(1, &tci);

	TabName			= ResStr(IDS_FILTERS_AUDIO);
	tci.pszText		= TabName.GetBuffer();
	tci.cchTextMax	= TabName.GetLength();
	m_Tab.InsertItem(2, &tci);

	NMHDR hdr;
	hdr.code		= TCN_SELCHANGE;
	hdr.hwndFrom	= m_Tab.m_hWnd;

	SendMessage (WM_NOTIFY, m_Tab.GetDlgCtrlID(), (LPARAM)&hdr);

	SetHandCursor(m_hWnd, IDC_BUTTON1);
	SetHandCursor(m_hWnd, IDC_BUTTON2);
	SetHandCursor(m_hWnd, IDC_BUTTON3);
	SetHandCursor(m_hWnd, IDC_BUTTON4);
	SetHandCursor(m_hWnd, IDC_BUTTON5);
	SetHandCursor(m_hWnd, IDC_BUTTON6);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageInternalFilters::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	CPPageInternalFiltersListBox* list = &m_listSrc;
	for (int l=0; l<3; l++) {
		for (int i = 0; i < list->GetCount(); i++) {
			filter_t* f = (filter_t*) list->GetItemDataPtr(i);

			switch (f->type) {
				case SOURCE_FILTER:
					s.SrcFilters[f->flag] = !!list->GetCheck(i);
					break;
				case DECODER:
					s.TraFilters[f->flag] = !!list->GetCheck(i);
					break;
				case DXVA_DECODER:
					s.DXVAFilters[f->flag] = !!list->GetCheck(i);
					break;
				case FFMPEG_DECODER:
					s.FFmpegFilters[f->flag] = !!list->GetCheck(i);
					break;
			}
		}

		list = (l == 1) ? &m_listVideo : &m_listAudio;
	}

	return __super::OnApply();
}

void CPPageInternalFilters::ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr))
{
	if (!CreateInstance) {
		return;
	}

	HRESULT hr;
	CUnknown* pObj = CreateInstance(NULL, &hr);

	if (!pObj) {
		return;
	}

	CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pObj;

	if (SUCCEEDED(hr)) {
		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk) {
			CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);
			ps.AddPages(pSPP);
			ps.DoModal();
		}
	}
}

void CPPageInternalFilters::OnSelChange()
{
}

void CPPageInternalFilters::OnCheckBoxChange()
{
	m_listSrc.UpdateCheckState();
	m_listVideo.UpdateCheckState();
	m_listAudio.UpdateCheckState();

	SetModified();
}


void CPPageInternalFilters::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	switch (m_Tab.GetCurSel()) {
		case SOURCE :
			m_listSrc.ShowWindow(SW_SHOW);
			m_listVideo.ShowWindow(SW_HIDE);
			m_listAudio.ShowWindow(SW_HIDE);

			m_btnAviCfg.ShowWindow(SW_SHOW);
			m_btnMpegCfg.ShowWindow(SW_SHOW);
			m_btnMatroskaCfg.ShowWindow(SW_SHOW);
			m_btnVideoCfg.ShowWindow(SW_HIDE);
			m_btnMPEG2Cfg.ShowWindow(SW_HIDE);
			m_btnAudioCfg.ShowWindow(SW_HIDE);
			break;
		case VIDEO :
			m_listSrc.ShowWindow(SW_HIDE);
			m_listVideo.ShowWindow(SW_SHOW);
			m_listAudio.ShowWindow(SW_HIDE);

			m_btnAviCfg.ShowWindow(SW_HIDE);
			m_btnMpegCfg.ShowWindow(SW_HIDE);
			m_btnMatroskaCfg.ShowWindow(SW_HIDE);
			m_btnVideoCfg.ShowWindow(SW_SHOW);
			m_btnMPEG2Cfg.ShowWindow(SW_SHOW);
			m_btnAudioCfg.ShowWindow(SW_HIDE);
			break;
		case AUDIO :
			m_listSrc.ShowWindow(SW_HIDE);
			m_listVideo.ShowWindow(SW_HIDE);
			m_listAudio.ShowWindow(SW_SHOW);

			m_btnAviCfg.ShowWindow(SW_HIDE);
			m_btnMpegCfg.ShowWindow(SW_HIDE);
			m_btnMatroskaCfg.ShowWindow(SW_HIDE);
			m_btnVideoCfg.ShowWindow(SW_HIDE);
			m_btnMPEG2Cfg.ShowWindow(SW_HIDE);
			m_btnAudioCfg.ShowWindow(SW_SHOW);
			break;
		default:
			break;
	}

	*pResult = 0;
}

void CPPageInternalFilters::OnAviSplitterConfig()
{
	ShowPPage(CreateInstance<CAviSplitterFilter>);
}

void CPPageInternalFilters::OnMpegSplitterConfig()
{
	ShowPPage(CreateInstance<CMpegSplitterFilter>);
}

void CPPageInternalFilters::OnMatroskaSplitterConfig()
{
	ShowPPage(CreateInstance<CMatroskaSplitterFilter>);
}

void CPPageInternalFilters::OnVideoDecConfig()
{
	ShowPPage(CreateInstance<CMPCVideoDecFilter>);
}

void CPPageInternalFilters::OnMPEG2DecConfig()
{
	ShowPPage(CreateInstance<CMpeg2DecFilter>);
}

void CPPageInternalFilters::OnAudioDecConfig()
{
	ShowPPage(CreateInstance<CMpaDecFilter>);
}
