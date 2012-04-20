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
#include <libpng/png.h>


typedef HRESULT (__stdcall * SetWindowThemeFunct)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);


// CPlayerToolBar

IMPLEMENT_DYNAMIC(CPlayerToolBar, CToolBar)
CPlayerToolBar::CPlayerToolBar() : fDisableImgListRemap(false)//ins:2452 bobdynlan:dont remap if external bmp is used//
//	: m_nButtonHeight(16)
{
}

CPlayerToolBar::~CPlayerToolBar()
{
//	if (m_reImgListActive.GetSafeHandle()) m_reImgListActive.DeleteImageList();//ins:2452 bobdynlan:cleanup, not needed//
//	if (m_reImgListDisabled.GetSafeHandle()) m_reImgListDisabled.DeleteImageList();//ins:2452 bobdynlan:cleanup, not needed//
	if (NULL != m_pButtonsImages) {
		delete m_pButtonsImages;
		m_pButtonsImages = NULL;
	}}

HBITMAP CPlayerToolBar::LoadExternalToolBar()
{
	CString path;
	GetModuleFileName(AfxGetInstanceHandle(), path.GetBuffer(_MAX_PATH), _MAX_PATH);
	path.ReleaseBuffer();
	path = path.Left(path.ReverseFind('\\')+1);

	FILE* fp = _tfopen(path + _T("toolbar.png"), _T("rb"));
	if (fp) {
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR | PNG_TRANSFORM_PACKING, 0);

		png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
		int bpp = png_get_channels(png_ptr, info_ptr) * 8;
		int width = png_get_image_width(png_ptr, info_ptr);
		int memWidth = width * bpp / 8;
		int height = png_get_image_height(png_ptr, info_ptr);
		BYTE* pData;
		BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), width, -height, 1, bpp, BI_RGB, 0, 0, 0, 0, 0}};

		HBITMAP hbm = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&pData, 0, 0);
		for (int i = 0; i < height; i++) {
			memcpy(pData + memWidth * i, row_pointers[i], memWidth);
		}

		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose(fp);

		return hbm;
	} else {
		return (HBITMAP)LoadImage(NULL, path + _T("toolbar.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	}
}

BOOL CPlayerToolBar::Create(CWnd* pParentWnd)
{
	if (!__super::CreateEx(pParentWnd,
						  TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_AUTOSIZE/*ins:2233 bobdynlan:add erase msg in customdraw*/|TBSTYLE_CUSTOMERASE,
						  WS_CHILD|WS_VISIBLE|CBRS_ALIGN_BOTTOM|CBRS_TOOLTIPS/*rem:2233 bobdynlan:disabled borders for now, hate NC-area*, CRect(2,2,0,3)*/)) {
		return FALSE;
	}
	if (!LoadToolBar(IDB_PLAYERTOOLBAR)) {
		return FALSE;
	}

	// Should never be RTLed
	ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

	GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	CToolBarCtrl& tb = GetToolBarCtrl();
	tb.DeleteButton(tb.GetButtonCount()-1);
	tb.DeleteButton(tb.GetButtonCount()-1);

	SetMute(AfxGetAppSettings().fMute);

	UINT styles[] = {
		TBBS_CHECKGROUP, TBBS_CHECKGROUP, TBBS_CHECKGROUP,
		TBBS_SEPARATOR,
		TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON,
		TBBS_SEPARATOR,
		TBBS_BUTTON/*|TBSTYLE_DROPDOWN*/,
		TBBS_SEPARATOR,
		TBBS_SEPARATOR,
		TBBS_CHECKBOX,
		/*TBBS_SEPARATOR,*/
	};

	for (int i = 0; i < countof(styles); i++) {
		SetButtonStyle(i, styles[i]|TBBS_DISABLED);
	}

	m_volctrl.Create(this);
	m_volctrl.SetRange(0, 100);

	if (AfxGetAppSettings().fDisableXPToolbars) {
		if (HMODULE h = LoadLibrary(_T("uxtheme.dll"))) {
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
			if (f) {
				f(m_hWnd, L" ", L" ");
			}
			FreeLibrary(h);
		}
	}

	// quick and dirty code from foxx1337; will leak, but don't care yet
	m_nButtonHeight = 16; //reset m_nButtonHeight
	HBITMAP hBmp = LoadExternalToolBar();
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
			fDisableImgListRemap = true;//ins:2452 bobdynlan:dont remap if external bmp is used//
		}
		delete bmp;
		DeleteObject(hBmp);
	}
//INS:2452 bobdynlan: system-independent colors for rc toolbar buttons
//-----------------------------------------------------------------------------
	AppSettings& s = AfxGetAppSettings();
	if (s.fDisableXPToolbars)
	{
		if (!fDisableImgListRemap)
		{
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 0);//nRemapState = 0 Remap Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 1);//nRemapState = 1 Remap Disabled
		}
		//ins:2451 bobdynlan: blend in separators when no XP-theming//
		COLORSCHEME cs;
			cs.dwSize = sizeof(COLORSCHEME);
			cs.clrBtnHighlight = 0x0046413c; //clr_csLight = RGB( 60, 65, 70)
			cs.clrBtnShadow    = 0x0037322d;//clr_csShadow = RGB( 45, 50, 55)
		GetToolBarCtrl().SetColorScheme(&cs);
		//ins:2451 bobdynlan:toolbar left border//
		GetToolBarCtrl().SetIndent(5);
	}
