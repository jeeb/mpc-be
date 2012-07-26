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
#include "PlayerSeekBar.h"
#include "MainFrm.h"

// CPlayerSeekBar

IMPLEMENT_DYNAMIC(CPlayerSeekBar, CDialogBar)

CPlayerSeekBar::CPlayerSeekBar() :
	m_start(0), m_stop(100), m_pos(0), m_posreal(0),
	m_fEnabled(false),
	m_tooltipState(TOOLTIP_HIDDEN), m_tooltipLastPos(-1), m_tooltipTimer(1)
{
}

CPlayerSeekBar::~CPlayerSeekBar()
{
}

BOOL CPlayerSeekBar::Create(CWnd* pParentWnd)
{
	VERIFY(CDialogBar::Create(pParentWnd, IDD_PLAYERSEEKBAR, WS_CHILD|WS_VISIBLE|CBRS_ALIGN_BOTTOM, IDD_PLAYERSEEKBAR));

	ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

	m_tooltip.Create(this, TTS_NOPREFIX | TTS_ALWAYSTIP);

	m_tooltip.SetMaxTipWidth(SHRT_MAX);

	m_tooltip.SetDelayTime(TTDT_AUTOPOP, SHRT_MAX);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
	m_tooltip.SetDelayTime(TTDT_RESHOW, 0);

	memset(&m_ti, 0, sizeof(TOOLINFO));
	m_ti.cbSize = sizeof(TOOLINFO);
	m_ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	m_ti.hwnd = m_hWnd;
	m_ti.hinst = AfxGetInstanceHandle();
	m_ti.lpszText = NULL;
	m_ti.uId = (UINT)m_hWnd;

	m_tooltip.SendMessage(TTM_ADDTOOL, 0, (LPARAM)&m_ti);

	AppSettings& s = AfxGetAppSettings();

	return TRUE;
}

BOOL CPlayerSeekBar::PreCreateWindow(CREATESTRUCT& cs)
{
	VERIFY(CDialogBar::PreCreateWindow(cs));

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;
	m_dwStyle |= CBRS_SIZE_FIXED;

	return TRUE;
}

void CPlayerSeekBar::Enable(bool fEnable)
{
	m_fEnabled = fEnable;
	Invalidate();
}

void CPlayerSeekBar::GetRange(__int64& start, __int64& stop)
{
	start = m_start;
	stop = m_stop;
}

void CPlayerSeekBar::SetRange(__int64 start, __int64 stop)
{
	if (start > stop) {
		start ^= stop, stop ^= start, start ^= stop;
	}
	m_start = start;
	m_stop = stop;
	if (m_pos < m_start || m_pos >= m_stop) {
		SetPos(m_start);
	}
}

__int64 CPlayerSeekBar::GetPos()
{
	return(m_pos);
}

__int64 CPlayerSeekBar::GetPosReal()
{
	return(m_posreal);
}

void CPlayerSeekBar::SetPos(__int64 pos)
{
	CWnd* w = GetCapture();
	if (w && w->m_hWnd == m_hWnd) {
		return;
	}

	SetPosInternal(pos);
}

void CPlayerSeekBar::SetPosInternal(__int64 pos)
{
	AppSettings& s = AfxGetAppSettings();

	if (s.fDisableXPToolbars) {
		if (m_pos == pos || m_stop <= pos) return;
	} else {
		if (m_pos == pos) {
			return;
		}
	}

	CRect before = GetThumbRect();
	m_pos = min(max(pos, m_start), m_stop);
	m_posreal = pos;
	CRect after = GetThumbRect();

	if (before != after) {
		if (!s.fDisableXPToolbars) {
			InvalidateRect (before | after);
		}

		CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
		if (pFrame && (s.fUseWin7TaskBar && pFrame->m_pTaskbarList)) {
			pFrame->m_pTaskbarList->SetProgressValue (pFrame->m_hWnd, pos, m_stop);
		}
	}
}

void CPlayerSeekBar::SetPosInternal2(__int64 pos2)
{
	AppSettings& s = AfxGetAppSettings();

	if (s.fDisableXPToolbars) {
		if (m_pos2 == pos2 || m_stop <= pos2) return;
	} else {
		if (m_pos2 == pos2) {
			return;
		}
	}

	CRect before = GetThumbRect();
	m_pos2 = min(max(pos2, m_start), m_stop);
	m_posreal2 = pos2;
	CRect after = GetThumbRect();

	if (before != after) {
		if (!s.fDisableXPToolbars) {
			InvalidateRect (before | after);
		}
	}
}

