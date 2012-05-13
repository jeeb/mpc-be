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
#include <math.h>
#include <atlbase.h>
#include <afxpriv.h>
#include "PlayerToolBar.h"
#include "MainFrm.h"


typedef HRESULT (__stdcall * SetWindowThemeFunct)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);


// CPlayerToolBar

IMPLEMENT_DYNAMIC(CPlayerToolBar, CToolBar)
CPlayerToolBar::CPlayerToolBar() : fDisableImgListRemap(false)
{
}

CPlayerToolBar::~CPlayerToolBar()
{
	if (NULL != m_pButtonsImages) {
		delete m_pButtonsImages;
		m_pButtonsImages = NULL;
	}
}

BOOL CPlayerToolBar::Create(CWnd* pParentWnd)
{
	if (!__super::CreateEx(pParentWnd,
						  TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_AUTOSIZE|TBSTYLE_CUSTOMERASE,
						  WS_CHILD|WS_VISIBLE|CBRS_ALIGN_BOTTOM|CBRS_TOOLTIPS)) {
		return FALSE;
	}

	if (!LoadToolBar(IDB_PLAYERTOOLBAR)) {
		return FALSE;
	}

	// Should never be RTLed
	ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

	GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	CToolBarCtrl& tb = GetToolBarCtrl();
	tb.DeleteButton(1);
	tb.DeleteButton(tb.GetButtonCount()-1);
	tb.DeleteButton(tb.GetButtonCount()-1);

	AppSettings& s = AfxGetAppSettings();

	SetMute(s.fMute);

	UINT styles[] = {
		TBBS_CHECKGROUP/*TBBS_CHECKGROUP, TBBS_CHECKGROUP*/, TBBS_CHECKGROUP,
		TBBS_SEPARATOR,
		TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON,
		TBBS_SEPARATOR,
		TBBS_BUTTON,
		TBBS_SEPARATOR,
		TBBS_SEPARATOR,
		TBBS_CHECKBOX,
	};

	for (int i = 0; i < countof(styles); i++) {
		SetButtonStyle(i, styles[i]|TBBS_DISABLED);
	}

	m_volctrl.Create(this);
	m_volctrl.SetRange(0, 100);

	if (s.fDisableXPToolbars) {
		if (HMODULE h = LoadLibrary(_T("uxtheme.dll"))) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
			if (f) {
				f(m_hWnd, L" ", L" ");
			}
			FreeLibrary(h);
		}
	}

	m_nButtonHeight = 16; //reset m_nButtonHeight

	int fp = m_logobm.FileExists("toolbar");

	HBITMAP hBmp;
	if (s.fDisableXPToolbars && NULL == fp) {
		hBmp = m_logobm.LoadExternalImage("toolbar", s.nThemeBrightness, s.nThemeRed, s.nThemeGreen, s.nThemeBlue);
	} else {
		hBmp = m_logobm.LoadExternalImage("toolbar", -1, -1, -1, -1);
	}

	if (NULL != hBmp) {
		CBitmap *bmp = new CBitmap();
		bmp->Attach(hBmp);
		BITMAP bitmapBmp;
		bmp->GetBitmap(&bitmapBmp);
		if (bitmapBmp.bmWidth == bitmapBmp.bmHeight * 15) {
			// the manual specifies that sizeButton should be sizeImage inflated by (7, 6)
			SetSizes(CSize(bitmapBmp.bmHeight + 7, bitmapBmp.bmHeight + 6), CSize(bitmapBmp.bmHeight, bitmapBmp.bmHeight));

			CDC dc;
			dc.CreateCompatibleDC(NULL);

			DIBSECTION dib;
			::GetObject(hBmp, sizeof(dib), &dib);
			int fileDepth = dib.dsBmih.biBitCount;
			m_pButtonsImages = new CImageList();
			if (32 == fileDepth) {
				m_pButtonsImages->Create(bitmapBmp.bmHeight, bitmapBmp.bmHeight, ILC_COLOR32 | ILC_MASK, 1, 0);
				m_pButtonsImages->Add(bmp, static_cast<CBitmap*>(0));	// alpha is the mask
			} else {
				m_pButtonsImages->Create(bitmapBmp.bmHeight, bitmapBmp.bmHeight, ILC_COLOR24 | ILC_MASK, 1, 0);
				m_pButtonsImages->Add(bmp, RGB(255, 0, 255));
			}
			m_nButtonHeight = bitmapBmp.bmHeight;
			GetToolBarCtrl().SetImageList(m_pButtonsImages);
			fDisableImgListRemap = true;
		}
		delete bmp;
		DeleteObject(hBmp);
	}

	if (s.fDisableXPToolbars) {
		if (!fDisableImgListRemap) {
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 0);//nRemapState = 0 Remap Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 1);//nRemapState = 1 Remap Disabled
		}

		COLORSCHEME cs;
		cs.dwSize		= sizeof(COLORSCHEME);
		cs.clrBtnHighlight	= 0x0046413c; //clr_csLight = RGB( 60, 65, 70)
		cs.clrBtnShadow		= 0x0037322d;//clr_csShadow = RGB( 45, 50, 55)
		GetToolBarCtrl().SetColorScheme(&cs);
		GetToolBarCtrl().SetIndent(5);
	}

	return TRUE;
}

