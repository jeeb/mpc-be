/*
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
#include "MPCVideoDecSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"
#include <ffmpeg/libavcodec/avcodec.h>
#include "ffImgfmt.h"

//
// CMPCVideoDecSettingsWnd
//

int g_AVDiscard[] = {
	AVDISCARD_NONE,
	AVDISCARD_DEFAULT,
	AVDISCARD_NONREF,
	AVDISCARD_BIDIR,
	AVDISCARD_NONKEY,
	AVDISCARD_ALL,
};

int FindDiscardIndex(int nValue)
{
	for (int i=0; i<_countof (g_AVDiscard); i++)
		if (g_AVDiscard[i] == nValue) {
			return i;
		}
	return 1;
}

CMPCVideoDecSettingsWnd::CMPCVideoDecSettingsWnd()
{
}

bool CMPCVideoDecSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMDF);

	m_pMDF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMDF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMDF) {
		return false;
	}

	return true;
}

void CMPCVideoDecSettingsWnd::OnDisconnect()
{
	if (m_pMDF) {
		m_pMDF->SetDialogHWND(0);
	}

	m_pMDF.Release();
}

bool CMPCVideoDecSettingsWnd::OnActivate()
{
	ASSERT(IPP_FONTSIZE == 13);
	const int h16 = IPP_SCALE(16);
	const int h20 = IPP_SCALE(20);
	const int h25 = IPP_SCALE(25);
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
	CPoint p(10, 10);
	GUID* DxvaGui = NULL;

	m_grpFFMpeg.Create(ResStr(IDS_VDF_SETTINGS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(IPP_SCALE(350), h20 + h25 * 3 + h20)), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Decoding threads
	m_txtThreadNumber.Create(ResStr(IDS_VDF_THREADNUMBER), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbThreadNumber.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_THREAD_NUMBER);
	m_cbThreadNumber.AddString (ResStr (IDS_VDF_AUTO));
	CString ThreadNumberStr;
	for (int i = 1; i <= 16; i++) {
		ThreadNumberStr.Format(_T("%d"), i);
		m_cbThreadNumber.AddString(ThreadNumberStr);
	}
	p.y += h25;

	// H264 deblocking mode
	m_txtDiscardMode.Create(ResStr(IDS_VDF_SKIPDEBLOCK), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDiscardMode.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DISCARD_MODE);
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONE));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_DEFAULT));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONREF));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_BIDIR));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONKFRM));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_ALL));
	p.y += h25;

	// Deinterlacing
	m_txtDeinterlacing.Create(ResStr(IDS_VDF_DEINTERLACING), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDeinterlacing.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DEINTERLACING);
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_AUTO));
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_DEINTER_TOP));
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_DEINTER_BOTTOM));
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_DEINTER_PROGRESSIVE));
	p.y += h25;

	// Read AR from stream
	m_cbARMode.Create(ResStr(IDS_VDF_AR_MODE), dwStyle | BS_AUTO3STATE | BS_LEFTTEXT, CRect(p, CSize(IPP_SCALE(340), m_fontheight)), this, IDC_PP_AR);
	m_cbARMode.SetCheck(FALSE);
	p.y += h25;

	//
	m_grpDXVA.Create(ResStr(IDS_VDF_DXVA_SETTING), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(IPP_SCALE(350), h20 + h25 +h20 * 3 + m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;

	// DXVA Compatibility check
	m_txtDXVACompatibilityCheck.Create(ResStr(IDS_VDF_DXVACOMPATIBILITY), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(225), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDXVACompatibilityCheck.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DXVA_CHECK);
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_FULLCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_LEVELCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_REFCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_ALLCHECK));
	p.y += h25;

	// Set DXVA for SD (H.264)
	m_cbDXVA_SD.Create(ResStr(IDS_VDF_DXVA_SD), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(IPP_SCALE(340), m_fontheight)), this, IDC_PP_DXVA_SD);
	m_cbDXVA_SD.SetCheck (FALSE);
	p.y += h20;

	// DXVA mode
	m_txtDXVAMode.Create(ResStr(IDS_VDF_DXVA_MODE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(120), m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtDXVAMode.Create(WS_CHILD | WS_VISIBLE | WS_DISABLED, CRect(p + CPoint(IPP_SCALE(120), 0), CSize(IPP_SCALE(220), m_fontheight)), this, 0);
	p.y += h20;

	// Video card description
	m_txtVideoCardDescription.Create(ResStr(IDS_VDF_VIDEOCARD), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(120), m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtVideoCardDescription.Create(ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_DISABLED, CRect(p + CPoint(IPP_SCALE(120), 0), CSize(IPP_SCALE(220), m_fontheight * 2)), this, 0);
	m_edtVideoCardDescription.SetWindowText (m_pMDF->GetVideoCardDescription());

	DxvaGui = m_pMDF->GetDXVADecoderGuid();
	if (DxvaGui != NULL) {
		CString DXVAMode = GetDXVAMode (DxvaGui);
		m_edtDXVAMode.SetWindowText (DXVAMode);
	} else {
		m_txtDXVAMode.ShowWindow (SW_HIDE);
		m_edtDXVAMode.ShowWindow (SW_HIDE);
	}

	// === New swscaler options
	p = CPoint(IPP_SCALE(365), 10);
	int width_s = IPP_SCALE(180);
	int btn_w   = m_fontheight + 12;
	int btn_h   = m_fontheight + 4;
	int combo_w = IPP_SCALE(85);
	int label_w = width_s - combo_w;
	m_grpFmtConv.Create(ResStr(IDS_VDF_COLOR_FMT_CONVERSION), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, IPP_SCALE(225) + m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Software output formats
	m_txtSwOutputFormats.Create(ResStr(IDS_VDF_COLOR_OUTPUT_FORMATS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_lstSwOutputFormats.Create(dwStyle|WS_BORDER|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS, CRect(p, CSize(width_s - btn_w, IPP_SCALE(13 * 6 + 20))), this, 0);

	// Software Output formats order
	m_cbSwOutputFormatUp.Create(_T("\x35"), dwStyle|BS_PUSHBUTTON, CRect(p + CPoint(width_s - btn_w, 0), CSize(btn_w, btn_h)), this, IDC_PP_SWOUTPUTFORMATUP);
	p.y += btn_h;
	m_cbSwOutputFormatDown.Create(_T("\x36"), dwStyle|BS_PUSHBUTTON, CRect(p + CPoint(width_s - btn_w, 0), CSize(btn_w, btn_h)), this, IDC_PP_SWOUTPUTFORMATDOWN);
	p.y += IPP_SCALE(13 * 6 + 9);

	// Preset
	m_txtSwPreset.Create(ResStr(IDS_VDF_COLOR_PRESET), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwPreset.Create (dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWPRESET);
	m_cbSwPreset.AddString(_T("Fastest"));
	m_cbSwPreset.AddString(_T("Fast"));
	m_cbSwPreset.AddString(_T("Normal"));
	m_cbSwPreset.AddString(_T("Full"));
	p.y += h25;

	// Standard (ITU-R Recommendation)
	m_txtSwStandard.Create(ResStr(IDS_VDF_COLOR_STANDARD), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwStandard.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWSTANDARD);
	m_cbSwStandard.AddString(_T("SD (BT.601)"));
	m_cbSwStandard.AddString(_T("HD (BT.709)"));
	m_cbSwStandard.AddString(ResStr(IDS_VDF_AUTO));
	p.y += h25;

	// Input levels
	m_txtSwInputLevels.Create(ResStr(IDS_VDF_COLOR_INPUT_LEVELS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwInputLevels.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWINPUTLEVELS);
	m_cbSwInputLevels.AddString(_T("TV (16-235)"));
	m_cbSwInputLevels.AddString(_T("PC (0-255)"));
	m_cbSwInputLevels.AddString(ResStr(IDS_VDF_AUTO));
	p.y += h25;

	// Output levels
	m_txtSwOutputLevels.Create(ResStr(IDS_VDF_COLOR_OUTPUT_LEVELS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwOutputLevels.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWOUTPUTLEVELS);
	m_cbSwOutputLevels.AddString(_T("TV (16-235)"));
	m_cbSwOutputLevels.AddString(_T("PC (0-255)"));
	m_cbSwOutputLevels.AddString(ResStr(IDS_VDF_AUTO));
	p.y += h25;

	// Software version, useful info for stand-alone filter
	m_strSwVersion.Format(_T("v%d.%d.%d.%d"),MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_PATCH,MPC_VERSION_STATUS);
	m_txtSwVersion.Create(m_strSwVersion, WS_DISABLED|WS_VISIBLE|WS_CHILD|SS_RIGHT|SS_PATHELLIPSIS, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	//

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}
	LOGFONT lf = {m_fontheight*7/4,0L,0L,0L,FW_NORMAL,0,0,0,SYMBOL_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE|DEFAULT_PITCH,_T("Webdings")};
	m_arrowsFont.CreateFontIndirect(&lf);
	m_cbSwOutputFormatUp.SetFont(&m_arrowsFont);
	m_cbSwOutputFormatDown.SetFont(&m_arrowsFont);

	CorrectComboListWidth(m_cbDXVACompatibilityCheck);
	CorrectComboListWidth(m_cbDiscardMode);

	if (m_pMDF) {
		m_pMDF->SetDialogHWND(this->GetSafeHwnd());

		m_cbThreadNumber.SetCurSel		(m_pMDF->GetThreadNumber());
		m_cbDiscardMode.SetCurSel		(FindDiscardIndex (m_pMDF->GetDiscardMode()));
		m_cbDeinterlacing.SetCurSel		((int)m_pMDF->GetDeinterlacing());

		m_cbARMode.SetCheck(m_pMDF->GetARMode());

		m_cbDXVACompatibilityCheck.SetCurSel(m_pMDF->GetDXVACheckCompatibility());
		m_cbDXVA_SD.SetCheck(m_pMDF->GetDXVA_SD());

		// === New swscaler options
		int k = 0;
		while (LPCTSTR str = m_pMDF->GetSwFormatName(k)) {
			m_lstSwOutputFormats.AddString(str);
			int nCheck = m_pMDF->GetSwFormatState(k);
			m_lstSwOutputFormats.SetCheck(k, nCheck);
			m_lstSwOutputFormats.SetItemData(k, 10 * k + nCheck); // remember the original order and check state
			k++;
		}
		m_cbSwPreset.SetCurSel(m_pMDF->GetSwPreset());
		m_cbSwStandard.SetCurSel(m_pMDF->GetSwStandard());
		m_cbSwInputLevels.SetCurSel(m_pMDF->GetSwInputLevels());
		m_cbSwOutputLevels.SetCurSel(m_pMDF->GetSwOutputLevels());

		unsigned __int64 m_nOutCsp = m_pMDF->GetOutputFormat();

		m_lstSwOutputFormats.EnableWindow(m_nOutCsp != FF_CSP_UNSUPPORTED);
		m_cbSwOutputFormatUp.EnableWindow(m_nOutCsp != FF_CSP_UNSUPPORTED);
		m_cbSwOutputFormatDown.EnableWindow(m_nOutCsp != FF_CSP_UNSUPPORTED);

		m_cbSwPreset.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		m_cbSwStandard.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		m_cbSwInputLevels.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		m_cbSwOutputLevels.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		//
	}

	SetClassLongPtr(m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	SetClassLongPtr(GetDlgItem(IDC_PP_THREAD_NUMBER)->m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_HAND));

	EnableToolTips(TRUE);

	SetDirty(false);

	return true;
}

void CMPCVideoDecSettingsWnd::OnDeactivate()
{
}

bool CMPCVideoDecSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
		m_pMDF->SetThreadNumber		(m_cbThreadNumber.GetCurSel());
		m_pMDF->SetDiscardMode		(g_AVDiscard[m_cbDiscardMode.GetCurSel()]);
		m_pMDF->SetDeinterlacing	((MPC_DEINTERLACING_FLAGS)m_cbDeinterlacing.GetCurSel());

		m_pMDF->SetARMode(m_cbARMode.GetCheck());

		m_pMDF->SetDXVACheckCompatibility(m_cbDXVACompatibilityCheck.GetCurSel());

		m_pMDF->SetDXVA_SD(m_cbDXVA_SD.GetCheck());

		// === New swscaler options
		int refresh = 0; // no refresh

		if (m_cbSwPreset.GetCurSel() != m_pMDF->GetSwPreset()
				|| m_cbSwStandard.GetCurSel() != m_pMDF->GetSwStandard()
				|| m_cbSwInputLevels.GetCurSel() != m_pMDF->GetSwInputLevels()
				|| m_cbSwOutputLevels.GetCurSel() != m_pMDF->GetSwOutputLevels()) {
			refresh = 1; // soft refresh - signal new swscaler colorspace details
		}

		CMediaType mt;
		m_pMDF->GetOutputMediaType(&mt);
		CString OutputType = GetGUIDString(mt.subtype);
		OutputType.Replace(L"MEDIASUBTYPE_", L"");
		FreeMediaType(mt);

		CString SettingsOutputType;
		m_lstSwOutputFormats.GetText(0, SettingsOutputType);

		if (OutputType != SettingsOutputType
				|| m_lstSwOutputFormats.GetCheck(0) != m_lstSwOutputFormats.GetItemData(0)) {
			refresh = 2; // hard refresh - signal new output format
		}
		
		m_pMDF->SetSwRefresh(refresh);

		if (refresh < 2) {
			for (int i = 0; i < m_lstSwOutputFormats.GetCount(); i++) {
				if (i * 10 + m_lstSwOutputFormats.GetCheck(i) != m_lstSwOutputFormats.GetItemData(i)) {
					refresh = 2;
					break;
				}
			}
		}

		if (refresh == 2) {
			CString SwFormatsStr;
			for (int i = 0; i < m_lstSwOutputFormats.GetCount(); i++) {
				CString name;
				m_lstSwOutputFormats.GetText(i, name);
				SwFormatsStr.AppendFormat(_T("%s%s;"), name, m_lstSwOutputFormats.GetCheck(i) ? _T("+") : _T("-"));
			}
			m_pMDF->SetSwFormats(SwFormatsStr);

			// re-build output formats
			m_lstSwOutputFormats.ResetContent();
			int k = 0;
			while (LPCTSTR str = m_pMDF->GetSwFormatName(k)) {
				m_lstSwOutputFormats.AddString(str);
				int nCheck = m_pMDF->GetSwFormatState(k);
				m_lstSwOutputFormats.SetCheck(k, nCheck);
				m_lstSwOutputFormats.SetItemData(k, 10 * k + nCheck); // remember the original order and check state
				k++;
			}
		}

		if (refresh >= 1) {
			m_pMDF->SetSwPreset(m_cbSwPreset.GetCurSel());
			m_pMDF->SetSwStandard(m_cbSwStandard.GetCurSel());
			m_pMDF->SetSwInputLevels(m_cbSwInputLevels.GetCurSel());
			m_pMDF->SetSwOutputLevels(m_cbSwOutputLevels.GetCurSel());
		}
		//

		m_pMDF->Apply();
	}

	return true;
}


BEGIN_MESSAGE_MAP(CMPCVideoDecSettingsWnd, CInternalPropertyPageWnd)
	// === New swscaler options
	ON_BN_CLICKED( IDC_PP_SWOUTPUTFORMATUP, OnClickedSwOutputFormatUp)
	ON_BN_CLICKED( IDC_PP_SWOUTPUTFORMATDOWN, OnClickedSwOutputFormatDown)
	//
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()

// === New swscaler options
void CMPCVideoDecSettingsWnd::OnClickedSwOutputFormatUp()
{
	int pos = m_lstSwOutputFormats.GetCurSel();
	int count = m_lstSwOutputFormats.GetCount();
	int selected;	int selectedUp; 
	CString text;
	if ((pos != LB_ERR) && (count > 1) && (pos-1 >= 0)) {
		selected = m_lstSwOutputFormats.GetCheck(pos);
		selectedUp = m_lstSwOutputFormats.GetCheck(pos-1);
		m_lstSwOutputFormats.GetText(pos, text);        
		m_lstSwOutputFormats.DeleteString(pos);
		m_lstSwOutputFormats.InsertString(pos-1, text);
		m_lstSwOutputFormats.SetCheck(pos-1, selected);
		m_lstSwOutputFormats.SetCheck(pos, selectedUp);
		m_lstSwOutputFormats.SetCurSel(pos-1);
	}
}

void CMPCVideoDecSettingsWnd::OnClickedSwOutputFormatDown()
{
	int pos		= m_lstSwOutputFormats.GetCurSel();
	int count	= m_lstSwOutputFormats.GetCount();
	int selected;
	int selectedDown; 
	CString text;

	if ((pos != LB_ERR) && (count > 1) && (pos+1 < count)) {
		selected = m_lstSwOutputFormats.GetCheck(pos);
		selectedDown = m_lstSwOutputFormats.GetCheck(pos+1);
		m_lstSwOutputFormats.GetText(pos,text);
		m_lstSwOutputFormats.DeleteString(pos);
		m_lstSwOutputFormats.InsertString(pos+1,text);
		m_lstSwOutputFormats.SetCheck(pos+1,selected);
		m_lstSwOutputFormats.SetCheck(pos,selectedDown);
		m_lstSwOutputFormats.SetCurSel(pos+1);
	}
}
//

BOOL CMPCVideoDecSettingsWnd::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	CToolTipCtrl* pToolTip = AfxGetModuleThreadState()->m_pToolTip;
	if (pToolTip) {
		pToolTip->SetMaxTipWidth(SHRT_MAX);
	}

	UINT_PTR nID = pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND) {
		nID = ::GetDlgCtrlID((HWND)nID);
		if (nID == IDC_PP_AR) {
			pTTT->lpszText = _T("Checked - will be used AR from stream.\nUnchecked - will be used AR from container.\nIndeterminate - AR from stream will not be used on files with a container AR (recommended).");
			*pResult = 0;

			return TRUE;
		}
	}

	return FALSE;
}

// ====== Codec filter property page (for standalone filter only)

CMPCVideoDecCodecWnd::CMPCVideoDecCodecWnd()
{
}

bool CMPCVideoDecCodecWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMDF);

	m_pMDF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMDF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMDF) {
		return false;
	}

	return true;
}

void CMPCVideoDecCodecWnd::OnDisconnect()
{
	m_pMDF.Release();
}

typedef struct {
	ULONGLONG	CodecId;
	LPCTSTR		CodeName;
} MPCFILTER_VIDEO_CODECS;

MPCFILTER_VIDEO_CODECS mpc_codecs[] = {
	{MPCVD_H264_DXVA,	_T("H.264/AVC (DXVA)")},
	{MPCVD_MPEG2_DXVA,	_T("MPEG-2 (DXVA)")},
	{MPCVD_VC1_DXVA,	_T("VC1 (DXVA)")},
	{MPCVD_WMV3_DXVA,	_T("WMV3 (DXVA)")},
	{MPCVD_AMVV,		_T("AMV video")},
	{MPCVD_PRORES,		_T("Apple ProRes")},
	{MPCVD_BINKV,		_T("Bink video")},
	{MPCVD_CLLC,		_T("Canopus Lossless")},
	{MPCVD_DIRAC,		_T("Dirac")},
	{MPCVD_DIVX,		_T("DivX")},
	{MPCVD_DV,			_T("DV video")},
	{MPCVD_FLASH,		_T("FLV1/4")},
	{MPCVD_H263,		_T("H.263")},
	{MPCVD_H264,		_T("H.264/AVC (FFmpeg)")},
	{MPCVD_HEVC,		_T("HEVC")},
	{MPCVD_INDEO,		_T("Indeo 3/4/5")},
	{MPCVD_LAGARITH,	_T("Lagarith")},
	{MPCVD_MJPEG,		_T("MJPEG")},
	{MPCVD_MPEG1,		_T("MPEG-1 (FFmpeg)")},
	{MPCVD_MPEG2,		_T("MPEG-2 (FFmpeg)")},
	{MPCVD_MSMPEG4,		_T("MS-MPEG4")},
	{MPCVD_PNG,			_T("PNG")},
	{MPCVD_SCREC,		_T("Screen Recorder (CSCD/TSCC/QTRle)")},
	{MPCVD_SVQ3,		_T("SVQ1/3")},
	{MPCVD_THEORA,		_T("Theora")},
	{MPCVD_UTVD,		_T("Ut video")},
	{MPCVD_VC1,			_T("VC1 (FFmpeg)")},
	{MPCVD_VP356,		_T("VP3/5/6")},
	{MPCVD_VP8,			_T("VP8")},
	{MPCVD_WMV,			_T("WMV1/2/3")},
	{MPCVD_XVID,		_T("Xvid/MPEG-4")},
	{MPCVD_RV,			_T("Real Video")},
	{MPCVD_V210,		_T("Uncompressed video (v210)")},
};

bool CMPCVideoDecCodecWnd::OnActivate()
{
	DWORD dwStyle			= WS_VISIBLE|WS_CHILD|WS_BORDER;
	int nPos				= 0;
	ULONGLONG nActiveCodecs	= m_pMDF ? m_pMDF->GetActiveCodecs() : 0;

	m_grpSelectedCodec.Create (_T("Selected codecs"), WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  10, 330, 280), this, (UINT)IDC_STATIC);

	m_lstCodecs.Create (dwStyle | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP, CRect (20,30, 320, 270), this, 0);

	for (size_t i = 0; i < _countof(mpc_codecs); i++) {
		m_lstCodecs.AddString(mpc_codecs[i].CodeName);
		m_lstCodecs.SetCheck(nPos++, (nActiveCodecs & mpc_codecs[i].CodecId) != 0);
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	return true;
}

void CMPCVideoDecCodecWnd::OnDeactivate()
{
}

bool CMPCVideoDecCodecWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
		ULONGLONG nActiveCodecs	= 0;
		int nPos				= 0;

		for (size_t i = 0; i < _countof(mpc_codecs); i++) {
			if (m_lstCodecs.GetCheck(nPos++)) {
				nActiveCodecs |= mpc_codecs[i].CodecId;
			}
		}

		m_pMDF->SetActiveCodecs(nActiveCodecs);

		m_pMDF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMPCVideoDecCodecWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()
