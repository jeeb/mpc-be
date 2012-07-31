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
#include "PPageTweaks.h"
#include "MainFrm.h"


// CPPageTweaks dialog

IMPLEMENT_DYNAMIC(CPPageTweaks, CPPageBase)
CPPageTweaks::CPPageTweaks()
	: CPPageBase(CPPageTweaks::IDD, CPPageTweaks::IDD)
//	, m_fDisableXPToolbars(FALSE)
//	, m_nThemeBrightness(15)
//	, m_nThemeRed(256)
//	, m_nThemeGreen(256)
//	, m_nThemeBlue(256)
//	, m_fFileNameOnSeekBar(TRUE)
//	, m_clrFaceABGR(0x00ffffff)
//	, m_clrOutlineABGR(0x00868686)
	, m_nJumpDistS(0)
	, m_nJumpDistM(0)
	, m_nJumpDistL(0)
//	, m_OSD_Size(0)
	, m_fNotifyMSN(TRUE)
	, m_fPreventMinimize(FALSE)
//	, m_fUseWin7TaskBar(TRUE)
	, m_fDontUseSearchInFolder(FALSE)
	, m_fLCDSupport(FALSE)
{
}

CPPageTweaks::~CPPageTweaks()
{
}

void CPPageTweaks::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

//	DDX_Check(pDX, IDC_CHECK3, m_fDisableXPToolbars);
//	DDX_Control(pDX, IDC_CHECK3, m_fDisableXPToolbarsCtrl);
//	DDX_Slider(pDX, IDC_SLIDER1, m_nThemeBrightness);
//	DDX_Slider(pDX, IDC_SLIDER2, m_nThemeRed);
//	DDX_Slider(pDX, IDC_SLIDER3, m_nThemeGreen);
//	DDX_Slider(pDX, IDC_SLIDER4, m_nThemeBlue);
//	DDX_Control(pDX, IDC_SLIDER1, m_ThemeBrightnessCtrl);
//	DDX_Control(pDX, IDC_SLIDER2, m_ThemeRedCtrl);
//	DDX_Control(pDX, IDC_SLIDER3, m_ThemeGreenCtrl);
//	DDX_Control(pDX, IDC_SLIDER4, m_ThemeBlueCtrl);
//	DDX_Check(pDX, IDC_CHECK5, m_fFileNameOnSeekBar);
	DDX_Text(pDX, IDC_EDIT1, m_nJumpDistS);
	DDX_Text(pDX, IDC_EDIT2, m_nJumpDistM);
	DDX_Text(pDX, IDC_EDIT3, m_nJumpDistL);
	DDX_Check(pDX, IDC_CHECK4, m_fNotifyMSN);
	DDX_Check(pDX, IDC_CHECK6, m_fPreventMinimize);
//	DDX_Check(pDX, IDC_CHECK_WIN7, m_fUseWin7TaskBar);
	DDX_Check(pDX, IDC_CHECK7, m_fDontUseSearchInFolder);
//	DDX_Check(pDX, IDC_CHECK8, m_fUseTimeTooltip);
//	DDX_Control(pDX, IDC_COMBO3, m_TimeTooltipPosition);
	DDX_Control(pDX, IDC_COMBO1, m_FontType);
	DDX_Control(pDX, IDC_COMBO2, m_FontSize);
//	DDX_Check(pDX, IDC_CHECK1, m_fFastSeek);
	DDX_Check(pDX, IDC_CHECK_LCD, m_fLCDSupport);
//	DDX_Check(pDX, IDC_CHECK_PRV, m_fSmartSeek);
}

int CALLBACK EnumFontProc(ENUMLOGFONT FAR* lf, NEWTEXTMETRIC FAR* tm, int FontType, LPARAM dwData)
{
	CAtlArray<CString>* fntl = (CAtlArray<CString>*)dwData;

	if (FontType == TRUETYPE_FONTTYPE) {
		fntl->Add(lf->elfFullName);
	}

	return true;
}

BOOL CPPageTweaks::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