BOOL CPlayerToolBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!__super::PreCreateWindow(cs)) {
		return FALSE;
	}

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;

	return TRUE;
}

void CPlayerToolBar::CreateRemappedImgList(UINT bmID, int nRemapState, CImageList& reImgList)
{
	//nRemapState = 0 Remap Active
	//nRemapState = 1 Remap Disabled
	//nRemapState = 2 Undo  Active
	//nRemapState = 3 Undo  Disabled
	AppSettings& s = AfxGetAppSettings();
	COLORMAP cmActive[] =
	{ 
		0x00000000, s.clrFaceABGR,//0x00ffffff, //button_face in toolbar.bmp rc RGB(  0,  0,  0) to clr_resLight = RGB(255,255,255)
		0x00808080, s.clrOutlineABGR,//0x00c0c0c0,//button_outline in toolbar.bmp rc RGB(128,128,128) to clr_resShadow = RGB(192,192,192)
		0x00c0c0c0, 0x00ff00ff//background = transparency mask// toolbar.bmp rc RGB(192,192,192) to clr_resMask = RGB(255,  0,255)
	};
	COLORMAP cmDisabled[] =
	{ 
		0x00000000, 0x00ff00ff,//button_face -> transparency mask
		0x00808080, s.clrOutlineABGR,//0x00c0c0c0,//button_outline same as Active
		0x00c0c0c0, 0x00ff00ff//background = transparency mask
	};
	COLORMAP cmUndoActive[] =
	{ 
		0x00c0c0c0, 0x00ff00ff//background = transparency mask// toolbar.bmp rc RGB(192,192,192) to clr_resMask = RGB(255,  0,255)
	};
	COLORMAP cmUndoDisabled[] =
	{ 
		0x00000000, 0x00A0A0A0,//button_face -> black to gray?!	
		0x00c0c0c0, 0x00ff00ff//background = transparency mask
	};
	CBitmap bm;
	switch (nRemapState)
	{
		default:
		case 0:
			bm.LoadMappedBitmap(bmID,CMB_MASKED,cmActive,3);
			break;
		case 1:
			bm.LoadMappedBitmap(bmID,CMB_MASKED,cmDisabled,3);
			break;				
		case 2:
			bm.LoadMappedBitmap(bmID,CMB_MASKED,cmUndoActive,1);
			break;				
		case 3:
			bm.LoadMappedBitmap(bmID,CMB_MASKED,cmUndoDisabled,2);
			break;
	}
	BITMAP bmInfo;
	VERIFY(bm.GetBitmap(&bmInfo));
	VERIFY(reImgList.Create(bmInfo.bmHeight, bmInfo.bmHeight, bmInfo.bmBitsPixel | ILC_MASK, 1, 0));//ILC_COLOR8
	VERIFY(reImgList.Add(&bm, 0x00ff00ff) != -1);
}

