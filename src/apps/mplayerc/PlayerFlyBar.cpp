/*
 * $Id$
 *
 * Copyright (C) 2012 Sergey "judelaw" and Sergey "Exodus8"
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
	r_ExitIcon(0,0,0,0),
	r_MinIcon(0,0,0,0),
	r_RestoreIcon(0,0,0,0),
	r_SettingsIcon(0,0,0,0),
	r_InfoIcon(0,0,0,0),
	r_FSIcon(0,0,0,0),
	r_LockIcon(0,0,0,0)
{
	hBmp = m_logobm.LoadExternalImage("flybar", -1, -1, -1, -1);
	BITMAP bm;
	::GetObject(hBmp, sizeof(bm), &bm);
	iw = bm.bmHeight;
}

CFlyBar::~CFlyBar()
{
	Destroy();
}

void CFlyBar::Destroy()
{
	DeleteObject(hBmp);
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
	
	m_tooltip.AddTool(this, _T(""));
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

	r_ExitIcon		= CRect(rcBar.right-4-(iw),	rcBar.top+4, rcBar.right-4, rcBar.bottom-4);
	r_MinIcon		= CRect(rcBar.right-4-(iw*3), rcBar.top+4, rcBar.right-4-(iw*2), rcBar.bottom-4);
	r_RestoreIcon	= CRect(rcBar.right-4-(iw*2), rcBar.top+4, rcBar.right-4-(iw), rcBar.bottom-4);
	r_SettingsIcon	= CRect(rcBar.right-4-(iw*7), rcBar.top+4, rcBar.right-4-(iw*6), rcBar.bottom-4);
	r_InfoIcon		= CRect(rcBar.right-4-(iw*6), rcBar.top+4, rcBar.right-4-(iw*5), rcBar.bottom-4);
	r_FSIcon		= CRect(rcBar.right-4-(iw*4), rcBar.top+4, rcBar.right-4-(iw*3), rcBar.bottom-4);
	r_LockIcon		= CRect(rcBar.right-4-(iw*9), rcBar.top+4, rcBar.right-4-(iw*8), rcBar.bottom-4);
}

void CFlyBar::DrawBitmap(CDC *pDC, int x, int y, int z)
{
	CDC *mdci = GetDC(), hdcSrc;
	hdcSrc.CreateCompatibleDC(mdci);
	hdcSrc.SelectObject(CBitmap::FromHandle(hBmp));

	pDC->StretchBlt(x - 4 - (iw * z), 4, iw, iw, &hdcSrc, iw * y, 0, iw, iw, SRCCOPY);

	hdcSrc.DeleteDC();
	mdci->DeleteDC();
}

void CFlyBar::OnLButtonUp(UINT nFlags, CPoint point)
{

	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
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
		pFrame->ShowWindow(SW_SHOWMINIMIZED);
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
		OAFilterState fs = pFrame->GetMediaState();
		if (fs != -1) {
			pFrame->OnFileProperties();
		}
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
		if (str != ResStr(IDS_AG_EXIT)) {
			m_tooltip.UpdateTipText(ResStr(IDS_AG_EXIT), this);
		}
		bt_idx = 0;
	} else if (r_MinIcon.PtInRect(point)) {
		if (str != ResStr(IDS_TOOLTIP_MINIMIZE)) {
			m_tooltip.UpdateTipText(ResStr(IDS_TOOLTIP_MINIMIZE), this);
		}
		bt_idx = 1;
	} else if (r_RestoreIcon.PtInRect(point)) {
		CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
		WINDOWPLACEMENT wp;
		pFrame->GetWindowPlacement(&wp);
		wp.showCmd == SW_SHOWMAXIMIZED ? str2 = ResStr(IDS_TOOLTIP_RESTORE) : str2 = ResStr(IDS_TOOLTIP_MAXIMIZE);
		if (str != str2) {
			m_tooltip.UpdateTipText(str2, this);
		}
		bt_idx = 2;
	} else if (r_SettingsIcon.PtInRect(point)) {
		if (str != ResStr(IDS_AG_OPTIONS)) {
			m_tooltip.UpdateTipText(ResStr(IDS_AG_OPTIONS), this);
		}
		bt_idx = 3;
	} else if (r_InfoIcon.PtInRect(point)) {
		if (str != ResStr(IDS_AG_PROPERTIES)) {
			m_tooltip.UpdateTipText(ResStr(IDS_AG_PROPERTIES), this);
		}
		bt_idx = 4;
	} else if (r_FSIcon.PtInRect(point)) {
		CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
		pFrame->m_fFullScreen ? str2 = ResStr(IDS_TOOLTIP_WINDOW) : str2 = ResStr(IDS_TOOLTIP_FULLSCREEN);
		if (str != str2) {
			m_tooltip.UpdateTipText(str2, this);
		}
		bt_idx = 5;
	} else if (r_LockIcon.PtInRect(point)) {
		AppSettings& s = AfxGetAppSettings();
		s.fFlybarOnTop ? str2 = ResStr(IDS_TOOLTIP_UNLOCK) : str2 = ResStr(IDS_TOOLTIP_LOCK);
		if (str != str2) {
			m_tooltip.UpdateTipText(str2, this);
		}
		bt_idx = 6;
	} else {
		if (str != _T("")) {
			m_tooltip.UpdateTipText(_T(""), this);
		}
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
	int iCursorHeight = 24;
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
		int x = rcBar.Width();

		CMainFrame* pFrame	= (CMainFrame*)GetParentFrame();
		WINDOWPLACEMENT wp;
		pFrame->GetWindowPlacement(&wp);

		OAFilterState fs	= pFrame->GetMediaState();
		CDC mdc;
		mdc.CreateCompatibleDC(&dc);
		CBitmap bm;
		bm.CreateCompatibleBitmap(&dc, x, rcBar.Height());
		CBitmap* pOldBm = mdc.SelectObject(&bm);
		mdc.SetBkMode(TRANSPARENT);

		int sep[][2] = {{0,1},{13,14},{15,16},{12,12},{10,11},{17,18},{5,6},{7,7},{21,22},{4,4},{2,3},{8,9},{19,20}};

		for (int i = 0; i < 2; i++) {

			if (!i || bt_idx == 0) {
				DrawBitmap(&mdc, x, sep[0][i], 1);
			}

			if (!i || bt_idx == 1) {
				DrawBitmap(&mdc, x, sep[1][i], 3);
			}

			if (!i || bt_idx == 2) {
				if (wp.showCmd == SW_SHOWMAXIMIZED) {
					DrawBitmap(&mdc, x, sep[2][i], 2);
				} else if (pFrame->m_fFullScreen) {
					DrawBitmap(&mdc, x, sep[3][i], 2);
				} else {
					DrawBitmap(&mdc, x, sep[4][i], 2);
				}
			}

			if (!i || bt_idx == 3) {
				DrawBitmap(&mdc, x, sep[5][i], 7);
			}

			if (!i || bt_idx == 4) {
				if (fs != -1) {
					DrawBitmap(&mdc, x, sep[6][i], 6);
				} else {
					DrawBitmap(&mdc, x, sep[7][i], 6);
				}
			}

			if (!i || bt_idx == 5) {
				if (pFrame->m_fFullScreen) {
					DrawBitmap(&mdc, x, sep[8][i], 4);
				} else if (wp.showCmd == SW_SHOWMAXIMIZED || (s.IsD3DFullscreen() && fs != -1)) {
					DrawBitmap(&mdc, x, sep[9][i], 4);
				} else {
					DrawBitmap(&mdc, x, sep[10][i], 4);
				}
			}

			if (!i || bt_idx == 6) {
				if (s.fFlybarOnTop) {
					DrawBitmap(&mdc, x, sep[11][i], 9);
				} else {
					DrawBitmap(&mdc, x, sep[12][i], 9);
				}
			}

		}

		dc.BitBlt(0, 0, x, rcBar.Height(), &mdc, 0, 0, SRCCOPY);

		mdc.SelectObject(pOldBm);
		bm.DeleteObject();
		mdc.DeleteDC();
	}
	bt_idx = -1;
}