CRect CPlayerSeekBar::GetChannelRect()
{
	CRect r;
	GetClientRect(&r);
	
	if (AfxGetAppSettings().fDisableXPToolbars) {
		//r.DeflateRect(1,1,1,1); 
	} else {
		r.DeflateRect(8, 9, 9, 0);
		r.bottom = r.top + 5;
	}

	return(r);
}

CRect CPlayerSeekBar::GetThumbRect()
{
	CRect r = GetChannelRect();

	int x = r.left + (int)((m_start < m_stop) ? (__int64)r.Width() * (m_pos - m_start) / (m_stop - m_start) : 0);
	int y = r.CenterPoint().y;

	if (AfxGetAppSettings().fDisableXPToolbars) {
		 r.SetRect(x, y-2, x+3, y+3);
	} else {
		r.SetRect(x, y, x, y);
		r.InflateRect(6, 7, 7, 8);
	}

	return(r);
}

CRect CPlayerSeekBar::GetInnerThumbRect()
{
	CRect r = GetThumbRect();

	bool fEnabled = m_fEnabled && m_start < m_stop;
	r.DeflateRect(3, fEnabled ? 5 : 4, 3, fEnabled ? 5 : 4);

	return(r);
}

__int64 CPlayerSeekBar::CalculatePosition(CPoint point)
{
	__int64 pos = -1;
	CRect r = GetChannelRect();

	if (r.left >= r.right) {
		pos = -1;
	} else if (point.x < r.left) {
		pos = m_start;
	} else if (point.x >= r.right) {
		pos = m_stop;
	} else {
		__int64 w = r.right - r.left;
		if (m_start < m_stop) {
			pos = m_start + ((m_stop - m_start) * (point.x - r.left) + (w/2)) / w;
		}
	}

	return pos;
}

__int64 CPlayerSeekBar::CalculatePosition2(CPoint point)
{
	__int64 pos2 = -1;
	CRect r = GetChannelRect();

	if (r.left >= r.right) {
		pos2 = -1;
	} else if (point.x < r.left) {
		pos2 = m_start;
	} else if (point.x >= r.right) {
		pos2 = m_stop;
	} else {
		__int64 w = r.right - r.left;
		if (m_start < m_stop) {
			pos2 = m_start + ((m_stop - m_start) * (point.x - r.left) + (w/2)) / w;
		}
	}

	return pos2;
}


void CPlayerSeekBar::MoveThumb(CPoint point)
{
	__int64 pos = CalculatePosition(point);

	if (pos >= 0) {
		SetPosInternal(pos);
	}
}

void CPlayerSeekBar::MoveThumb2(CPoint point)
{
	__int64 pos2 = CalculatePosition2(point);

	if (pos2 >= 0) {
		SetPosInternal2(pos2);
	}
}

BEGIN_MESSAGE_MAP(CPlayerSeekBar, CDialogBar)
	//{{AFX_MSG_MAP(CPlayerSeekBar)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_COMMAND_EX(ID_PLAY_STOP, OnPlayStop)
END_MESSAGE_MAP()

// CPlayerSeekBar message handlers

