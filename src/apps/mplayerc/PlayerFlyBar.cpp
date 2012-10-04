/*
 * $Id$
 *
 * Copyright (C) 2012 Sergey "judelaw"
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
#include "PlayerFlyBar.h"
#include "MainFrm.h"


// CPrevView

CFlyBar::CFlyBar() : 
	bt_idx(-1),
	//ih(24),
	iw(24),
	r_ExitIcon(0,0,0,0),
	r_MinIcon(0,0,0,0),
	r_RestoreIcon(0,0,0,0),
	r_SettingsIcon(0,0,0,0),
	r_InfoIcon(0,0,0,0),
	r_FSIcon(0,0,0,0),
	r_LockIcon(0,0,0,0)
{
	SetDefault();
}

CFlyBar::~CFlyBar()
{
	if (m_ExitIcon_a) DestroyIcon(m_ExitIcon_a);
	if (m_ExitIcon) DestroyIcon(m_ExitIcon);
	if (m_MinIcon_a) DestroyIcon(m_MinIcon_a);
	if (m_MinIcon) DestroyIcon(m_MinIcon);
	if (m_MaxIcon_a) DestroyIcon(m_MaxIcon_a);
	if (m_MaxIcon) DestroyIcon(m_MaxIcon);
	if (m_RestoreIcon_a) DestroyIcon(m_RestoreIcon_a);
	if (m_RestoreIcon) DestroyIcon(m_RestoreIcon);
	if (m_SettingsIcon_a) DestroyIcon(m_SettingsIcon_a);
	if (m_SettingsIcon) DestroyIcon(m_SettingsIcon);
	if (m_InfoIcon_a) DestroyIcon(m_InfoIcon_a);
	if (m_InfoIcon) DestroyIcon(m_InfoIcon);
	if (m_FSIcon_a) DestroyIcon(m_FSIcon_a);
	if (m_FSIcon) DestroyIcon(m_FSIcon);
	if (m_WindowIcon_a) DestroyIcon(m_WindowIcon_a);
	if (m_WindowIcon) DestroyIcon(m_WindowIcon);
	if (m_LockIcon) DestroyIcon(m_LockIcon);
	if (m_UnLockIcon) DestroyIcon(m_UnLockIcon);
	if (m_LockIcon_a) DestroyIcon(m_LockIcon_a);
	if (m_UnLockIcon_a) DestroyIcon(m_UnLockIcon_a);
}


IMPLEMENT_DYNAMIC(CFlyBar, CWnd)

BEGIN_MESSAGE_MAP(CFlyBar, CWnd)
	ON_WM_CREATE()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CFlyBar message handlers

BOOL CFlyBar::PreTranslateMessage(MSG* pMsg)
{
	m_tooltip.RelayEvent(pMsg);
	
	switch (pMsg->message) {
		case WM_LBUTTONDOWN :
		case WM_LBUTTONDBLCLK :
		case WM_MBUTTONDOWN :
		case WM_MBUTTONUP :
		case WM_MBUTTONDBLCLK :
		case WM_RBUTTONDOWN :
		case WM_RBUTTONUP :
		case WM_RBUTTONDBLCLK :

		CMainFrame* pFrame	= (CMainFrame*)GetParentFrame();
		pFrame->SetFocus();
		break;
	}

	return CWnd::PreTranslateMessage(pMsg);
}


LRESULT CFlyBar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_MOUSELEAVE) {
		m_tooltip.UpdateTipText(_T(""), this);
	}

	return CWnd::WindowProc(message, wParam, lParam);
}

BOOL CFlyBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;

	return TRUE;
}

int CFlyBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}
	
	m_tooltip.Create(this, TTS_ALWAYSTIP);
	EnableToolTips(true);
	
	CRect r;
	GetWindowRect(&r);
	m_tooltip.AddTool(this,(LPTSTR)(LPCTSTR)_T(""), r);
	m_tooltip.Activate(TRUE);

	m_tooltip.SetDelayTime(TTDT_AUTOPOP, -1);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 500);
	m_tooltip.SetDelayTime(TTDT_RESHOW, 0);

	return 0;
}

void CFlyBar::CalcButtonsRect()
{
	CRect rcBar;
	GetWindowRect(&rcBar);
	
	CRect r_Exit(rcBar.right-4-(iw), rcBar.top+4, rcBar.right-4, rcBar.bottom-4);
	CRect r_Restore(rcBar.right-4-(iw*2), rcBar.top+4, rcBar.right-4-(iw), rcBar.bottom-4);
	CRect r_Min(rcBar.right-4-(iw*3), rcBar.top+4, rcBar.right-4-(iw*2), rcBar.bottom-4);
	CRect r_FS(rcBar.right-4-(iw*4), rcBar.top+4, rcBar.right-4-(iw*3), rcBar.bottom-4);
	CRect r_Info(rcBar.right-4-(iw*6), rcBar.top+4, rcBar.right-4-(iw*5), rcBar.bottom-4);
	CRect r_Settings(rcBar.right-4-(iw*7), rcBar.top+4, rcBar.right-4-(iw*6), rcBar.bottom-4);
	CRect r_Lock(rcBar.right-4-(iw*9), rcBar.top+4, rcBar.right-4-(iw*8), rcBar.bottom-4);

	r_ExitIcon		= r_Exit;
	r_MinIcon		= r_Min;
	r_RestoreIcon	= r_Restore;
	r_SettingsIcon	= r_Settings;
	r_InfoIcon		= r_Info;
	r_FSIcon		= r_FS;
	r_LockIcon		= r_Lock;
}


void CFlyBar::OnLButtonUp(UINT nFlags, CPoint point)
{

	CMainFrame* pFrame	= (CMainFrame*)GetParentFrame();
	pFrame->SetFocus();
	
	CRect rcBar;
	GetWindowRect(rcBar);
	WINDOWPLACEMENT wp;
	pFrame->GetWindowPlacement(&wp);
	CPoint p;
	GetCursorPos(&p);

	CalcButtonsRect();

	bt_idx = -1;
	if (r_ExitIcon.PtInRect(p)) {
		ShowWindow(SW_HIDE);
		pFrame->OnClose();
	} else if (r_MinIcon.PtInRect(p)) {
		pFrame->ShowWindow(SW_SHOWMINIMIZED );
	} else if (r_RestoreIcon.PtInRect(p) && !pFrame->m_fFullScreen) {
		
		if (wp.showCmd == SW_SHOWMAXIMIZED) {
			pFrame->ShowWindow(SW_SHOWNORMAL);
		} else if (wp.showCmd != SW_SHOWMAXIMIZED) {
			pFrame->ShowWindow(SW_SHOWMAXIMIZED);
		}
		Invalidate();
	} else if (r_SettingsIcon.PtInRect(p)) {
		pFrame->OnViewOptions();
		Invalidate();
	} else if (r_InfoIcon.PtInRect(p)) {
		OAFilterState fs	= pFrame->GetMediaState();
		if (fs != -1) pFrame->OnFileProperties();
		Invalidate();
	} else if (r_FSIcon.PtInRect(p) && wp.showCmd != SW_SHOWMAXIMIZED) {
		pFrame->ToggleFullscreen(true, true);
		Invalidate();
	} else if (r_LockIcon.PtInRect(p)) {
		AfxGetAppSettings().fFlybarOnTop = !AfxGetAppSettings().fFlybarOnTop;
	}
}

void CFlyBar::OnMouseMove(UINT nFlags, CPoint point)
{
	SetCursor(LoadCursor(NULL, IDC_HAND));

	CRect rcBar;
	GetWindowRect(&rcBar);

	ClientToScreen(&point);

	int ibt = bt_idx;
	CString str, str2;
	m_tooltip.GetText(str,this);
	
	if (r_ExitIcon.PtInRect(point)) {
		if (str != ResStr(IDS_AG_EXIT)) m_tooltip.UpdateTipText(ResStr(IDS_AG_EXIT), this);
		bt_idx = 0;
	} else if (r_MinIcon.PtInRect(point)) {
		if (str != ResStr(IDS_TOOLTIP_MINIMIZE)) m_tooltip.UpdateTipText(ResStr(IDS_TOOLTIP_MINIMIZE), this);
		bt_idx = 1;
	} else if (r_RestoreIcon.PtInRect(point)) {
		CMainFrame* pFrame	= (CMainFrame*)GetParentFrame();
		WINDOWPLACEMENT wp;
		pFrame->GetWindowPlacement(&wp);
		wp.showCmd == SW_SHOWMAXIMIZED ? str2 = ResStr(IDS_TOOLTIP_MINIMIZE) : str2 = ResStr(IDS_TOOLTIP_MAXIMIZE);
		if (str != str2) m_tooltip.UpdateTipText(str2, this);
		bt_idx = 2;
	} else if (r_SettingsIcon.PtInRect(point)) {
		if (str != ResStr(IDS_AG_OPTIONS)) m_tooltip.UpdateTipText(ResStr(IDS_AG_OPTIONS), this);
		bt_idx = 3;
	} else if (r_InfoIcon.PtInRect(point)) {
		if (str != ResStr(IDS_AG_PROPERTIES)) m_tooltip.UpdateTipText(ResStr(IDS_AG_PROPERTIES), this);
		bt_idx = 4;
	} else if (r_FSIcon.PtInRect(point)) {
		CMainFrame* pFrame	= (CMainFrame*)GetParentFrame();
		pFrame->m_fFullScreen ? str2 = ResStr(IDS_TOOLTIP_WINDOW) : str2 = ResStr(IDS_TOOLTIP_FULLSCREEN);
		if (str != str2) m_tooltip.UpdateTipText(str2, this);
		bt_idx = 5;
	} else if (r_LockIcon.PtInRect(point)) {
		AppSettings& s = AfxGetAppSettings();
		s.fFlybarOnTop ? str2 = ResStr(IDS_TOOLTIP_UNLOCK) : str2 = ResStr(IDS_TOOLTIP_LOCK);
		if (str != str2) m_tooltip.UpdateTipText(str2, this);
		bt_idx = 6;
	} else {
		if (str != _T("")) m_tooltip.UpdateTipText(_T(""), this);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		bt_idx = -1;
	}

	// set tooltip position
	CRect r_tooltip;
	m_tooltip.GetWindowRect(&r_tooltip);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(this->m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
	CPoint p;
	p.x = max(0, min(point.x, mi.rcMonitor.right - r_tooltip.Width()));
	int iCursorHeight = 24; // ???
	m_tooltip.SetWindowPos(NULL, p.x, point.y + iCursorHeight, r_tooltip.Width(), iCursorHeight, SWP_NOACTIVATE|SWP_NOZORDER);

	Invalidate();

	//CWnd::OnMouseMove(nFlags, point);
}

void CFlyBar::OnPaint()
{
	CPaintDC dc(this);

	if (IsWindowVisible()) {
		AppSettings& s = AfxGetAppSettings();

		CRect rcBar;
		GetClientRect(&rcBar);
		ClientToScreen(&rcBar);

		CMainFrame* pFrame	= (CMainFrame*)GetParentFrame();
		WINDOWPLACEMENT wp;
		pFrame->GetWindowPlacement(&wp);
		OAFilterState fs	= pFrame->GetMediaState();
		CDC mdc;
		mdc.CreateCompatibleDC(&dc);
		CBitmap bm;
		bm.CreateCompatibleBitmap(&dc, rcBar.Width(), rcBar.Height());
		CBitmap* pOldBm = mdc.SelectObject(&bm);
		mdc.SetBkMode(TRANSPARENT);

//		SetDefault();

		DrawIconEx(mdc, rcBar.Width()-4-(iw), 4, m_ExitIcon, 0,0, 0, NULL, DI_NORMAL);
		DrawIconEx(mdc, rcBar.Width()-4-(iw*2), 4, wp.showCmd == SW_SHOWMAXIMIZED ? m_RestoreIcon : m_MaxIcon, 0,0, 0, NULL, DI_NORMAL);
		DrawIconEx(mdc, rcBar.Width()-4-(iw*3), 4, m_MinIcon, 0,0, 0, NULL, DI_NORMAL);
		DrawIconEx(mdc, rcBar.Width()-4-(iw*4), 4, pFrame->m_fFullScreen ? m_WindowIcon : m_FSIcon, 0,0, 0, NULL, DI_NORMAL);
		DrawIconEx(mdc, rcBar.Width()-4-(iw*6), 4, m_InfoIcon, 0,0, 0, NULL, DI_NORMAL);
		DrawIconEx(mdc, rcBar.Width()-4-(iw*7), 4, m_SettingsIcon, 0,0, 0, NULL, DI_NORMAL);
		DrawIconEx(mdc, rcBar.Width()-4-(iw*9), 4, s.fFlybarOnTop ? m_LockIcon : m_UnLockIcon, 0,0, 0, NULL, DI_NORMAL);

		switch (bt_idx) {
			case -1:
				break;
			case 0:
				DrawIconEx(mdc, rcBar.Width()-4-(iw), 4, m_ExitIcon_a, 0,0, 0, NULL, DI_NORMAL);
				break;
			case 1:
				DrawIconEx(mdc, rcBar.Width()-4-(iw*3), 4, m_MinIcon_a, 0,0, 0, NULL, DI_NORMAL);
				break;
			case 2:
				DrawIconEx(mdc, rcBar.Width()-4-(iw*2), 4, wp.showCmd == SW_SHOWMAXIMIZED ? m_RestoreIcon_a : (pFrame->m_fFullScreen ? m_MaxIcon : m_MaxIcon_a), 0,0, 0, NULL, DI_NORMAL);
				break;
			case 3:
				DrawIconEx(mdc, rcBar.Width()-4-(iw*7), 4, m_SettingsIcon_a, 0,0, 0, NULL, DI_NORMAL);
				break;
			case 4:
				DrawIconEx(mdc, rcBar.Width()-4-(iw*6), 4, fs !=-1 ? m_InfoIcon_a : m_InfoIcon, 0,0, 0, NULL, DI_NORMAL);
				break;
			case 5:
				DrawIconEx(mdc, rcBar.Width()-4-(iw*4), 4, pFrame->m_fFullScreen ? m_WindowIcon_a : (wp.showCmd == SW_SHOWMAXIMIZED ? m_FSIcon : m_FSIcon_a), 0,0, 0, NULL, DI_NORMAL);
				break;
			case 6:
				DrawIconEx(mdc, rcBar.Width()-4-(iw*9), 4, s.fFlybarOnTop ? m_LockIcon_a : m_UnLockIcon_a, 0,0, 0, NULL, DI_NORMAL);
				break;
		}

		dc.BitBlt(0, 0, rcBar.Width(), rcBar.Height(), &mdc, 0, 0, SRCCOPY);

		mdc.SelectObject(pOldBm);
		bm.DeleteObject();
		mdc.DeleteDC();
	}
	bt_idx = -1;
}


void CFlyBar::SetDefault()
{
	if (m_ExitIcon_a) DestroyIcon(m_ExitIcon_a);
	if (m_ExitIcon) DestroyIcon(m_ExitIcon);
	if (m_MinIcon_a) DestroyIcon(m_MinIcon_a);
	if (m_MinIcon) DestroyIcon(m_MinIcon);
	if (m_MaxIcon_a) DestroyIcon(m_MaxIcon_a);
	if (m_MaxIcon) DestroyIcon(m_MaxIcon);
	if (m_RestoreIcon_a) DestroyIcon(m_RestoreIcon_a);
	if (m_RestoreIcon) DestroyIcon(m_RestoreIcon);
	if (m_SettingsIcon_a) DestroyIcon(m_SettingsIcon_a);
	if (m_SettingsIcon) DestroyIcon(m_SettingsIcon);
	if (m_InfoIcon_a) DestroyIcon(m_InfoIcon_a);
	if (m_InfoIcon) DestroyIcon(m_InfoIcon);
	if (m_FSIcon_a) DestroyIcon(m_FSIcon_a);
	if (m_FSIcon) DestroyIcon(m_FSIcon);
	if (m_WindowIcon_a) DestroyIcon(m_WindowIcon_a);
	if (m_WindowIcon) DestroyIcon(m_WindowIcon);
	if (m_LockIcon) DestroyIcon(m_LockIcon);
	if (m_UnLockIcon) DestroyIcon(m_UnLockIcon);
	if (m_LockIcon_a) DestroyIcon(m_LockIcon_a);
	if (m_UnLockIcon_a) DestroyIcon(m_UnLockIcon_a);

	m_ExitIcon_a		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_EXIT_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_ExitIcon			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_EXIT), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_MinIcon_a			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_MINIMIZE_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_MinIcon			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_MINIMIZE), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_MaxIcon_a			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_MAXIMIZE_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_MaxIcon			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_MAXIMIZE), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_RestoreIcon_a		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_RESTORE_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_RestoreIcon		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_RESTORE), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_SettingsIcon_a	= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_SETTINGS_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_SettingsIcon		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_SETTINGS), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_InfoIcon_a		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_INFO_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_InfoIcon			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_INFO), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_FSIcon_a			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_FULLSCREEN_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_FSIcon			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_FULLSCREEN), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_WindowIcon_a		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_WINDOW_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_WindowIcon		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_WINDOW), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_LockIcon			= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_LOCK), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_UnLockIcon		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_UNLOCK), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_LockIcon_a		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_LOCK_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	m_UnLockIcon_a		= (HICON)LoadImage(AfxGetInstanceHandle(),  MAKEINTRESOURCE(IDR_FB_UNLOCK_A), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
	
	bt_idx = -1;
}