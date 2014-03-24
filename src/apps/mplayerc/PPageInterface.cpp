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
#include "MainFrm.h"
#include "PPageInterface.h"
#include "../../DSUtil/SysVersion.h"


// CPPageInterface dialog

IMPLEMENT_DYNAMIC(CPPageInterface, CPPageBase)
CPPageInterface::CPPageInterface()
	: CPPageBase(CPPageInterface::IDD, CPPageInterface::IDD)
	, m_fDisableXPToolbars(FALSE)
	, m_nThemeBrightness(0)
	, m_nThemeRed(255)
	, m_nThemeGreen(255)
	, m_nThemeBlue(255)
	, m_nOSDTransparent(0)
	, m_fFileNameOnSeekBar(TRUE)
	, m_clrFaceABGR(0x00ffffff)
	, m_clrOutlineABGR(0x00868686)
	, m_clrFontABGR(0x00E0E0E0)
	, m_clrGrad1ABGR(0x00302820)
	, m_clrGrad2ABGR(0x00302820)
	, m_OSD_Size(0)
	, m_fUseWin7TaskBar(TRUE)
	, m_fSmartSeek(FALSE)
	, m_fChapterMarker(FALSE)
	, m_fFlybar(TRUE)
	, m_fFontShadow(FALSE)
	, m_fFontAA(TRUE)
	, m_OSDBorder(1)
{
}

CPPageInterface::~CPPageInterface()
{
}

void CPPageInterface::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK3, m_fDisableXPToolbars);
	DDX_Control(pDX, IDC_CHECK3, m_fDisableXPToolbarsCtrl);
	DDX_Slider(pDX, IDC_SLIDER1, m_nThemeBrightness);
	DDX_Slider(pDX, IDC_SLIDER2, m_nThemeRed);
	DDX_Slider(pDX, IDC_SLIDER3, m_nThemeGreen);
	DDX_Slider(pDX, IDC_SLIDER4, m_nThemeBlue);
	DDX_Slider(pDX, IDC_SLIDER_OSDTRANS, m_nOSDTransparent);
	DDX_Control(pDX, IDC_SLIDER1, m_ThemeBrightnessCtrl);
	DDX_Control(pDX, IDC_SLIDER2, m_ThemeRedCtrl);
	DDX_Control(pDX, IDC_SLIDER3, m_ThemeGreenCtrl);
	DDX_Control(pDX, IDC_SLIDER4, m_ThemeBlueCtrl);
	DDX_Control(pDX, IDC_SLIDER_OSDTRANS, m_OSDTransparentCtrl);
	DDX_Check(pDX, IDC_CHECK5, m_fFileNameOnSeekBar);
	DDX_Check(pDX, IDC_CHECK_WIN7, m_fUseWin7TaskBar);
	DDX_Check(pDX, IDC_CHECK8, m_fUseTimeTooltip);
	DDX_Control(pDX, IDC_COMBO3, m_TimeTooltipPosition);
	DDX_Control(pDX, IDC_COMBO1, m_FontType);
	DDX_Control(pDX, IDC_COMBO2, m_FontSize);
	DDX_Check(pDX, IDC_CHECK_PRV, m_fSmartSeek);
	DDX_Check(pDX, IDC_CHECK_CHM, m_fChapterMarker);
	DDX_Check(pDX, IDC_CHECK_FLYBAR, m_fFlybar);
	DDX_Text(pDX, IDC_EDIT4, m_OSDBorder);
	DDX_Control(pDX, IDC_SPIN10, m_OSDBorderCtrl);
	DDX_Check(pDX, IDC_CHECK_SHADOW, m_fFontShadow);
	DDX_Check(pDX, IDC_CHECK_AA, m_fFontAA);
}

int CALLBACK EnumFontProc(ENUMLOGFONT FAR* lf, NEWTEXTMETRIC FAR* tm, int FontType, LPARAM dwData)
{
	CAtlArray<CString>* fntl = (CAtlArray<CString>*)dwData;

	if (FontType == TRUETYPE_FONTTYPE) {
		fntl->Add(lf->elfFullName);
	}

	return true;
}