void CPlayerSeekBar::OnPaint()
{
	CPaintDC dc(this);

	AppSettings& s = AfxGetAppSettings();

	int R, G, B, R2, G2, B2;

	bool fEnabled = m_fEnabled && m_start < m_stop;

	if (s.fDisableXPToolbars) {
		CRect rt;
		CString str = ((CMainFrame*)AfxGetMyApp()->GetMainWnd())->m_strFn;

		CDC memdc;
		CBitmap m_bmPaint;
		CRect r,rf,rc;
		GetClientRect(&r);
		memdc.CreateCompatibleDC(&dc);
		m_bmPaint.CreateCompatibleBitmap(&dc, r.Width(), r.Height());
		CBitmap *bmOld = memdc.SelectObject(&m_bmPaint);

		bFileNameOnSeekBar = s.fFileNameOnSeekBar;

		GRADIENT_RECT gr[1] = {{0, 1}};
		int pa = 255 * 256;

		int fp = m_logobm.FileExists("background");

		if (NULL != fp) {
			ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
			m_logobm.LoadExternalGradient("background", &memdc, r, 0, s.nThemeBrightness, R, G, B);
		} else {
			ThemeRGB(0, 5, 10, R, G, B);
			ThemeRGB(15, 20, 25, R2, G2, B2);
			TRIVERTEX tv[2] = {
				{r.left, r.top, R*256, G*256, B*256, pa},
				{r.right, r.bottom, R2*256, G2*256, B2*256, pa},
			};
			memdc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
		}

		memdc.SetBkMode(TRANSPARENT);

		CPen penPlayed(s.clrFaceABGR == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, s.clrFaceABGR);
		CPen penPlayedOutline(s.clrOutlineABGR == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, s.clrOutlineABGR);
		
		rc = GetChannelRect();
		int nposx = GetThumbRect().right-2;
		int nposy = r.top;
		
		ThemeRGB(30, 35, 40, R, G, B);
		CPen penPlayed1(PS_SOLID,0,RGB(R,G,B));
		memdc.SelectObject(&penPlayed1);
		memdc.MoveTo(rc.left, rc.top);
		memdc.LineTo(rc.right, rc.top);

		ThemeRGB(80, 85, 90, R, G, B);
		CPen penPlayed2(PS_SOLID,0,RGB(R,G,B));
		memdc.SelectObject(&penPlayed2);
		memdc.MoveTo(rc.left -1, rc.top +19);
		memdc.LineTo(rc.right+2, rc.top +19);

		if (fEnabled) {

			if (NULL != fp) {
				rc.right = nposx;
				rc.left = rc.left + 1;
				rc.top = rc.top + 1;
				rc.bottom = rc.bottom - 2;

				ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
				m_logobm.LoadExternalGradient("background", &memdc, rc, 0, s.nThemeBrightness, R, G, B);

				rc = GetChannelRect();
			} else {
				ThemeRGB(0, 5, 10, R, G, B);
				ThemeRGB(105, 110, 115, R2, G2, B2);
				TRIVERTEX tv[2] = {
					{rc.left, rc.top, R*256, G*256, B*256, pa},
					{nposx, rc.bottom-3, R2*256, G2*256, B2*256, pa},
				};
				memdc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);

				CRect rc2;
				rc2.left = nposx - 5;
				rc2.right = nposx;
				rc2.top = rc.top;
				rc2.bottom = rc.bottom;

				ThemeRGB(0, 5, 10, R, G, B);
				ThemeRGB(205, 210, 215, R2, G2, B2);
				TRIVERTEX tv2[2] = {
					{rc2.left, rc2.top, R*256, G*256, B*256, pa},
					{rc2.right, rc.bottom-3, R2*256, G2*256, B2*256, pa},
				};
				memdc.GradientFill(tv2, 2, gr, 1, GRADIENT_FILL_RECT_V);
			}

			ThemeRGB(80, 85, 90, R, G, B);
			CPen penPlayed3(PS_SOLID,0,RGB(R,G,B));
			memdc.SelectObject(&penPlayed3);
			memdc.MoveTo(rc.left, rc.top);//active_top
			memdc.LineTo(nposx, rc.top);

			//CPen penPlayed4(PS_SOLID,0,RGB(iThemeBrightness+115,iThemeBrightness+120,iThemeBrightness+125));

		}

		if (bFileNameOnSeekBar) {
			CFont font2;
			ThemeRGB(135, 140, 145, R, G, B);
			memdc.SetTextColor(RGB(R,G,B));
			font2.CreateFont(
							13,				// nHeight
							0,				// nWidth
							0,				// nEscapement
							0,				// nOrientation
							FW_NORMAL,			// nWeight
							FALSE,				// bItalic
							FALSE,				// bUnderline
							0,				// cStrikeOut
							ANSI_CHARSET,			// nCharSet
							OUT_RASTER_PRECIS,		// nOutPrecision
							CLIP_DEFAULT_PRECIS,		// nClipPrecision
							ANTIALIASED_QUALITY,        	// nQuality
							VARIABLE_PITCH | FF_MODERN, 	// nPitchAndFamily
							_T("Tahoma")                	// lpszFacename
							);

			CFont* oldfont2 = memdc.SelectObject(&font2);
			SetBkMode(memdc, TRANSPARENT);
			rt = rc;
			rt.left = rc.left+6;
			rt.top = rc.top - 2;
			if (!AfxGetAppSettings().bStatusBarIsVisible) rt.right = rc.right - 150;
			memdc.DrawText(str, str.GetLength(), &rt, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

			ThemeRGB(205, 210, 215, R, G, B);
			memdc.SetTextColor(RGB(R,G,B));
			CRect rt2;
			rt2 = rc;
			rt2.left = rc.left+6;
			rt2.right = (nposx > rc.right - 150 && !AfxGetAppSettings().bStatusBarIsVisible ? rc.right - 150 : nposx);
			rt2.top = rc.top - 2;

			if (nposx > rt.right-15) {
				rt2.right = rt.right;
				memdc.DrawText(str, str.GetLength(), &rt2, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
			} else {
				rt2.right = nposx;
				memdc.DrawText(str, str.GetLength(), &rt2, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			}

			if (!AfxGetAppSettings().bStatusBarIsVisible) {
				CString strT = AfxGetAppSettings().strTimeOnSeekBar;
				CRect rT = rc;
				rT.left = rc.right - 140;
				rT.right = rc.right - 6;
				ThemeRGB(200, 205, 210, R, G, B);
				memdc.SetTextColor(RGB(R,G,B));
				memdc.DrawText(strT, strT.GetLength(), &rT, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
			}
		}

		dc.BitBlt(r.left, r.top, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
		DeleteObject(memdc.SelectObject(bmOld));
		memdc.DeleteDC();
		m_bmPaint.DeleteObject();
	} else {
		COLORREF
		white	= GetSysColor(COLOR_WINDOW),
		shadow	= GetSysColor(COLOR_3DSHADOW),
		light	= GetSysColor(COLOR_3DHILIGHT),
		bkg	= GetSysColor(COLOR_BTNFACE);

		// thumb
		{
			CRect r = GetThumbRect(), r2 = GetInnerThumbRect();
			CRect rt = r, rit = r2;

			dc.Draw3dRect(&r, light, 0);
			r.DeflateRect(0, 0, 1, 1);
			dc.Draw3dRect(&r, light, shadow);
			r.DeflateRect(1, 1, 1, 1);

			CBrush b(bkg);

			dc.FrameRect(&r, &b);
			r.DeflateRect(0, 1, 0, 1);
			dc.FrameRect(&r, &b);

			r.DeflateRect(1, 1, 0, 0);
			dc.Draw3dRect(&r, shadow, bkg);

			if (fEnabled) {
				r.DeflateRect(1, 1, 1, 2);
				CPen white(PS_INSIDEFRAME, 1, white);
				CPen* old = dc.SelectObject(&white);
				dc.MoveTo(r.left, r.top);
				dc.LineTo(r.right, r.top);
				dc.MoveTo(r.left, r.bottom);
				dc.LineTo(r.right, r.bottom);
				dc.SelectObject(old);
				dc.SetPixel(r.CenterPoint().x, r.top, 0);
				dc.SetPixel(r.CenterPoint().x, r.bottom, 0);
			}

			dc.SetPixel(r.CenterPoint().x+5, r.top-4, bkg);

			{
				CRgn rgn1, rgn2;
				rgn1.CreateRectRgnIndirect(&rt);
				rgn2.CreateRectRgnIndirect(&rit);
				ExtSelectClipRgn(dc, rgn1, RGN_DIFF);
				ExtSelectClipRgn(dc, rgn2, RGN_OR);
			}
		}

		// channel
		{
			CRect r = GetChannelRect();

			dc.FillSolidRect(&r, fEnabled ? white : bkg);
			r.InflateRect(1, 1);
			dc.Draw3dRect(&r, shadow, light);
			dc.ExcludeClipRect(&r);
		}

		// background
		{
			CRect r;
			GetClientRect(&r);
			CBrush b(bkg);
			dc.FillRect(&r, &b);
		}

	}
}

void CPlayerSeekBar::OnSize(UINT nType, int cx, int cy)
{
	HideToolTip();

	CDialogBar::OnSize(nType, cx, cy);

	Invalidate();
}

void CPlayerSeekBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (AfxGetAppSettings().fDisableXPToolbars && m_fEnabled) {
		SetCapture();
		MoveThumb(point);
	} else {
		if (m_fEnabled && (GetChannelRect() | GetThumbRect()).PtInRect(point)) {
			SetCapture();
			MoveThumb(point);
			GetParent()->PostMessage(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);
		} else {
			CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
			if (!pFrame->m_fFullScreen) {
				MapWindowPoints(pFrame, &point, 1);
				pFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
			}
		}
	}

	CDialogBar::OnLButtonDown(nFlags, point);
}

void CPlayerSeekBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	if (AfxGetAppSettings().fDisableXPToolbars && m_fEnabled) {
		GetParent()->PostMessage(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);
	}

	CDialogBar::OnLButtonUp(nFlags, point);
}

void CPlayerSeekBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (!AfxGetAppSettings().bStatusBarIsVisible) {

		CRect rc = GetChannelRect();
		CRect rT = rc;
		rT.left = rc.right - 140;
		rT.right = rc.right - 6;
		CPoint p;
		GetCursorPos(&p);
		ScreenToClient(&p);

		if (rT.PtInRect(p)) {
			AfxGetAppSettings().fRemainingTime = !AfxGetAppSettings().fRemainingTime;
		}
	}
	CDialogBar::OnRButtonDown(nFlags, point);
}


void CPlayerSeekBar::UpdateTooltip(CPoint point)
{
	m_tooltipPos = CalculatePosition(point);

	if (m_fEnabled && m_start < m_stop && (GetChannelRect() | GetThumbRect()).PtInRect(point)) {
		if (m_tooltipState == TOOLTIP_HIDDEN && m_tooltipPos != m_tooltipLastPos) {

			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
			tme.hwndTrack = m_hWnd;
			tme.dwFlags = TME_LEAVE;
			TrackMouseEvent(&tme);

			m_tooltipState = TOOLTIP_TRIGGERED;
			m_tooltipTimer = SetTimer(m_tooltipTimer, SHOW_DELAY, NULL);
		}
	} else {
		HideToolTip();
	}

	if (m_tooltipState == TOOLTIP_VISIBLE && m_tooltipPos != m_tooltipLastPos) {
		UpdateToolTipText();
		UpdateToolTipPosition(point);

		m_tooltipTimer = SetTimer(m_tooltipTimer, ((CMainFrame*)GetParentFrame())->CanPreviewUse() ? 10 : AUTOPOP_DELAY, NULL);
	}
}

void CPlayerSeekBar::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd* w = GetCapture();

	if (w && w->m_hWnd == m_hWnd && (nFlags & MK_LBUTTON)) {
		MoveThumb(point);
		if (!AfxGetAppSettings().fDisableXPToolbars) {
			GetParent()->PostMessage(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBTRACK), (LPARAM)m_hWnd);
		}
	}

	CMainFrame* pFrame	= ((CMainFrame*)GetParentFrame());

	if (AfxGetAppSettings().fUseTimeTooltip || pFrame->CanPreviewUse()) {
		UpdateTooltip(point);
	}

	OAFilterState fs	= pFrame->GetMediaState();
	if (fs != -1) {
		pt2 = point;
		//MoveThumb2(point);
		//pFrame->ChangeVideoWindow2(m_pos2);
		//pFrame->m_wndView2.Invalidate();
		//pFrame->m_wndView2.ShowWindow(SW_SHOW);
	} else {
		pFrame->PreviewWindowHide();
	}

	CDialogBar::OnMouseMove(nFlags, point);
}