//	m_fDisableXPToolbars	= s.fDisableXPToolbars;
//	m_nThemeBrightness		= m_nThemeBrightness_Old	= s.nThemeBrightness;
//	m_nThemeRed				= m_nThemeRed_Old			= s.nThemeRed;
//	m_nThemeGreen			= m_nThemeGreen_Old			= s.nThemeGreen;
//	m_nThemeBlue			= m_nThemeBlue_Old			= s.nThemeBlue;
//	m_ThemeBrightnessCtrl.SetRange(0, 100);
//	m_ThemeRedCtrl.SetRange(0, 255);
//	m_ThemeGreenCtrl.SetRange(0, 255);
//	m_ThemeBlueCtrl.SetRange(0, 255);
//	m_fFileNameOnSeekBar	= s.fFileNameOnSeekBar;
//	m_clrFaceABGR			= s.clrFaceABGR;
//	m_clrOutlineABGR		= s.clrOutlineABGR;
	m_nJumpDistS			= s.nJumpDistS;
	m_nJumpDistM			= s.nJumpDistM;
	m_nJumpDistL			= s.nJumpDistL;
	m_fNotifyMSN			= s.fNotifyMSN;

	m_fPreventMinimize			= s.fPreventMinimize;
//	m_fUseWin7TaskBar			= s.fUseWin7TaskBar;
	m_fDontUseSearchInFolder	=s.fDontUseSearchInFolder;

//	m_fUseTimeTooltip = s.fUseTimeTooltip;
//	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_ABOVE));
//	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_BELOW));
//	m_TimeTooltipPosition.SetCurSel(s.nTimeTooltipPosition);
//	m_TimeTooltipPosition.EnableWindow(m_fUseTimeTooltip);

//	m_OSD_Size	= s.nOSDSize;
//	m_OSD_Font	= s.strOSDFont;

//	m_fFastSeek		= s.fFastSeek;
	m_fLCDSupport	= s.fLCDSupport;
//	m_fSmartSeek	= s.fSmartSeek;

	m_FontType.Clear();
	m_FontSize.Clear();
	HDC dc = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	CAtlArray<CString> fntl;
	EnumFontFamilies(dc, NULL,(FONTENUMPROC)EnumFontProc, (LPARAM)&fntl);
	DeleteDC(dc);

	for (size_t i = 0; i < fntl.GetCount(); ++i) {
		if (i > 0 && fntl[i-1] == fntl[i]) {
			continue;
		}

		m_FontType.AddString(fntl[i]);
	}

//	CorrectComboListWidth(m_FontType);
//	int iSel = m_FontType.FindStringExact(0, m_OSD_Font);
//	if (iSel == CB_ERR) iSel = 0;
//	m_FontType.SetCurSel(iSel);

	CString str;

//	for (int i = 10; i < 26; ++i) {
//		str.Format(_T("%d"), i);
//		m_FontSize.AddString(str);

//		if (m_OSD_Size == i) {
//			iSel = i;
//		}
//	}

//	m_FontSize.SetCurSel(iSel - 10);

	EnableToolTips(TRUE);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageTweaks::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	bool m_fToolbarRefresh = false;

//	if (s.fDisableXPToolbars == true && m_fDisableXPToolbars == FALSE) {
//		m_fToolbarRefresh = true;
//	}

//	if (s.fDisableXPToolbars == false && m_fDisableXPToolbars == TRUE) {
//		m_fToolbarRefresh = true;
//	}

//	if (s.clrFaceABGR != m_clrFaceABGR || s.clrOutlineABGR != m_clrOutlineABGR) {
//		m_fToolbarRefresh = true;
//	}

//	if (s.nThemeBrightness != m_nThemeBrightness 
//		|| s.nThemeRed != m_nThemeRed
//		|| s.nThemeGreen != m_nThemeGreen
//		|| s.nThemeBlue != m_nThemeBlue
//		|| s.fFileNameOnSeekBar != !!m_fFileNameOnSeekBar) {
//		m_fToolbarRefresh=true;
//	}

//	if (m_fToolbarRefresh) {
//		s.clrFaceABGR		= m_clrFaceABGR;
//		s.clrOutlineABGR	= m_clrOutlineABGR;
//		s.nThemeBrightness	= m_nThemeBrightness;
//		s.nThemeRed			= m_nThemeRed;
//		s.nThemeGreen		= m_nThemeGreen;
//		s.nThemeBlue		= m_nThemeBlue;
//		s.fFileNameOnSeekBar = !!m_fFileNameOnSeekBar;

//		HWND WndToolBar = ((CMainFrame*)AfxGetMainWnd())->m_hWnd_toolbar;

//		if (::IsWindow(WndToolBar)) {
//			s.fToolbarRefresh = true;

//			::PostMessage(WndToolBar, WM_SIZE, SIZE_RESTORED, MAKELPARAM(320, 240));
//		}
//	}

//	s.fDisableXPToolbars	= !!m_fDisableXPToolbars;
	s.nJumpDistS			= m_nJumpDistS;
	s.nJumpDistM			= m_nJumpDistM;
	s.nJumpDistL			= m_nJumpDistL;
	s.fNotifyMSN			= !!m_fNotifyMSN;

	s.fPreventMinimize		= !!m_fPreventMinimize;