BOOL CPPageInterface::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

	m_fDisableXPToolbars	= s.fDisableXPToolbars;
	m_nThemeBrightness		= m_nThemeBrightness_Old	= s.nThemeBrightness;
	m_nThemeRed				= m_nThemeRed_Old			= s.nThemeRed;
	m_nThemeGreen			= m_nThemeGreen_Old			= s.nThemeGreen;
	m_nThemeBlue			= m_nThemeBlue_Old			= s.nThemeBlue;
	m_nOSDTransparent		= m_nOSDTransparent_Old		= s.nOSDTransparent;
	m_OSDBorder				= m_OSDBorder_Old			= s.nOSDBorder;

	m_ThemeBrightnessCtrl.SetRange(0, 100);
	m_ThemeRedCtrl.SetRange(0, 255);
	m_ThemeGreenCtrl.SetRange(0, 255);
	m_ThemeBlueCtrl.SetRange(0, 255);
	m_OSDTransparentCtrl.SetRange(0, 255);
	m_OSDBorderCtrl.SetRange32(0, 5);

	m_fFileNameOnSeekBar	= s.fFileNameOnSeekBar;
	m_clrFaceABGR			= s.clrFaceABGR;
	m_clrOutlineABGR		= s.clrOutlineABGR;
	m_clrFontABGR			= s.clrFontABGR;
	m_clrGrad1ABGR			= s.clrGrad1ABGR;
	m_clrGrad2ABGR			= s.clrGrad2ABGR;
	m_fUseWin7TaskBar		= s.fUseWin7TaskBar;
	GetDlgItem(IDC_CHECK_WIN7)->EnableWindow(IsWinSevenOrLater());

	m_fUseTimeTooltip = s.fUseTimeTooltip;
	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_ABOVE));
	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_BELOW));
	m_TimeTooltipPosition.SetCurSel(s.nTimeTooltipPosition);
	m_TimeTooltipPosition.EnableWindow(m_fUseTimeTooltip);

	m_OSD_Size	= s.nOSDSize;
	m_OSD_Font	= s.strOSDFont;

	m_fSmartSeek		= s.fSmartSeek;
	m_fChapterMarker	= s.fChapterMarker;
	m_fFlybar			= s.fFlybar;
	m_fFontShadow		= m_fFontShadow_Old = s.fFontShadow;
	m_fFontAA			= m_fFontAA_Old = s.fFontAA;
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

	CorrectComboListWidth(m_FontType);
	int iSel = m_FontType.FindStringExact(0, m_OSD_Font);
	if (iSel == CB_ERR) iSel = 0;
	m_FontType.SetCurSel(iSel);

	CString str;

	for (int i = 10; i < 26; ++i) {
		str.Format(_T("%d"), i);
		m_FontSize.AddString(str);

		if (m_OSD_Size == i) {
			iSel = i;
		}
	}

	m_FontSize.SetCurSel(iSel - 10);

	EnableToolTips(TRUE);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageInterface::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.clrFaceABGR			= m_clrFaceABGR;
	s.clrOutlineABGR		= m_clrOutlineABGR;
	s.clrFontABGR			= m_clrFontABGR;
	s.clrGrad1ABGR			= m_clrGrad1ABGR;
	s.clrGrad2ABGR			= m_clrGrad2ABGR;
	s.nThemeBrightness		= m_nThemeBrightness;
	s.nThemeRed				= m_nThemeRed;
	s.nThemeGreen			= m_nThemeGreen;
	s.nThemeBlue			= m_nThemeBlue;
	s.nOSDTransparent		= m_nOSDTransparent;
	s.fFileNameOnSeekBar	= !!m_fFileNameOnSeekBar;
	s.nOSDBorder			= m_OSDBorder;

	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
	if (pMainFrame->m_OSD && s.nOSDTransparent != m_nOSDTransparent_Old) {
		pMainFrame->m_OSD.SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - s.nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
	}

	HWND WndToolBar			= ((CMainFrame*)AfxGetMainWnd())->m_hWnd_toolbar;
	BOOL fDisableXPToolbars	= s.fDisableXPToolbars;
	s.fDisableXPToolbars	= !!m_fDisableXPToolbars;
	if (::IsWindow(WndToolBar) && (s.fDisableXPToolbars != !!fDisableXPToolbars)) {
		::PostMessage(WndToolBar,								WM_SIZE, s.nLastWindowType, MAKELPARAM(s.rcLastWindowPos.Width(), s.rcLastWindowPos.Height()));
		::PostMessage(((CMainFrame*)AfxGetMainWnd())->m_hWnd,	WM_SIZE, s.nLastWindowType, MAKELPARAM(s.rcLastWindowPos.Width(), s.rcLastWindowPos.Height()));
	}

	s.fUseWin7TaskBar		= !!m_fUseWin7TaskBar;
	s.fUseTimeTooltip		= !!m_fUseTimeTooltip;
	s.nTimeTooltipPosition	= m_TimeTooltipPosition.GetCurSel();
	s.nOSDSize				= m_OSD_Size;

	m_FontType.GetLBText(m_FontType.GetCurSel(),s.strOSDFont);

	s.fSmartSeek			= !!m_fSmartSeek;
	s.fChapterMarker		= !!m_fChapterMarker;
	s.fFlybar				= !!m_fFlybar;
	s.fFontShadow			= !!m_fFontShadow;
	s.fFontAA				= !!m_fFontAA;

	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());

	if (s.fFlybar && !pFrame->m_wndFlyBar) {
		pFrame->CreateFlyBar();

	} else if (!s.fFlybar && pFrame->m_wndFlyBar){
		pFrame->DestroyFlyBar();
	}

	pFrame->CreateThumbnailToolbar();
	pFrame->UpdateThumbarButton();
	pFrame->SetDwmPreview();

	pFrame->Invalidate();

	return __super::OnApply();
}