void CPlayerToolBar::SwitchRemmapedImgList(UINT bmID, int nRemapState)
{
	//nRemapState = 0 Remap Active
	//nRemapState = 1 Remap Disabled
	//nRemapState = 2 Undo  Active
	//nRemapState = 3 Undo  Disabled
	CToolBarCtrl& ctrl = GetToolBarCtrl();
	if (nRemapState == 0 || nRemapState == 2) {
		if (m_reImgListActive.GetSafeHandle()) m_reImgListActive.DeleteImageList();//cleanup
		CreateRemappedImgList(bmID, nRemapState, m_reImgListActive);//remap
		ASSERT(m_reImgListActive.GetSafeHandle());
		ctrl.SetImageList(&m_reImgListActive);//switch to
	} else {
		if (m_reImgListDisabled.GetSafeHandle()) m_reImgListDisabled.DeleteImageList();//cleanup
		CreateRemappedImgList(bmID, nRemapState, m_reImgListDisabled);//remap
		ASSERT(m_reImgListDisabled.GetSafeHandle());
		ctrl.SetDisabledImageList(&m_reImgListDisabled);//switch to
	}
}

void CPlayerToolBar::ArrangeControls()
{
	if (!::IsWindow(m_volctrl.m_hWnd)) {
		return;
	}

	AppSettings& s = AfxGetAppSettings();
	if (s.fDisableXPToolbars && s.fToolbarRefresh) {//if switching from default to handsome :)
		if (!fDisableImgListRemap) {		
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 0);//nRemapState = 0 Remap Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 1);//nRemapState = 1 Remap Disabled
		}

		COLORSCHEME cs;
		cs.dwSize		= sizeof(COLORSCHEME);
		cs.clrBtnHighlight	= 0x0046413c; //clr_csLight = RGB( 60, 65, 70)
		cs.clrBtnShadow		= 0x0037322d;//clr_csShadow = RGB( 45, 50, 55)

		GetToolBarCtrl().SetColorScheme(&cs);
		GetToolBarCtrl().SetIndent(5);

		if (::IsWindow(m_volctrl.GetSafeHwnd())) {
			m_volctrl.EnableWindow(FALSE);
			m_volctrl.EnableWindow(TRUE);
		}
		//reset flag from PPageTweaks
		s.fToolbarRefresh=false;
		//kill default theme?!
		if (HMODULE h = LoadLibrary(_T("uxtheme.dll"))) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
			if (f) {
				f(m_hWnd, L" ", L" ");
			}
			FreeLibrary(h);
		}
	} else if (!s.fDisableXPToolbars && s.fToolbarRefresh) {//if switching from handsome to default :(
		if (!fDisableImgListRemap) {		
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 2);//nRemapState = 2 Undo  Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 3);//nRemapState = 3 Undo  Disabled
		}

		COLORSCHEME cs;
		cs.dwSize		= sizeof(COLORSCHEME);
		cs.clrBtnHighlight	= GetSysColor(COLOR_BTNFACE);
		cs.clrBtnShadow		= GetSysColor(COLOR_BTNSHADOW);

		GetToolBarCtrl().SetColorScheme(&cs);
		GetToolBarCtrl().SetIndent(0);

		if (::IsWindow(m_volctrl.GetSafeHwnd())) {
			m_volctrl.EnableWindow(FALSE);
			m_volctrl.EnableWindow(TRUE);
		}
		//reset flag from PPageTweaks
		s.fToolbarRefresh=false;
		//restore default theme?!
		if (HMODULE h = LoadLibrary(_T("uxtheme.dll"))) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
			if (f) {
				f(m_hWnd, L"Explorer", NULL);
			}
			FreeLibrary(h);
		}
	}

	CRect r;
	GetClientRect(&r);

	CRect br = GetBorders();

	CRect r9;
	GetItemRect(9, &r9);

	CRect vr;
	m_volctrl.GetClientRect(&vr);
	CRect vr2(r.right + br.right - 60, r.bottom - 25, r.right +br.right + 6, r.bottom);

	if (s.fDisableXPToolbars) {
		int m_nBMedian = r.bottom - 3 - 0.5 * m_nButtonHeight;
		vr2.SetRect(r.right + br.right - 60, m_nBMedian - 14, r.right +br.right + 6, m_nBMedian + 10);
	}

	m_volctrl.MoveWindow(vr2);

	UINT nID;
	UINT nStyle;
	int iImage;
	GetButtonInfo(11, nID, nStyle, iImage);
	SetButtonInfo(10, GetItemID(10), TBBS_SEPARATOR, vr2.left - iImage - r9.right - (r9.bottom - r9.top) + 10);
}

void CPlayerToolBar::SetMute(bool fMute)
{
	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	bi.iImage = fMute?13:12;
	tb.SetButtonInfo(ID_VOLUME_MUTE, &bi);

	AfxGetAppSettings().fMute = fMute;
}