//--------------------------------------------------------------------------INS
	return TRUE;
}

BOOL CPlayerToolBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!__super::PreCreateWindow(cs)) {
		return FALSE;
	}

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;
	//	m_dwStyle |= CBRS_SIZE_FIXED;

	return TRUE;
}

//INS:2452 bobdynlan: system-independent colors for rc toolbar buttons
//my first version was around foxx1337's implementation. 
//fine on create, but after adding switch support without restart, 
//gdi leaks counter increased with ~8 every time :)
//then I took a peek at XTrueColorToolBar to see if I can fix it
//got some skeleton parts of functions from there but did not help me much
//-----------------------------------------------------------------------------
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
	//int m_nBtnCount = bmInfo.bmWidth / bmInfo.bmHeight;
	//ASSERT(m_nBtnCount >= 10);
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
	if (nRemapState == 0 || nRemapState == 2)
	{
		if (m_reImgListActive.GetSafeHandle()) m_reImgListActive.DeleteImageList();//cleanup
		CreateRemappedImgList(bmID, nRemapState, m_reImgListActive);//remap
		ASSERT(m_reImgListActive.GetSafeHandle());
		ctrl.SetImageList(&m_reImgListActive);//switch to
	}
	else
	{
		if (m_reImgListDisabled.GetSafeHandle()) m_reImgListDisabled.DeleteImageList();//cleanup
		CreateRemappedImgList(bmID, nRemapState, m_reImgListDisabled);//remap
		ASSERT(m_reImgListDisabled.GetSafeHandle());
		ctrl.SetDisabledImageList(&m_reImgListDisabled);//switch to
	}
}
//--------------------------------------------------------------------------INS

