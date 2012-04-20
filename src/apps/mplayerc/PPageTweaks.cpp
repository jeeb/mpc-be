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
	, m_fDisableXPToolbars(FALSE)
	, m_clrFaceABGR(0x00e9e9e9)//ins:2452 bobdynlan:default clrFace//
	, m_clrOutlineABGR(0x00a0a0a0)//ins:2452 bobdynlan:default clrOutline//
	, m_fUseWMASFReader(FALSE)
	, m_nJumpDistS(0)
	, m_nJumpDistM(0)
	, m_nJumpDistL(0)
	, m_OSD_Size(0)
	, m_fNotifyMSN(TRUE)
	, m_fPreventMinimize(FALSE)
	, m_fUseWin7TaskBar(TRUE)
	, m_fDontUseSearchInFolder(FALSE)
{
	m_fWMASFReader = SUCCEEDED(CComPtr<IBaseFilter>().CoCreateInstance(
								   GUIDFromCString(_T("{187463A0-5BB7-11D3-ACBE-0080C75E246E}")))); // WM ASF Reader
}

CPPageTweaks::~CPPageTweaks()
{
}

void CPPageTweaks::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK3, m_fDisableXPToolbars);
	DDX_Control(pDX, IDC_CHECK3, m_fDisableXPToolbarsCtrl);
	DDX_Check(pDX, IDC_CHECK2, m_fUseWMASFReader);
	DDX_Control(pDX, IDC_CHECK2, m_fUseWMASFReaderCtrl);
	DDX_Text(pDX, IDC_EDIT1, m_nJumpDistS);
	DDX_Text(pDX, IDC_EDIT2, m_nJumpDistM);
	DDX_Text(pDX, IDC_EDIT3, m_nJumpDistL);
	DDX_Check(pDX, IDC_CHECK4, m_fNotifyMSN);
	DDX_Check(pDX, IDC_CHECK6, m_fPreventMinimize);
	DDX_Check(pDX, IDC_CHECK_WIN7, m_fUseWin7TaskBar);
	DDX_Check(pDX, IDC_CHECK7, m_fDontUseSearchInFolder);
	DDX_Check(pDX, IDC_CHECK8, m_fUseTimeTooltip);
	DDX_Control(pDX, IDC_COMBO3, m_TimeTooltipPosition);
	DDX_Control(pDX, IDC_COMBO1, m_FontType);
	DDX_Control(pDX, IDC_COMBO2, m_FontSize);
	DDX_Check(pDX, IDC_CHECK1, m_fFastSeek);
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

	m_fDisableXPToolbars = s.fDisableXPToolbars;
	m_clrFaceABGR = s.clrFaceABGR;//ins:2452 bobdynlan:initiate clrFace from reg//
	m_clrOutlineABGR = s.clrOutlineABGR;//ins:2452 bobdynlan:initiate clrOutline from reg//
	m_fUseWMASFReader = s.fUseWMASFReader;
	m_nJumpDistS = s.nJumpDistS;
	m_nJumpDistM = s.nJumpDistM;
	m_nJumpDistL = s.nJumpDistL;
	m_fNotifyMSN = s.fNotifyMSN;

	m_fPreventMinimize = s.fPreventMinimize;
	m_fUseWin7TaskBar = s.fUseWin7TaskBar;
	m_fDontUseSearchInFolder =s.fDontUseSearchInFolder;

	m_fUseTimeTooltip = s.fUseTimeTooltip;
	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_ABOVE));
	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_BELOW));
	m_TimeTooltipPosition.SetCurSel(s.nTimeTooltipPosition);
	m_TimeTooltipPosition.EnableWindow(m_fUseTimeTooltip);

	m_OSD_Size = s.nOSDSize;
	m_OSD_Font = s.strOSDFont;

	m_fFastSeek = s.fFastSeek;

	CString str;
	int iSel = 0;
	m_FontType.Clear();
	m_FontSize.Clear();
	HDC dc = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	CAtlArray<CString> fntl;
	EnumFontFamilies(dc, NULL,(FONTENUMPROC)EnumFontProc, (LPARAM)&fntl);
	DeleteDC(dc);
	for (size_t i=0; i< fntl.GetCount(); i++) {
		if (i>0 && fntl[i-1] == fntl[i]) {
			continue;
		}
		m_FontType.AddString(fntl[i]);
	}
	for (int i=0; i< m_FontType.GetCount(); i++) {
		m_FontType.GetLBText(i,str);
		if (m_OSD_Font == str) {
			iSel=i;
		}
	}
	m_FontType.SetCurSel(iSel);

	for (int i=10; i<26; i++) {
		str.Format(_T("%d"), i);
		m_FontSize.AddString(str);
		if (m_OSD_Size == i) {
			iSel=i;
		}
	}
	m_FontSize.SetCurSel(iSel-10);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageTweaks::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();