//	s.fUseWin7TaskBar		= !!m_fUseWin7TaskBar;
//	s.fUseTimeTooltip		= !!m_fUseTimeTooltip;
//	s.nTimeTooltipPosition	= m_TimeTooltipPosition.GetCurSel();
//	s.nOSDSize				= m_OSD_Size;

	s.fDontUseSearchInFolder	= !!m_fDontUseSearchInFolder;
	m_FontType.GetLBText(m_FontType.GetCurSel(),s.strOSDFont);

//	s.fFastSeek		= !!m_fFastSeek;
	s.fLCDSupport	= !!m_fLCDSupport;
//	s.fSmartSeek	= !!m_fSmartSeek;

	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());

//	if (m_fUseWin7TaskBar) {
//		pFrame->CreateThumbnailToolbar();
//	}

	pFrame->UpdateThumbarButton();
	pFrame->Invalidate();

	return __super::OnApply();
}

void CPPageTweaks::OnCancel()
{
	AppSettings& s = AfxGetAppSettings();

//	s.nThemeBrightness	= m_nThemeBrightness_Old;
//	s.nThemeRed			= m_nThemeRed_Old;
//	s.nThemeGreen			= m_nThemeGreen_Old;
//	s.nThemeBlue			= m_nThemeBlue_Old;

//	OnThemeChange();
}

//void CPPageTweaks::OnThemeChange()
//{
//	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());

//	HWND WndToolBar = ((CMainFrame*)AfxGetMainWnd())->m_hWnd_toolbar;

//	if (::IsWindow(WndToolBar)) {
//		AfxGetAppSettings().fToolbarRefresh = true;

//		::PostMessage(WndToolBar, WM_SIZE, SIZE_RESTORED, MAKELPARAM(320, 240));
//	}

//	pFrame->Invalidate();
//}

BEGIN_MESSAGE_MAP(CPPageTweaks, CPPageBase)
//	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateCheck3)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFACE, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLROUTLINE, OnCustomDrawBtns)
	ON_BN_CLICKED(IDC_BUTTON_CLRFACE, OnClickClrFace)
	ON_BN_CLICKED(IDC_BUTTON_CLROUTLINE, OnClickClrOutline)
	ON_BN_CLICKED(IDC_BUTTON_CLRDEFAULT, OnClickClrDefault)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
//	ON_BN_CLICKED(IDC_CHECK8, OnUseTimeTooltipClicked)
//	ON_CBN_SELCHANGE(IDC_COMBO1, OnChngOSDCombo)
//	ON_CBN_SELCHANGE(IDC_COMBO2, OnChngOSDCombo)
//	ON_UPDATE_COMMAND_UI(IDC_SLIDER1, OnUpdateThemeBrightness)
//	ON_UPDATE_COMMAND_UI(IDC_SLIDER2, OnUpdateThemeRed)
//	ON_UPDATE_COMMAND_UI(IDC_SLIDER3, OnUpdateThemeGreen)
//	ON_UPDATE_COMMAND_UI(IDC_SLIDER4, OnUpdateThemeBlue)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageTweaks message handlers

//void CPPageTweaks::OnUpdateCheck3(CCmdUI* pCmdUI)
//{
//	GetDlgItem(IDC_BUTTON_CLRFACE)->EnableWindow(m_fDisableXPToolbars);
//	GetDlgItem(IDC_BUTTON_CLROUTLINE)->EnableWindow(m_fDisableXPToolbars);
//	GetDlgItem(IDC_BUTTON_CLRDEFAULT)->EnableWindow(m_fDisableXPToolbars);
//	GetDlgItem(IDC_STATIC_CLRFACE)->EnableWindow(m_fDisableXPToolbars);
//	GetDlgItem(IDC_STATIC_CLROUTLINE)->EnableWindow(m_fDisableXPToolbars);
//}

void CPPageTweaks::OnClickClrDefault()
{
	AppSettings& s = AfxGetAppSettings();

//	m_clrFaceABGR = 0x00ffffff;
//	m_clrOutlineABGR = 0x00868686;
	GetDlgItem(IDC_BUTTON_CLRFACE)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->Invalidate();

	PostMessage(WM_COMMAND, IDC_CHECK3);

//	s.nThemeBrightness	= m_nThemeBrightness	= 15;
//	s.nThemeRed			= m_nThemeRed			= 255;
//	s.nThemeGreen			= m_nThemeGreen			= 255;
//	s.nThemeBlue			= m_nThemeBlue			= 255;

//	OnThemeChange();

	UpdateData(FALSE);
}

