/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
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
#include "mplayerc.h"
#include "PPageInternalFilters.h"
#include "ComPropertySheet.h"
#include "../../filters/Filters.h"
#include "InternalFiltersConfig.h"


static filter_t s_filters[] = {

// Source filters
#if INTERNAL_SOURCEFILTER_AVI
	{_T("AVI"), SOURCE_FILTER, SOURCE, SRC_AVI, IDS_SRC_AVI, NULL},
#endif
#if INTERNAL_SOURCEFILTER_CDDA
	{_T("CDDA (Audio CD)"), SOURCE_FILTER, SOURCE, SRC_CDDA, IDS_SRC_CDDA, NULL},
#endif
#if INTERNAL_SOURCEFILTER_CDXA
	{_T("CDXA (VCD/SVCD/XCD)"), SOURCE_FILTER, SOURCE, SRC_CDXA, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_DSM
	{_T("DirectShow Media"), SOURCE_FILTER, SOURCE, SRC_DSM, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_DTSAC3
	{_T("DTS/AC3"), SOURCE_FILTER, SOURCE, SRC_DTSAC3, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_VTS
	{_T("DVD Video Title Set"), SOURCE_FILTER, SOURCE, SRC_VTS, IDS_SRC_VTS, NULL},
#endif
#if INTERNAL_SOURCEFILTER_DVSOURCE
	{_T("DVD2AVI Project File"), SOURCE_FILTER, SOURCE, SRC_D2V, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_FLIC
	{_T("FLI/FLC"), SOURCE_FILTER, SOURCE, SRC_FLIC, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_FLAC
	{_T("FLAC"), SOURCE_FILTER, SOURCE, SRC_FLAC, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_FLV
	{_T("FLV"), SOURCE_FILTER, SOURCE, SRC_FLV, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_MATROSKA
	{_T("Matroska"), SOURCE_FILTER, SOURCE, SRC_MATROSKA, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_MP4
	{_T("MP4/MOV"), SOURCE_FILTER, SOURCE, SRC_MP4, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_MPEGAUDIO
	{_T("MPEG Audio"), SOURCE_FILTER, SOURCE, SRC_MPA, IDS_SRC_MPA, NULL},
#endif
#if INTERNAL_SOURCEFILTER_MPEG
	{_T("MPEG PS/TS/PVA"), SOURCE_FILTER, SOURCE, SRC_MPEG, 0, CreateInstance<CMpegSplitterFilter>},
#endif
#if INTERNAL_SOURCEFILTER_OGG
	{_T("Ogg"), SOURCE_FILTER, SOURCE, SRC_OGG, 0, NULL},
#endif
#if INTERNAL_SOURCEFILTER_REALMEDIA
	{_T("RealMedia"), SOURCE_FILTER, SOURCE, SRC_REALMEDIA, IDS_SRC_REALMEDIA, NULL},
#endif
#if INTERNAL_SOURCEFILTER_SHOUTCAST
	{_T("SHOUTcast"), SOURCE_FILTER, SOURCE, SRC_SHOUTCAST, 0, NULL},
#endif

// Audio decoder
#if INTERNAL_DECODER_AAC
	{_T("AAC"), FFMPEG_DECODER, AUDIO, FFM_AAC, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_AC3
	{_T("AC3"), DECODER, AUDIO, TRA_AC3, IDS_TRA_AC3, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_DTS
	{_T("DTS"), DECODER, AUDIO, TRA_DTS, IDS_TRA_DTS, CreateInstance<CMpaDecFilter>},
	{_T("LPCM"), DECODER, AUDIO, TRA_LPCM, IDS_TRA_LPCM, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_MPEGAUDIO
	{_T("MPEG Audio"), FFMPEG_DECODER, AUDIO, FFM_MPA, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_VORBIS
	{_T("Vorbis"), FFMPEG_DECODER, AUDIO, FFM_VORBIS, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_FLAC
	{_T("FLAC"), DECODER, AUDIO, TRA_FLAC, 0, NULL /* TODO: CreateInstance<CMpaDecFilter>*/},
#endif
#if INTERNAL_DECODER_NELLYMOSER
	{_T("Nellymoser"), FFMPEG_DECODER, AUDIO, FFM_NELLY, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_ALAC
	{_T("ALAC"), FFMPEG_DECODER, AUDIO, FFM_ALAC, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_ALS
	{_T("ALS"), FFMPEG_DECODER, AUDIO, FFM_ALS, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_AMR
	{_T("AMR"), FFMPEG_DECODER, AUDIO, FFM_AMR, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_REALAUDIO
	{_T("RealAudio"), DECODER, AUDIO, TRA_RA, IDS_TRA_RA, NULL},
#endif
#if INTERNAL_DECODER_PS2AUDIO
	{_T("PS2 Audio (PCM/ADPCM)"), DECODER, AUDIO, TRA_PS2AUD, IDS_TRA_PS2AUD, CreateInstance<CMpaDecFilter>},
#endif
#if INTERNAL_DECODER_PCM
	{_T("Other PCM/ADPCM"), DECODER, AUDIO, TRA_PCM, IDS_TRA_FFMPEG, CreateInstance<CMpaDecFilter>},
#endif

// DXVA decoder
#if INTERNAL_DECODER_H264_DXVA
	{_T("H264/AVC (DXVA)"), DXVA_DECODER, VIDEO, TRA_DXVA_H264, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_VC1_DXVA
	{_T("VC1 (DXVA)"), DXVA_DECODER, VIDEO, TRA_DXVA_VC1, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_WMV3_DXVA
	{_T("WMV3 (DXVA)"), DXVA_DECODER, VIDEO, TRA_DXVA_WMV3, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_MPEG2_DXVA
	{_T("MPEG-2 Video (DXVA)"), DXVA_DECODER, VIDEO, TRA_DXVA_MPEG2, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif

// Video Decoder
#if INTERNAL_DECODER_MPEG1
	{_T("MPEG-1 Video"), DECODER, VIDEO, TRA_MPEG1, IDS_TRA_MPEG1, CreateInstance<CMpeg2DecFilter>},
#endif
#if INTERNAL_DECODER_MPEG2
	{_T("MPEG-2 Video"), DECODER, VIDEO, TRA_MPEG2, IDS_TRA_MPEG2, CreateInstance<CMpeg2DecFilter>},
#endif
#if INTERNAL_DECODER_REALVIDEO
	{_T("RealVideo"), DECODER, VIDEO, TRA_RV, IDS_TRA_RV, NULL},
#endif

#if INTERNAL_DECODER_H264
	{_T("H264/AVC (FFmpeg)"), FFMPEG_DECODER, VIDEO, FFM_H264, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_VC1
	{_T("VC1 (FFmpeg)"), FFMPEG_DECODER, VIDEO, FFM_VC1, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_XVID
	{_T("Xvid/MPEG-4"), FFMPEG_DECODER, VIDEO, FFM_XVID, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_DIVX
	{_T("DivX"), FFMPEG_DECODER, VIDEO, FFM_DIVX, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_MSMPEG4
	{_T("MS MPEG-4"), FFMPEG_DECODER, VIDEO, FFM_MSMPEG4, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_FLV
	{_T("FLV1/4"), FFMPEG_DECODER, VIDEO, FFM_FLV4, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_VP356
	{_T("VP3/5/6"), FFMPEG_DECODER, VIDEO, FFM_VP356, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_VP8
	{_T("VP8"), FFMPEG_DECODER, VIDEO, FFM_VP8, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_WMV
	{_T("WMV1/2/3"), FFMPEG_DECODER, VIDEO, FFM_WMV, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_SVQ
	{_T("SVQ1/3"), FFMPEG_DECODER, VIDEO, FFM_SVQ3, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_H263
	{_T("H263"), FFMPEG_DECODER, VIDEO, FFM_H263, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_AMVV
	{_T("AMV video"), FFMPEG_DECODER, VIDEO, FFM_AMVV, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_THEORA
	{_T("Theora"), FFMPEG_DECODER, VIDEO, FFM_THEORA, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_MJPEG
	{_T("MJPEG"), FFMPEG_DECODER, VIDEO, FFM_MJPEG, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif
#if INTERNAL_DECODER_INDEO
	{_T("Indeo 3/4/5"), FFMPEG_DECODER, VIDEO, FFM_INDEO, IDS_TRA_FFMPEG, CreateInstance<CMPCVideoDecFilter>},
#endif

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
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CPPageInternalFiltersListBox::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	filter_t* f = (filter_t*)GetItemDataPtr(pNMHDR->idFrom);
	if (f->nHintID == 0) {
		return FALSE;
	}

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

	static CString m_strTipText;

	m_strTipText = ResStr(f->nHintID);

	pTTT->lpszText = m_strTipText.GetBuffer();

	*pResult = 0;

	return TRUE;    // message was handled
}

void CPPageInternalFiltersListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	CFont* pOldFont = NULL;

	if ((lpDrawItemStruct->itemData != 0) && ((filter_t*)lpDrawItemStruct->itemData)->CreateInstance) {
		if (!(HFONT)m_bold) {
			CFont* pFont = pDC->GetCurrentFont();

			LOGFONT lf;
			pFont->GetLogFont(&lf);
			lf.lfWeight = FW_BOLD;

			m_bold.CreateFontIndirect(&lf);
		}

		if ((HFONT)m_bold) {
			pOldFont = pDC->SelectObject(&m_bold);
		}
	}

	__super::DrawItem(lpDrawItemStruct);

	if (pOldFont) {
		pDC->SelectObject(pOldFont);
	}
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
	for (int i = 0; i < countof(s_filters); i++) {
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
}

BEGIN_MESSAGE_MAP(CPPageInternalFilters, CPPageBase)
	ON_LBN_DBLCLK(IDC_LIST1, OnLbnDblclkList1)
	ON_LBN_DBLCLK(IDC_LIST2, OnLbnDblclkList2)
	ON_LBN_DBLCLK(IDC_LIST3, OnLbnDblclkList3)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelChange)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelChange)
	ON_LBN_SELCHANGE(IDC_LIST3, OnSelChange)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckBoxChange)
	ON_CLBN_CHKCHANGE(IDC_LIST2, OnCheckBoxChange)
	ON_CLBN_CHKCHANGE(IDC_LIST3, OnCheckBoxChange)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CPPageInternalFilters::OnTcnSelchangeTab1)
END_MESSAGE_MAP()

// CPPageInternalFilters message handlers

BOOL CPPageInternalFilters::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	for (int i = 0; i < countof(s_filters)-1; i++) {
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

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
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

void CPPageInternalFilters::ShowPPage(CPPageInternalFiltersListBox& l)
{
	int i = l.GetCurSel();
	if (i < 0) {
		return;
	}

	filter_t* f = (filter_t*)l.GetItemDataPtr(i);
	if (!f || !f->CreateInstance) {
		return;
	}

	HRESULT hr;
	CUnknown* pObj = f->CreateInstance(NULL, &hr);
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

void CPPageInternalFilters::OnLbnDblclkList1()
{
	ShowPPage(m_listSrc);
}

void CPPageInternalFilters::OnLbnDblclkList2()
{
	ShowPPage(m_listVideo);
}

void CPPageInternalFilters::OnLbnDblclkList3()
{
	ShowPPage(m_listAudio);
}

void CPPageInternalFilters::OnSelChange()
{
	// We only catch the message so that the page is not marked as modified.
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
			break;
		case VIDEO :
			m_listSrc.ShowWindow(SW_HIDE);
			m_listVideo.ShowWindow(SW_SHOW);
			m_listAudio.ShowWindow(SW_HIDE);
			break;
		case AUDIO :
			m_listSrc.ShowWindow(SW_HIDE);
			m_listVideo.ShowWindow(SW_HIDE);
			m_listAudio.ShowWindow(SW_SHOW);
			break;
		default:
			break;
	}

	*pResult = 0;
}