//INS:2452 bobdynlan: redraw toolbar if !fDisableXPToolbars
//refresh PlayerToolBar buttons without restart disabled until gdi leak proof
//-----------------------------------------------------------------------------
	bool m_fToolbarRefresh=false;
	if (s.fDisableXPToolbars == true && m_fDisableXPToolbars == FALSE) m_fToolbarRefresh=true;
	if (s.fDisableXPToolbars == false && m_fDisableXPToolbars == TRUE) m_fToolbarRefresh=true;
	if (s.clrFaceABGR != m_clrFaceABGR || s.clrOutlineABGR != m_clrOutlineABGR) m_fToolbarRefresh=true;

	if (m_fToolbarRefresh)
	{
		s.clrFaceABGR = m_clrFaceABGR;//ins:2452 bobdynlan:user-selected color for button face//
		s.clrOutlineABGR = m_clrOutlineABGR;//ins:2452 bobdynlan:user-selected color for button outline//
		//define m_hWnd_toolbar in mainfrm.h, initiate in mainfrm.cpp OnCreate
		HWND WndToolBar= ((CMainFrame*)AfxGetMainWnd())->m_hWnd_toolbar;
	    if (::IsWindow(WndToolBar))
		{
			s.fToolbarRefresh = true;//set refresh flag
			//triggers PlayerToolBar::ArrangeControls() at OnSize
			::PostMessage(WndToolBar, WM_SIZE, SIZE_RESTORED, MAKELPARAM(320, 240));//w,h ignored(good) by SIZE_RESTORED?!
		}
	}
//--------------------------------------------------------------------------INS
	s.fDisableXPToolbars = !!m_fDisableXPToolbars;
	s.fUseWMASFReader = !!m_fUseWMASFReader;
	s.nJumpDistS = m_nJumpDistS;
	s.nJumpDistM = m_nJumpDistM;
	s.nJumpDistL = m_nJumpDistL;
	s.fNotifyMSN = !!m_fNotifyMSN;

	s.fPreventMinimize = !!m_fPreventMinimize;
	s.fUseWin7TaskBar = !!m_fUseWin7TaskBar;
	s.fDontUseSearchInFolder = !!m_fDontUseSearchInFolder;
	s.fUseTimeTooltip = !!m_fUseTimeTooltip;
	s.nTimeTooltipPosition = m_TimeTooltipPosition.GetCurSel();
	s.nOSDSize = m_OSD_Size;
	m_FontType.GetLBText(m_FontType.GetCurSel(),s.strOSDFont);

	s.fFastSeek = !!m_fFastSeek;

	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	if (m_fUseWin7TaskBar) {
		pFrame->CreateThumbnailToolbar();
	}
	pFrame->UpdateThumbarButton();

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageTweaks, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateCheck3)
//INS:2452 bobdynlan: setup related controls
//-----------------------------------------------------------------------------
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFACE, OnCustomDrawBtns)//ins:2452 bobdynlan: show colors instead of an ungly number//
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLROUTLINE, OnCustomDrawBtns)//ins:2452 bobdynlan: show colors instead of an ungly number//
	ON_BN_CLICKED(IDC_BUTTON_CLRFACE, OnClickClrFace)//ins:2452 bobdynlan: clrFace picker//
	ON_BN_CLICKED(IDC_BUTTON_CLROUTLINE, OnClickClrOutline)//ins:2452 bobdynlan: clrOutline picker//
	ON_BN_CLICKED(IDC_BUTTON_CLRDEFAULT, OnClickClrDefault)//ins:2452 bobdynlan: reset colors//
//--------------------------------------------------------------------------INS
	ON_UPDATE_COMMAND_UI(IDC_CHECK2, OnUpdateCheck2)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_CHECK8, OnUseTimeTooltipClicked)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnChngOSDCombo)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnChngOSDCombo)
END_MESSAGE_MAP()


// CPPageTweaks message handlers