void CPPageTweaks::OnClickClrFace()
{
	CColorDialog clrpicker;	
	clrpicker.m_cc.Flags |= CC_SOLIDCOLOR|CC_RGBINIT;
//	clrpicker.m_cc.rgbResult = m_clrFaceABGR;

//	if (clrpicker.DoModal() == IDOK) {
//		m_clrFaceABGR = clrpicker.GetColor();
//	}

	PostMessage(WM_COMMAND, IDC_CHECK3);

	UpdateData(FALSE);
}

void CPPageTweaks::OnClickClrOutline()
{
	CColorDialog clrpicker;	
	clrpicker.m_cc.Flags |= CC_SOLIDCOLOR|CC_RGBINIT;
//	clrpicker.m_cc.rgbResult = m_clrOutlineABGR;

//	if (clrpicker.DoModal() == IDOK) {
//		m_clrOutlineABGR = clrpicker.GetColor();
//	}

	PostMessage(WM_COMMAND, IDC_CHECK3);

	UpdateData(FALSE);
}

void CPPageTweaks::OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	//if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE || pNMCD->dwItemSpec == IDC_BUTTON_CLROUTLINE) {
		if (pNMCD->dwDrawStage == CDDS_PREPAINT) {
			CDC dc;
			dc.Attach(pNMCD->hdc);
			CRect r;
			CopyRect(&r,&pNMCD->rc);
			CPen penFrEnabled (PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
			CPen penFrDisabled (PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
			CPen *penOld = dc.SelectObject(&penFrEnabled);

			if (CDIS_HOT == pNMCD->uItemState || CDIS_HOT + CDIS_FOCUS == pNMCD->uItemState || CDIS_DISABLED == pNMCD->uItemState) { 
				dc.SelectObject(&penFrDisabled);
			}

			dc.RoundRect(r.left, r.top, r.right, r.bottom, 6, 4);		
			r.DeflateRect(2,2,2,2);
//			dc.FillSolidRect(&r, pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE ? m_clrFaceABGR : m_clrOutlineABGR);
			dc.SelectObject(&penOld);
			dc.Detach();

			*pResult = CDRF_SKIPDEFAULT;
		}
	//}
}

void CPPageTweaks::OnBnClickedButton1()
{
	m_nJumpDistS = DEFAULT_JUMPDISTANCE_1;
	m_nJumpDistM = DEFAULT_JUMPDISTANCE_2;
	m_nJumpDistL = DEFAULT_JUMPDISTANCE_3;

	UpdateData(FALSE);

	SetModified();
}

//void CPPageTweaks::OnChngOSDCombo()
//{
//	CString str;
//	m_OSD_Size = m_FontSize.GetCurSel()+10;
//	m_FontType.GetLBText(m_FontType.GetCurSel(),str);

//	((CMainFrame*)AfxGetMainWnd())->m_OSD.DisplayMessage(OSD_TOPLEFT, _T("OSD test"), 2000, m_OSD_Size, str);

//	SetModified();
//}

//void CPPageTweaks::OnUseTimeTooltipClicked()
//{
//	m_TimeTooltipPosition.EnableWindow(IsDlgButtonChecked(IDC_CHECK8));

//	SetModified();
//}

void CPPageTweaks::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	AppSettings& s = AfxGetAppSettings();

//	if (*pScrollBar == m_ThemeBrightnessCtrl) {
//		UpdateData();
//		s.nThemeBrightness	= m_nThemeBrightness;
//		OnThemeChange();
//	}

//	if (*pScrollBar == m_ThemeRedCtrl) {
//		UpdateData();
//		s.nThemeRed			= m_nThemeRed;
//		OnThemeChange();
//	}

//	if (*pScrollBar == m_ThemeGreenCtrl) {
//		UpdateData();
//		s.nThemeGreen			= m_nThemeGreen;
//		OnThemeChange();
//	}

//	if (*pScrollBar == m_ThemeBlueCtrl) {
//		UpdateData();
//		s.nThemeBlue			= m_nThemeBlue;
//		OnThemeChange();
//	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

//void CPPageTweaks::OnUpdateThemeBrightness(CCmdUI* pCmdUI)
//{
//	UpdateData();

//	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
//}

//void CPPageTweaks::OnUpdateThemeRed(CCmdUI* pCmdUI)
//{
//	UpdateData();

//	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
//}

//void CPPageTweaks::OnUpdateThemeGreen(CCmdUI* pCmdUI)
//{
//	UpdateData();

//	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
//}

//void CPPageTweaks::OnUpdateThemeBlue(CCmdUI* pCmdUI)
//{
//	UpdateData();

//	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
//}