LRESULT CPlayerSeekBar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_MOUSELEAVE) {
		HideToolTip();
		((CMainFrame*)GetParentFrame())->PreviewWindowHide();
	}

	return CWnd::WindowProc(message, wParam, lParam);
}

BOOL CPlayerSeekBar::PreTranslateMessage(MSG* pMsg)
{
	POINT ptWnd(pMsg->pt);
	this->ScreenToClient(&ptWnd);

	if (m_fEnabled && AfxGetAppSettings().fUseTimeTooltip && m_start < m_stop && (GetChannelRect() | GetThumbRect()).PtInRect(ptWnd)) {
		m_tooltip.RelayEvent(pMsg);
	}

	return CDialogBar::PreTranslateMessage(pMsg);
}

BOOL CPlayerSeekBar::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

BOOL CPlayerSeekBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_fEnabled && m_start < m_stop && m_stop != 100) {
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CPlayerSeekBar::OnPlayStop(UINT nID)
{
	SetPos(0);
	Invalidate();

	return FALSE;
}

void CPlayerSeekBar::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_tooltipTimer) {
		switch (m_tooltipState) {
			case TOOLTIP_TRIGGERED:
			{
				CPoint point;

				GetCursorPos(&point);
				ScreenToClient(&point);

				if (m_fEnabled && m_start < m_stop && (GetChannelRect() | GetThumbRect()).PtInRect(point)) {
					m_tooltipTimer = SetTimer(m_tooltipTimer, ((CMainFrame*)GetParentFrame())->CanPreviewUse() ? 10 : AUTOPOP_DELAY, NULL);
					m_tooltipPos = CalculatePosition(point);
					UpdateToolTipText();
					if (!((CMainFrame*)GetParentFrame())->CanPreviewUse()) {
						m_tooltip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
					}
					UpdateToolTipPosition(point);
					m_tooltipState = TOOLTIP_VISIBLE;
				}
			}
			break;
			case TOOLTIP_VISIBLE:
				HideToolTip();
				MoveThumb2(pt2);
				((CMainFrame*)GetParentFrame())->PreviewWindowShow(m_pos2);

				break;
		}

	}

	CWnd::OnTimer(nIDEvent);
}