void CPPageTweaks::OnUpdateCheck3(CCmdUI* pCmdUI)
{
//INS:2452 bobdynlan: enable/disable the added controls
//-----------------------------------------------------------------------------
	GetDlgItem(IDC_BUTTON_CLRFACE)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_BUTTON_CLRDEFAULT)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_STATIC_CLRFACE)->EnableWindow(m_fDisableXPToolbars);
	GetDlgItem(IDC_STATIC_CLROUTLINE)->EnableWindow(m_fDisableXPToolbars);
//--------------------------------------------------------------------------INS
}
//INS:2452 bobdynlan: add events
//-----------------------------------------------------------------------------
void CPPageTweaks::OnClickClrDefault()
{
	m_clrFaceABGR = 0x00e9e9e9;
	m_clrOutlineABGR = 0x00a0a0a0;
	GetDlgItem(IDC_BUTTON_CLRFACE)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->Invalidate();
	//CheckDlgButton(IDC_CHECK3, BST_UNCHECKED);
	//CheckDlgButton(IDC_CHECK3, BST_CHECKED);
	PostMessage(WM_COMMAND, IDC_CHECK3);//hack to enable the apply button
	UpdateData(FALSE);
}
void CPPageTweaks::OnClickClrFace()
{
	CColorDialog clrpicker;	
	clrpicker.m_cc.Flags |= /*CC_FULLOPEN|*/CC_SOLIDCOLOR|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrFaceABGR;
	if (clrpicker.DoModal() == IDOK) m_clrFaceABGR = clrpicker.GetColor();
	PostMessage(WM_COMMAND, IDC_CHECK3);//hack to enable the apply button
	UpdateData(FALSE);
}
void CPPageTweaks::OnClickClrOutline()
{
	CColorDialog clrpicker;	
	clrpicker.m_cc.Flags |= /*CC_FULLOPEN|*/CC_SOLIDCOLOR|CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrOutlineABGR;
	if (clrpicker.DoModal() == IDOK) m_clrOutlineABGR = clrpicker.GetColor();
	PostMessage(WM_COMMAND, IDC_CHECK3);//hack to enable the apply button
	UpdateData(FALSE);
}
void CPPageTweaks::OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult)
{
	//TRACE (" (CUSTOMDRAW) ");
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;
	//if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE || pNMCD->dwItemSpec == IDC_BUTTON_CLROUTLINE)
	//{
		if (pNMCD->dwDrawStage == CDDS_PREPAINT)
		{
			CDC dc;
			dc.Attach(pNMCD->hdc);
			CRect r;
			CopyRect(&r,&pNMCD->rc);
			CPen penFrEnabled (PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
			CPen penFrDisabled (PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
			CPen *penOld = dc.SelectObject(&penFrEnabled);
			if (CDIS_HOT == pNMCD->uItemState || CDIS_HOT + CDIS_FOCUS == pNMCD->uItemState || CDIS_DISABLED == pNMCD->uItemState) 
				dc.SelectObject(&penFrDisabled);
			dc.RoundRect(r.left, r.top, r.right, r.bottom, 6, 4);		
			r.DeflateRect(2,2,2,2);
			dc.FillSolidRect(&r, pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE ? m_clrFaceABGR : m_clrOutlineABGR);
			dc.SelectObject(&penOld);
			dc.Detach();
			//TRACE("\n\n uItemState=%2x (%2d,%2d,W=%2d,H=%2d)\n\n",pNMCD->uItemState, r.left, r.top, r.Width(), r.Height()); 
			*pResult = CDRF_SKIPDEFAULT;
		}
	//}
}
//--------------------------------------------------------------------------INS

void CPPageTweaks::OnUpdateCheck2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_fWMASFReader);
}

void CPPageTweaks::OnBnClickedButton1()
{
	m_nJumpDistS = DEFAULT_JUMPDISTANCE_1;
	m_nJumpDistM = DEFAULT_JUMPDISTANCE_2;
	m_nJumpDistL = DEFAULT_JUMPDISTANCE_3;

	UpdateData(FALSE);
	SetModified();
}

void CPPageTweaks::OnChngOSDCombo()
{
	CString str;
	m_OSD_Size = m_FontSize.GetCurSel()+10;
	m_FontType.GetLBText(m_FontType.GetCurSel(),str);
	((CMainFrame*)AfxGetMainWnd())->m_OSD.DisplayMessage(OSD_TOPLEFT, _T("Test"), 2000, m_OSD_Size, str);
	SetModified();
}

void CPPageTweaks::OnUseTimeTooltipClicked()
{
	m_TimeTooltipPosition.EnableWindow(IsDlgButtonChecked(IDC_CHECK8));

	SetModified();
}