void CPPageInterface::OnCancel()
{
	AppSettings& s = AfxGetAppSettings();

	s.nThemeBrightness	= m_nThemeBrightness_Old;
	s.nThemeRed			= m_nThemeRed_Old;
	s.nThemeGreen		= m_nThemeGreen_Old;
	s.nThemeBlue		= m_nThemeBlue_Old;
	s.nOSDBorder		= m_OSDBorder_Old;
	s.fFontShadow		= !!m_fFontShadow_Old;
	s.fFontAA			= !!m_fFontAA_Old;

	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
	if (pMainFrame->m_OSD && m_nOSDTransparent != m_nOSDTransparent_Old) {
		pMainFrame->m_OSD.SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - m_nOSDTransparent_Old, LWA_ALPHA | LWA_COLORKEY);
	}
	s.nOSDTransparent	= m_nOSDTransparent_Old;

	pMainFrame->m_OSD.ClearMessage();

	OnThemeChange();
}

void CPPageInterface::OnThemeChange()
{
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());

	HWND WndToolBar = ((CMainFrame*)AfxGetMainWnd())->m_hWnd_toolbar;

	if (::IsWindow(WndToolBar)) {
		::PostMessage(WndToolBar, WM_SIZE, SIZE_RESTORED, MAKELPARAM(320, 240));
	}

	pFrame->Invalidate();

	((CMainFrame*)AfxGetMainWnd())->m_wndPlaylistBar.Invalidate();
}