void CPlayerSeekBar::HideToolTip()
{
	if (m_tooltipState > TOOLTIP_HIDDEN) {
		KillTimer(m_tooltipTimer);
		m_tooltip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
		m_tooltipState = TOOLTIP_HIDDEN;
	}
}

void CPlayerSeekBar::UpdateToolTipPosition(CPoint& point)
{
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	if (pFrame->CanPreviewUse()) {
		CRect Rect;
		pFrame->m_wndView2.GetWindowRect(Rect);
		int r_width		= Rect.Width();
		int r_height	= Rect.Height();

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		CPoint p1(point);
		point.x -= r_width / 2 - 2;
		point.y = GetChannelRect().TopLeft().y - (r_height + 13);
		ClientToScreen(&point);
		point.x = max(mi.rcWork.left + 5, min(point.x, mi.rcWork.right - r_width - 5));
		if (point.y <= 5) {
			p1.y = GetChannelRect().TopLeft().y + 30;
			ClientToScreen(&p1);

			point.y = p1.y;
		}
		pFrame->m_wndView2.MoveWindow(point.x, point.y, r_width, r_height);
	} else {
		CSize size = m_tooltip.GetBubbleSize(&m_ti);
		CRect r;
		GetWindowRect(r);

		if (AfxGetAppSettings().nTimeTooltipPosition == TIME_TOOLTIP_ABOVE_SEEKBAR) {
			point.x -= size.cx / 2 - 2;
			point.y = GetChannelRect().TopLeft().y - (size.cy + 13);
		} else {
			point.x += 10;
			point.y += 20;
		}
		point.x = max(0, min(point.x, r.Width() - size.cx));
		ClientToScreen(&point);

		m_tooltip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(point.x, point.y));
		m_tooltipLastPos = m_tooltipPos;
	}
}

void CPlayerSeekBar::UpdateToolTipText()
{
	DVD_HMSF_TIMECODE tcNow = RT2HMS_r(m_tooltipPos);

	/*
	if (tcNow.bHours > 0) {
		m_tooltipText.Format(_T("%02d:%02d:%02d"), tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
	} else {
		m_tooltipText.Format(_T("%02d:%02d"), tcNow.bMinutes, tcNow.bSeconds);
	}
	*/
	CString tooltipText;
	tooltipText.Format(_T("%02d:%02d:%02d"), tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	if (!pFrame->CanPreviewUse()) {
		m_ti.lpszText = (LPTSTR)(LPCTSTR)tooltipText;
		m_tooltip.SendMessage(TTM_SETTOOLINFO, 0, (LPARAM)&m_ti);
	} else {
		// TODO - Center Caption ...
		CString str = _T("               ") + tooltipText + _T("               ");
		pFrame->m_wndView2.SetWindowText(str);
	}
}