void CPlayerToolBar::ArrangeControls()
{
	if (!::IsWindow(m_volctrl.m_hWnd)) {
		return;
	}
//INS:2451 bobdynlan: system-independent colors for rc toolbar buttons 
//-----------------------------------------------------------------------------
	AppSettings& s = AfxGetAppSettings();
	//TRACE("\nfToolbarRefresh=%d\n",s.fToolbarRefresh);
	if (s.fDisableXPToolbars && s.fToolbarRefresh)//if switching from default to handsome :)
	{
		if (!fDisableImgListRemap)
		{		
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 0);//nRemapState = 0 Remap Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 1);//nRemapState = 1 Remap Disabled
		}
		//ins:2451 bobdynlan: blend in separators when no XP-theming//
		COLORSCHEME cs;
			cs.dwSize = sizeof(COLORSCHEME);
			cs.clrBtnHighlight = 0x0046413c; //clr_csLight = RGB( 60, 65, 70)
			cs.clrBtnShadow    = 0x0037322d;//clr_csShadow = RGB( 45, 50, 55)
		GetToolBarCtrl().SetColorScheme(&cs);
		//ins:2451 bobdynlan:toolbar left border//
		GetToolBarCtrl().SetIndent(5);
		//ins:2233 bobdynlan: refresh volume control, too
		if (::IsWindow(m_volctrl.GetSafeHwnd()))
		{
			m_volctrl.EnableWindow(FALSE);
			m_volctrl.EnableWindow(TRUE);
		}
		//reset flag from PPageTweaks
		s.fToolbarRefresh=false;
		//kill default theme?!
		if (HMODULE h = LoadLibrary(_T("uxtheme.dll")))
		{
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
			if (f) f(m_hWnd, L" ", L" ");
			FreeLibrary(h);
		}
	}
	else if (!s.fDisableXPToolbars && s.fToolbarRefresh)//if switching from handsome to default :(
	{
		if (!fDisableImgListRemap)
		{		
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 2);//nRemapState = 2 Undo  Active
			SwitchRemmapedImgList(IDB_PLAYERTOOLBAR, 3);//nRemapState = 3 Undo  Disabled
		}
		//ins:2451 bobdynlan: blend in separators when no XP-theming//
		COLORSCHEME cs;
			cs.dwSize = sizeof(COLORSCHEME);
			cs.clrBtnHighlight = GetSysColor(COLOR_BTNFACE);
			cs.clrBtnShadow    = GetSysColor(COLOR_BTNSHADOW);
		GetToolBarCtrl().SetColorScheme(&cs);
		//ins:2451 bobdynlan:toolbar left border//
		GetToolBarCtrl().SetIndent(0);
		//ins:2233 bobdynlan: refresh volume control, too
		if (::IsWindow(m_volctrl.GetSafeHwnd()))
		{
			m_volctrl.EnableWindow(FALSE);
			m_volctrl.EnableWindow(TRUE);
		}
		//reset flag from PPageTweaks
		s.fToolbarRefresh=false;
		//restore default theme?!
		if (HMODULE h = LoadLibrary(_T("uxtheme.dll")))
		{
			SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
			if (f) f(m_hWnd, L"Explorer", NULL);
			FreeLibrary(h);
		}
	}
//--------------------------------------------------------------------------INS
	CRect r;
	GetClientRect(&r);

	CRect br = GetBorders();

	CRect r10;
	GetItemRect(10, &r10);

	CRect vr;
	m_volctrl.GetClientRect(&vr);
	CRect vr2(r.right + br.right - 60, r.bottom - 25, r.right +br.right + 6, r.bottom);
//INS:2451 bobdynlan: centered volume control with large external toolbar bitmap
//-----------------------------------------------------------------------------
	if (s.fDisableXPToolbars)
	{
		int m_nBMedian = r.bottom - 3 - 0.5 * m_nButtonHeight;
		vr2.SetRect(r.right + br.right - 60, m_nBMedian - 14, r.right +br.right + 6, m_nBMedian + 11);
	}
//--------------------------------------------------------------------------INS
	m_volctrl.MoveWindow(vr2);

	UINT nID;
	UINT nStyle;
	int iImage;
	GetButtonInfo(12, nID, nStyle, iImage);
	SetButtonInfo(11, GetItemID(11), TBBS_SEPARATOR, vr2.left - iImage - r10.right - (r10.bottom - r10.top) + 11);
	//SetButtonInfo(10, GetItemID(10),TBBS_SEPARATOR, 1);//ins:2451 bobdynlan:litle hack to hide dummy separator, XP-theming only//
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
	/*
		volume = (int)pow(10, ((double)volume)/5000+2);
		volume = max(min(volume, 100), 1);
	*/
	m_volctrl.SetPosInternal(volume);
}

BEGIN_MESSAGE_MAP(CPlayerToolBar, CToolBar)
	//rem:2103 bobdynlan:paint on customdraw//ON_WM_PAINT()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)//ins:2217 bobdynlan:customdraw//
	ON_WM_SIZE()
	ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	ON_COMMAND_EX(ID_VOLUME_MUTE, OnVolumeMute)
	ON_UPDATE_COMMAND_UI(ID_VOLUME_MUTE, OnUpdateVolumeMute)
	ON_COMMAND_EX(ID_VOLUME_UP, OnVolumeUp)
	ON_COMMAND_EX(ID_VOLUME_DOWN, OnVolumeDown)
	//rem:2103 bobdynlan:no more NC-area//ON_WM_NCPAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// CPlayerToolBar message handlers