BEGIN_MESSAGE_MAP(CPPageInterface, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateCheck3)
	ON_COMMAND(IDC_CHECK_SHADOW, OnCheckShadow)
	ON_COMMAND(IDC_CHECK_AA, OnCheckAA)
	ON_UPDATE_COMMAND_UI(IDC_EDIT4, OnUpdateOSDBorder)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFACE, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLROUTLINE, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFONT, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRGRAD1, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRGRAD2, OnCustomDrawBtns)
	ON_BN_CLICKED(IDC_BUTTON_CLRFACE, OnClickClrFace)
	ON_BN_CLICKED(IDC_BUTTON_CLRFONT, OnClickClrFont)
	ON_BN_CLICKED(IDC_BUTTON_CLRGRAD1, OnClickClrGrad1)
	ON_BN_CLICKED(IDC_BUTTON_CLRGRAD2, OnClickClrGrad2)
	ON_BN_CLICKED(IDC_BUTTON_CLROUTLINE, OnClickClrOutline)
	ON_BN_CLICKED(IDC_BUTTON_CLRDEFAULT, OnClickClrDefault)
	ON_BN_CLICKED(IDC_CHECK8, OnUseTimeTooltipClicked)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnChngOSDCombo)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnChngOSDCombo)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER1, OnUpdateThemeBrightness)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER2, OnUpdateThemeRed)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER3, OnUpdateThemeGreen)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER4, OnUpdateThemeBlue)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_OSDTRANS, OnUpdateOSDTransparent)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageInterface message handlers

void CPPageInterface::OnUpdateCheck3(CCmdUI* pCmdUI)
{
	GetDlgItem(IDC_BUTTON_CLRFACE)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_BUTTON_CLRDEFAULT)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_STATIC_CLRFACE)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_STATIC_CLROUTLINE)->EnableWindow(m_fDisableXPToolbars);
}

void CPPageInterface::OnCheckShadow()
{
	AppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontShadow = s.fFontShadow;
	s.fFontShadow = !!m_fFontShadow;
	OnChngOSDCombo();

	s.fFontShadow = !!fFontShadow;
}

void CPPageInterface::OnCheckAA()
{
	AppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontAA = s.fFontAA;
	s.fFontAA = !!m_fFontAA;
	OnChngOSDCombo();

	s.fFontAA = !!fFontAA;
}

void CPPageInterface::OnUpdateOSDBorder(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();

	if (s.nOSDBorder != m_OSDBorder) {
		UpdateData();
		int nOSDBorder = s.nOSDBorder;
		s.nOSDBorder = m_OSDBorder;
		OnChngOSDCombo();

		s.nOSDBorder = nOSDBorder;
	}
}

void CPPageInterface::OnClickClrDefault()
{
	AppSettings& s = AfxGetAppSettings();

	m_clrFaceABGR = 0x00ffffff;
	m_clrOutlineABGR = 0x00868686;
	m_clrFontABGR = 0x00E0E0E0;
	m_clrGrad1ABGR = 0x00302820;
	m_clrGrad2ABGR = 0x00302820;
	GetDlgItem(IDC_BUTTON_CLRFACE)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRFONT)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRGRAD1)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRGRAD2)->Invalidate();
	PostMessage(WM_COMMAND, IDC_CHECK3);

	s.nThemeBrightness		= m_nThemeBrightness	= 15;
	s.nThemeRed				= m_nThemeRed			= 255;
	s.nThemeGreen			= m_nThemeGreen			= 255;
	s.nThemeBlue			= m_nThemeBlue			= 255;
	s.nOSDTransparent		= m_nOSDTransparent		= 100;

	m_fFontShadow = FALSE;
	m_fFontAA = TRUE;
	m_OSDBorder = 1;
	OnThemeChange();

	UpdateData(FALSE);
}

void CPPageInterface::OnClickClrFace()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrFaceABGR;

	if (clrpicker.DoModal() == IDOK) {
		m_clrFaceABGR = clrpicker.GetColor();
	}

	PostMessage(WM_COMMAND, IDC_CHECK3);

	UpdateData(FALSE);
}

void CPPageInterface::OnClickClrOutline()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrOutlineABGR;

	if (clrpicker.DoModal() == IDOK) {
		m_clrOutlineABGR = clrpicker.GetColor();
	}

	PostMessage(WM_COMMAND, IDC_CHECK3);

	UpdateData(FALSE);
}