bool CPlayerToolBar::IsMuted()
{
	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	tb.GetButtonInfo(ID_VOLUME_MUTE, &bi);
	return(bi.iImage==13);
}

int CPlayerToolBar::GetVolume()
{
	int volume = m_volctrl.GetPos(); // [0..100]
	if (IsMuted() || volume <= 0) {
		volume = -10000;
	} else {
		volume = min((int)(4000*log10(volume/100.0f)), 0); // 4000=2.0*100*20, where 2.0 is a special factor
	}

	return volume;
}

int CPlayerToolBar::GetMinWidth()
{
	return m_nButtonHeight * 9 + 155;
}

void CPlayerToolBar::SetVolume(int volume)
{
	m_volctrl.SetPosInternal(volume);
}

BEGIN_MESSAGE_MAP(CPlayerToolBar, CToolBar)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	ON_WM_SIZE()
	ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	ON_COMMAND_EX(ID_VOLUME_MUTE, OnVolumeMute)
	ON_UPDATE_COMMAND_UI(ID_VOLUME_MUTE, OnUpdateVolumeMute)

	ON_COMMAND_EX(ID_PLAY_PAUSE, OnPause)
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlay)
	ON_COMMAND_EX(ID_PLAY_STOP, OnStop)
	ON_COMMAND_EX(ID_FILE_CLOSEMEDIA, OnStop)

	ON_COMMAND_EX(ID_VOLUME_UP, OnVolumeUp)
	ON_COMMAND_EX(ID_VOLUME_DOWN, OnVolumeDown)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// CPlayerToolBar message handlers