//INS:2217 bobdynlan: customdraw
//-----------------------------------------------------------------------------
void CPlayerToolBar::OnCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{  
	//if (m_bDelayedButtonLayout) Layout(); //I guess this is not needed on customdraw
	NMTBCUSTOMDRAW* pTBCD = reinterpret_cast<NMTBCUSTOMDRAW*>( pNMHDR );
	LRESULT lr = CDRF_DODEFAULT;
	AppSettings& s = AfxGetAppSettings();
	if (s.fDisableXPToolbars)
	{
		switch(pTBCD->nmcd.dwDrawStage)
		{
		case CDDS_PREPAINT:
			{
			//TRACE(" TB-PREPAINT ");
			CDC dc;
			dc.Attach(pTBCD->nmcd.hdc);
			CRect r;
			GetClientRect(&r);
			TRIVERTEX tv[2] = {
				//bobdynlan: {x, y, Red*256, Green*256, Blue*256, Alpha*256}
				{ r.left, r.top, 85*256, 90*256, 95*256, 255*256},//same as PlayerSeekBar gradient - bottom
				{ r.right, r.bottom, 15*256, 20*256, 25*256, 255*256}
			};
			GRADIENT_RECT gr[1] = {
				{0, 1},
			};
			dc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
			dc.Detach();
			}
			lr |= CDRF_NOTIFYITEMDRAW;
			//lr |= CDRF_NOTIFYPOSTPAINT;
			break;
		case CDDS_POSTPAINT:
			TRACE(" TB-POSTPAINT ");
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
				bf.AlphaFormat = 0;
				bf.BlendFlags = 0;
				bf.BlendOp = AC_SRC_OVER;
				bf.SourceConstantAlpha = 80;

			CPen penFrHot (PS_SOLID,0,0x00e9e9e9);//clr_resFace	RGB(233,233,233)
			CPen penFrChecked (PS_SOLID,0,0x00808080);//clr_resDark	RGB(128,128,128)

			CPen *penSaved = dc.SelectObject(&penFrHot);
			CBrush *brushSaved = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

			//CDIS_SELECTED,CDIS_GRAYED,CDIS_DISABLED,CDIS_CHECKED,CDIS_FOCUS,CDIS_DEFAULT,CDIS_HOT,CDIS_MARKED,CDIS_INDETERMINATE
			if (CDIS_HOT == pTBCD->nmcd.uItemState || CDIS_CHECKED + CDIS_HOT == pTBCD->nmcd.uItemState) 
			{
				dc.SelectObject(&penFrHot);
				dc.RoundRect(r.left +1,r.top +1,r.right -2,r.bottom -1, 6, 4);		
				AlphaBlend(dc.m_hDC, r.left +2,r.top +2, r.Width() -4, 0.5* r.Height() -2, memdc, 0, 0, nW, nH, bf);
			}
			if (CDIS_CHECKED == pTBCD->nmcd.uItemState)		
			{
				dc.SelectObject(&penFrChecked);
				dc.RoundRect(r.left +1,r.top +1,r.right -2,r.bottom -1, 6, 4);
			}
			
//----------INS: judelaw : Hide SEPARATORS ---- BEGIN
			//Hide Separator3
			CRect r3;
			GetItemRect(3, &r3);
			TRIVERTEX tv3[2] = {
				{ r3.left, r3.top, 85*256, 90*256, 95*256, 255*256},
				{ r3.right, r3.bottom, 15*256, 20*256, 25*256, 255*256},
			};
			GRADIENT_RECT gr3[1] = {{0, 1},};
			dc.GradientFill(tv3, 2, gr3, 1, GRADIENT_FILL_RECT_V);

			//Hide Separator8
			CRect r8;
			GetItemRect(8, &r8);
			TRIVERTEX tv8[2] = {
				{ r8.left, r8.top, 85*256, 90*256, 95*256, 255*256},
				{ r8.right, r8.bottom, 15*256, 20*256, 25*256, 255*256},
			};
			GRADIENT_RECT gr8[1] = {{0, 1},};
			dc.GradientFill(tv8, 2, gr8, 1, GRADIENT_FILL_RECT_V);

			//Hide Separator10
			CRect r10;
			GetItemRect(10, &r10);
			TRIVERTEX tv10[2] = {
				{ r10.left, r10.top, 85*256, 90*256, 95*256, 255*256},
				{ r10.right, r10.bottom, 15*256, 20*256, 25*256, 255*256},
			};
			GRADIENT_RECT gr10[1] = {{0, 1},};
			dc.GradientFill(tv10, 2, gr10, 1, GRADIENT_FILL_RECT_V);

			//Hide Separator11
			CRect r11;
			GetItemRect(11, &r11);
			TRIVERTEX tv11[2] = {
				{ r11.left, r11.top, 85*256, 90*256, 95*256, 255*256},
				{ r11.right, r11.bottom, 15*256, 20*256, 25*256, 255*256},
			};
			GRADIENT_RECT gr11[1] = {{0, 1},};
			dc.GradientFill(tv11, 2, gr11, 1, GRADIENT_FILL_RECT_V);
//----------INS: judelaw : Hide SEPARATORS ------ END
			
			dc.SelectObject(&penSaved);
			dc.SelectObject(&brushSaved);
			dc.Detach();
			DeleteObject(memdc.SelectObject(bmOld));
			memdc.DeleteDC();
			//TRACE("\n\n uItemState=%2x (%2d,%2d,W=%2d,H=%2d)\n\n",pTBCD->nmcd.uItemState, r.left, r.top, r.Width(), r.Height()); 
			lr |= CDRF_SKIPDEFAULT;
			break;
		}
	}
	*pResult = lr;
}
//--------------------------------------------------------------------------INS
//REM:2103 bobdynlan: backup, no need to paint resized item 11 when customdraw
//-----------------------------------------------------------------------------
/*
void CPlayerToolBar::OnPaint()
{
	if (m_bDelayedButtonLayout) {
		Layout();
	}

	CPaintDC dc(this); // device context for painting

	DefWindowProc(WM_PAINT, WPARAM(dc.m_hDC), 0);

	{
		UINT nID;
		UINT nStyle = 0;
		int iImage = 0;
		GetButtonInfo(11, nID, nStyle, iImage);
		CRect ItemRect;
		GetItemRect(11, ItemRect);
		dc.FillSolidRect(ItemRect, GetSysColor(COLOR_BTNFACE));
	}
}
*/
//--------------------------------------------------------------------------REM

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
//INS:2233 bobdynlan: refresh volume control
//-----------------------------------------------------------------------------
	if (::IsWindow(m_volctrl.GetSafeHwnd()))
	{
		m_volctrl.EnableWindow(FALSE);
		m_volctrl.EnableWindow(TRUE);
	}