void CPPageInterface::OnClickClrFont()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrFontABGR;

	if (clrpicker.DoModal() == IDOK) {
		m_clrFontABGR = clrpicker.GetColor();
	}

	AppSettings& s = AfxGetAppSettings();

	UpdateData();
	int clrFontABGR = s.clrFontABGR;
	s.clrFontABGR = m_clrFontABGR;
	OnChngOSDCombo();

	s.clrFontABGR = clrFontABGR;
}

void CPPageInterface::OnClickClrGrad1()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrGrad1ABGR;

	if (clrpicker.DoModal() == IDOK) {
		m_clrGrad1ABGR = clrpicker.GetColor();
	}

	AppSettings& s = AfxGetAppSettings();

	UpdateData();
	int clrGrad1ABGR = s.clrGrad1ABGR;
	s.clrGrad1ABGR = m_clrGrad1ABGR;
	OnChngOSDCombo();

	s.clrGrad1ABGR = clrGrad1ABGR;
}

void CPPageInterface::OnClickClrGrad2()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrGrad2ABGR;

	if (clrpicker.DoModal() == IDOK) {
		m_clrGrad2ABGR = clrpicker.GetColor();
	}

	AppSettings& s = AfxGetAppSettings();

	UpdateData();
	int clrGrad2ABGR = s.clrGrad2ABGR;
	s.clrGrad2ABGR = m_clrGrad2ABGR;
	OnChngOSDCombo();

	s.clrGrad2ABGR = clrGrad2ABGR;
}

void CPPageInterface::OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLROUTLINE
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLRFONT
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD1
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD2) {
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
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE) {
				dc.FillSolidRect(&r, m_clrFaceABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLROUTLINE) {
				dc.FillSolidRect(&r, m_clrOutlineABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFONT) {
				dc.FillSolidRect(&r, m_clrFontABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD1) {
				dc.FillSolidRect(&r, m_clrGrad1ABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD2) {
				dc.FillSolidRect(&r, m_clrGrad2ABGR);
			}

			dc.SelectObject(&penOld);
			dc.Detach();

			*pResult = CDRF_SKIPDEFAULT;
		}
	}
}

void CPPageInterface::OnChngOSDCombo()
{
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();

	if (pMainFrame->m_OSD) {
		CString str;
		m_OSD_Size = m_FontSize.GetCurSel() + 10;
		m_FontType.GetLBText(m_FontType.GetCurSel(), str);

		pMainFrame->m_OSD.DisplayMessage(OSD_TOPLEFT, _T("OSD test"), 2000, m_OSD_Size, str);
		pMainFrame->m_OSD.SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - AfxGetAppSettings().nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
	}

	SetModified();
}

void CPPageInterface::OnUseTimeTooltipClicked()
{
	m_TimeTooltipPosition.EnableWindow(IsDlgButtonChecked(IDC_CHECK8));

	SetModified();
}

void CPPageInterface::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	AppSettings& s = AfxGetAppSettings();

	if (*pScrollBar == m_ThemeBrightnessCtrl) {
		UpdateData();
		s.nThemeBrightness	= m_nThemeBrightness;
		OnThemeChange();
	}

	if (*pScrollBar == m_ThemeRedCtrl) {
		UpdateData();
		s.nThemeRed			= m_nThemeRed;
		OnThemeChange();
	}

	if (*pScrollBar == m_ThemeGreenCtrl) {
		UpdateData();
		s.nThemeGreen		= m_nThemeGreen;
		OnThemeChange();
	}

	if (*pScrollBar == m_ThemeBlueCtrl) {
		UpdateData();
		s.nThemeBlue		= m_nThemeBlue;
		OnThemeChange();
	}

	if (*pScrollBar == m_OSDTransparentCtrl) {
		UpdateData();
		int nOSDTransparent	= s.nOSDTransparent;
		s.nOSDTransparent	= m_nOSDTransparent;
		OnChngOSDCombo();

		s.nOSDTransparent	= nOSDTransparent;
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageInterface::OnUpdateThemeBrightness(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateThemeRed(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateThemeGreen(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateThemeBlue(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateOSDTransparent(CCmdUI* pCmdUI)
{
	UpdateData();
}