void CPlayerToolBar::OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTBCUSTOMDRAW* pTBCD = reinterpret_cast<NMTBCUSTOMDRAW*>( pNMHDR );
	LRESULT lr = CDRF_DODEFAULT;
	AppSettings& s = AfxGetAppSettings();

	GRADIENT_RECT gr[1] = {{0, 1}};

	int sep[4] = {2, 7, 9, 10};

	if (s.fDisableXPToolbars) {
		iThemeBrightness = AfxGetAppSettings().nThemeBrightness;
		iThemeRed = AfxGetAppSettings().nThemeRed;
		iThemeGreen = AfxGetAppSettings().nThemeGreen;
		iThemeBlue = AfxGetAppSettings().nThemeBlue;
		int iRedRB, iGreenRB, iBlueRB, iRedLT, iGreenLT, iBlueLT, iAlphaLT, iAlphaRB;
		iRedLT		= 50 + iThemeBrightness;
		iGreenLT	= 55 + iThemeBrightness;
		iBlueLT		= 60 + iThemeBrightness;
		iAlphaLT	= 255;
		iRedRB		= 20 + iThemeBrightness;
		iGreenRB	= 25 + iThemeBrightness;
		iBlueRB		= 30 + iThemeBrightness;
		iAlphaRB	= 255;

		int fp = m_logobm.FileExists("background");

		switch(pTBCD->nmcd.dwDrawStage)
		{
		case CDDS_PREPAINT:
			{
			//TRACE(" TB-PREPAINT ");
			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;

			GetClientRect(&r);

			if (NULL != fp) {
				m_logobm.LoadExternalGradient("background", &dc, r, 21, iThemeBrightness, iThemeRed, iThemeGreen, iThemeBlue);
			} else {
				TRIVERTEX tv[2] = {
					{r.left, r.top, iRedLT*iThemeRed, iGreenLT*iThemeGreen, iBlueLT*iThemeBlue, iAlphaLT*256},
					{r.right, r.bottom, iRedRB*iThemeRed, iGreenRB*iThemeGreen, iBlueRB*iThemeBlue, iAlphaRB*256},
				};
				dc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
			}

			dc.Detach();
			}
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_POSTPAINT:
			//TRACE(" TB-POSTPAINT ");
			lr = CDRF_DODEFAULT;
			break;
		case CDDS_PREERASE:
			//same thing as onerasebkg return true?!
			//TRACE(" TB-PREERASE ");
			lr = CDRF_SKIPDEFAULT;
			break;
		case CDDS_ITEMPREPAINT:
			//TRACE(" TB-ITEMPREPAINT ");
			lr = CDRF_DODEFAULT;
			//Without XP-theming, buttons look bad over a dark background.
			if (s.fDisableXPToolbars)
			{
				//lr |= TBCDRF_HILITEHOTTRACK;//made custom glass-like hover
				lr |= TBCDRF_NOETCHEDEFFECT;
				lr |= TBCDRF_NOBACKGROUND;
				lr |= TBCDRF_NOEDGES;
				lr |= TBCDRF_NOOFFSET;
			}
			lr |= CDRF_NOTIFYPOSTPAINT;
			lr |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPOSTPAINT:
			//TRACE(" TB-ITEMPOSTPAINT ");
			lr = CDRF_DODEFAULT;
			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;
			CopyRect(&r,&pTBCD->nmcd.rc);

			CRect rGlassLike (0,0,8,8);
			int nW = rGlassLike.Width();
			int nH = rGlassLike.Height();
			CDC memdc;
			memdc.CreateCompatibleDC(&dc);
			CBitmap bmGlassLike;
			CBitmap *bmOld;
			bmGlassLike.CreateCompatibleBitmap(&dc, nW, nH);
			bmOld = memdc.SelectObject(&bmGlassLike);
			/*NOTE TO SELF: never ever do noob stuff like (CBrush*) (0x00ffffff)! AND debug release, too
			could not see it in debug, but in release the custom toolbar was failing to repaint from time to time
			I was blaming the PPageTweaks trigger at first, good thing that the detours library complained 
			about an access violation wich lead me here to fix it. Why did I use FillRect anyway */
			//CBrush brushGlassLike(0x00ffffff);
			memdc.FillSolidRect(rGlassLike, 0x00ffffff);//clr_resLight/white RGB(255,255,255)

			BLENDFUNCTION bf;  
			bf.AlphaFormat	= 0;
			bf.BlendFlags	= 0;
			bf.BlendOp	= AC_SRC_OVER;
			bf.SourceConstantAlpha = 80;

			CPen penFrHot (PS_SOLID,0,0x00e9e9e9);//clr_resFace	RGB(233,233,233)
			CPen penFrChecked (PS_SOLID,0,0x00808080);//clr_resDark	RGB(128,128,128)

			CPen *penSaved = dc.SelectObject(&penFrHot);
			CBrush *brushSaved = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

			//CDIS_SELECTED,CDIS_GRAYED,CDIS_DISABLED,CDIS_CHECKED,CDIS_FOCUS,CDIS_DEFAULT,CDIS_HOT,CDIS_MARKED,CDIS_INDETERMINATE
			if (CDIS_HOT == pTBCD->nmcd.uItemState || CDIS_CHECKED + CDIS_HOT == pTBCD->nmcd.uItemState) {
				dc.SelectObject(&penFrHot);
				dc.RoundRect(r.left +1,r.top +1,r.right -2,r.bottom -1, 6, 4);		
				AlphaBlend(dc.m_hDC, r.left +2,r.top +2, r.Width() -4, 0.5* r.Height() -2, memdc, 0, 0, nW, nH, bf);
			}

			if (CDIS_CHECKED == pTBCD->nmcd.uItemState) {
				dc.SelectObject(&penFrChecked);
				dc.RoundRect(r.left +1,r.top +1,r.right -2,r.bottom -1, 6, 4);
			}

			for (int j = 0; j < 4; j++) {
				GetItemRect(sep[j], &r);

				if (NULL != fp) {
					m_logobm.LoadExternalGradient("background", &dc, r, 21, iThemeBrightness, iThemeRed, iThemeGreen, iThemeBlue);
				} else {
					TRIVERTEX tv[2] = {
						{r.left, r.top, iRedLT*iThemeRed, iGreenLT*iThemeGreen, iBlueLT*iThemeBlue, iAlphaLT*256},
						{r.right, r.bottom, iRedRB*iThemeRed, iGreenRB*iThemeGreen, iBlueRB*iThemeBlue, iAlphaRB*256},
					};
					dc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
				}
			}

			dc.SelectObject(&penSaved);
			dc.SelectObject(&brushSaved);
			dc.Detach();
			DeleteObject(memdc.SelectObject(bmOld));
			memdc.DeleteDC();
			lr |= CDRF_SKIPDEFAULT;
			break;
		}
	} else {
		switch(pTBCD->nmcd.dwDrawStage)
		{
		case CDDS_ITEMPOSTPAINT:
			lr = CDRF_DODEFAULT;
			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;

			for (int j = 0; j < 4; j++) {
				GetItemRect(sep[j], &r);

				dc.FillSolidRect(r, GetSysColor(COLOR_WINDOW));
			}

			dc.Detach();
			lr |= CDRF_SKIPDEFAULT;
			break;
		}
	}

	*pResult = lr;
}