//--------------------------------------------------------------------------INS
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

//REM:2103 bobdynlan: backup, removed borders from Create so no more NC-area
//-----------------------------------------------------------------------------
/*
void CPlayerToolBar::OnNcPaint() // when using XP styles the NC area isn't drawn for our toolbar...
{
	CRect wr, cr;

	CWindowDC dc(this);
	GetClientRect(&cr);
	ClientToScreen(&cr);
	GetWindowRect(&wr);
	cr.OffsetRect(-wr.left, -wr.top);
	wr.OffsetRect(-wr.left, -wr.top);
	dc.ExcludeClipRect(&cr);
	dc.FillSolidRect(wr, GetSysColor(COLOR_BTNFACE));

	// Do not call CToolBar::OnNcPaint() for painting messages
}
*/
//--------------------------------------------------------------------------REM

void CPlayerToolBar::OnMouseMove(UINT nFlags, CPoint point)
{
	int i = getHitButtonIdx(point);

	if ((i==-1) || (GetButtonStyle(i)&(TBBS_SEPARATOR|TBBS_DISABLED))) {
		;
	} else {
		if ((i>11) || ((i<10) && ((CMainFrame*)GetParentFrame())->IsSomethingLoaded())) {
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		}
	}
	__super::OnMouseMove(nFlags, point);
}

void CPlayerToolBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	int i = getHitButtonIdx(point);
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());

	if ((i==-1) || (GetButtonStyle(i)&(TBBS_SEPARATOR|TBBS_DISABLED))) {
		if (!pFrame->m_fFullScreen) {
			MapWindowPoints(pFrame, &point, 1);
			pFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		}
	} else {
		if ((i>11) || ((i<10) && pFrame->IsSomethingLoaded())) {
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