void CPlayerToolBar::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	ArrangeControls();
}

void CPlayerToolBar::OnInitialUpdate()
{
	ArrangeControls();
}

BOOL CPlayerToolBar::OnVolumeMute(UINT nID)
{
	SetMute(!IsMuted());

	if (::IsWindow(m_volctrl.GetSafeHwnd())) {
		m_volctrl.EnableWindow(FALSE);
		m_volctrl.EnableWindow(TRUE);
	}

	return FALSE;
}

void CPlayerToolBar::OnUpdateVolumeMute(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(IsMuted());
}

BOOL CPlayerToolBar::OnVolumeUp(UINT nID)
{
	m_volctrl.IncreaseVolume();
	return FALSE;
}

BOOL CPlayerToolBar::OnVolumeDown(UINT nID)
{
	m_volctrl.DecreaseVolume();
	return FALSE;
}

void CPlayerToolBar::OnMouseMove(UINT nFlags, CPoint point)
{
	int i = getHitButtonIdx(point);

	if ((i == -1) || (GetButtonStyle(i)&(TBBS_SEPARATOR|TBBS_DISABLED))) {
		;
	} else {
		if ((i>10) || ((i<9) && ((CMainFrame*)GetParentFrame())->IsSomethingLoaded())) {
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		}
	}
	__super::OnMouseMove(nFlags, point);
}

BOOL CPlayerToolBar::OnPlay(UINT nID)
{
	CMainFrame* pFrame	= ((CMainFrame*)GetParentFrame());
	OAFilterState fs	= pFrame->GetMediaState();

	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	tb.GetButtonInfo(ID_PLAY_PLAY, &bi);
	bi.iImage = (fs == State_Paused || fs == State_Stopped) ? 1 : 0;
	tb.SetButtonInfo(ID_PLAY_PLAY, &bi);

	return FALSE;
}

BOOL CPlayerToolBar::OnStop(UINT nID)
{
	CMainFrame* pFrame	= ((CMainFrame*)GetParentFrame());
	OAFilterState fs	= pFrame->GetMediaState();

	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	tb.GetButtonInfo(ID_PLAY_PLAY, &bi);
	bi.iImage = 0;
	tb.SetButtonInfo(ID_PLAY_PLAY, &bi);

	return FALSE;
}

BOOL CPlayerToolBar::OnPause(UINT nID)
{
	CMainFrame* pFrame	= ((CMainFrame*)GetParentFrame());
	OAFilterState fs	= pFrame->GetMediaState();

	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	tb.GetButtonInfo(ID_PLAY_PLAY, &bi);
	bi.iImage = (fs == State_Paused) ? 1 : 0;
	tb.SetButtonInfo(ID_PLAY_PLAY, &bi);
	
	return FALSE;
}

void CPlayerToolBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CMainFrame* pFrame	= ((CMainFrame*)GetParentFrame());
	int Idx				= getHitButtonIdx(point);
	OAFilterState fs	= pFrame->GetMediaState();

	if (Idx == 0 && fs != -1) {
		pFrame->PostMessage(WM_COMMAND, ID_PLAY_PLAYPAUSE);
	} else if ((Idx == -1) || (GetButtonStyle(Idx)&(TBBS_SEPARATOR|TBBS_DISABLED))) {
		if (!pFrame->m_fFullScreen) {
			MapWindowPoints(pFrame, &point, 1);
			pFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		}
	} else {
		if ((Idx>10) || ((Idx<9) && pFrame->IsSomethingLoaded())) {
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		}

		__super::OnLButtonDown(nFlags, point);
	}
}

int CPlayerToolBar::getHitButtonIdx(CPoint point)
{
	int hit = -1; // -1 means not on any buttons, mute button is 12/13, others < 10, 11 is empty space between
	CRect r;

	for (int i = 0, j = GetToolBarCtrl().GetButtonCount(); i < j; i++) {
		GetItemRect(i, r);

		if (r.PtInRect(point)) {
			hit = i;
			break;
		}
	}

	return hit;
}
