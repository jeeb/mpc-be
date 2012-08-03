/*
 * $Id$
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 *
 * This file is part of MPC-BE.
 * YOU CANNOT USE THIS FILE WITHOUT AUTHOR PERMISSION!
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
#include "PreView.h"
#include "PngImage.h"
#include "MainFrm.h"

/////////////////////////////////////////////////////////////////////////////
// CPrevView

CPreView::CPreView() : tooltipstr(_T(""))
{
}

CPreView::~CPreView()
{
}

BOOL CPreView::SetWindowText(LPCWSTR lpString)
{
	tooltipstr = lpString;
	Invalidate();
	return ::SetWindowText(m_hWnd, lpString);
}

void CPreView::GetVideoRect(LPRECT lpRect)
{
	CRect r;
	GetClientRect(&r);

	if (AfxGetAppSettings().fDisableXPToolbars) {
		r.left		= 5;
		r.top		= 20;
		r.right		-= 5;
		r.bottom	-= 5;
	}
	m_view.MoveWindow(r);

	m_view.GetClientRect(lpRect);	
}

HWND CPreView::GetVideoHWND()
{
	return m_view.GetSafeHwnd();
}

IMPLEMENT_DYNAMIC(CPreView, CWnd)

BEGIN_MESSAGE_MAP(CPreView, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreView message handlers

BOOL CPreView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}
	cs.style &= ~WS_BORDER;

	return TRUE;
}

int CPreView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	CRect r;
	GetClientRect(r);

	if (!m_view.Create(_T(""), WS_CHILD|WS_VISIBLE, r, this)) {
		return -1;
	}

	return 0;
}

void CPreView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	AppSettings& s = AfxGetAppSettings();
	if (s.fDisableXPToolbars) {
		CDC memdc;
		CBitmap m_bmPaint;
		CRect r,rf,rc;
		GetClientRect(&r);
		memdc.CreateCompatibleDC(&dc);
		m_bmPaint.CreateCompatibleBitmap(&dc, r.Width(), r.Height());
		CBitmap *bmOld = memdc.SelectObject(&m_bmPaint);
		memdc.SetBkMode(TRANSPARENT);

		GRADIENT_RECT gr[1] = {{0, 1}};
				int pa = 255 * 256;

		CRect rtTop, rtLeft, rtRight, rtBottom;
		rtTop = rtLeft = rtRight = rtBottom = r;
		int R, G, B, R2, G2, B2;
		rtTop.bottom = rtTop.top + 20;
		ThemeRGB(125, 130, 135, R, G, B);
		ThemeRGB(95, 100, 105, R2, G2, B2);
		TRIVERTEX tv[2] = {
					{rtTop.left, rtTop.top, R*256, G*256, B*256, pa},
					{rtTop.right, rtTop.bottom, R2*256, G2*256, B2*256, pa},
				};
		memdc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);

		ThemeRGB(165, 170, 175, R, G, B);
		CPen penPlayed2(PS_SOLID,0,RGB(R,G,B));
		memdc.SelectObject(&penPlayed2);
		memdc.MoveTo(rtTop.left, rtTop.top);
		memdc.LineTo(rtTop.right, rtTop.top);



		rtLeft.right = rtLeft.left + 5;
		rtLeft.top = rtLeft.top + 19;
		ThemeRGB(95, 100, 105, R, G, B);
		ThemeRGB(35, 40, 45, R2, G2, B2);
		TRIVERTEX tv2[2] = {
					{rtLeft.left, rtLeft.top, R*256, G*256, B*256, pa},
					{rtLeft.right, rtLeft.bottom, R2*256, G2*256, B2*256, pa},
				};
		memdc.GradientFill(tv2, 2, gr, 1, GRADIENT_FILL_RECT_V);
		ThemeRGB(105, 110, 115, R, G, B);
		CPen penPlayed4(PS_SOLID,0,RGB(R,G,B));
		memdc.SelectObject(&penPlayed4);
		memdc.MoveTo(rtLeft.left, rtTop.top);
		memdc.LineTo(rtLeft.left, r.bottom);


		rtRight.left = rtRight.right - 5;
		rtRight.top = rtRight.top + 19;
		ThemeRGB(95, 100, 105, R, G, B);
		ThemeRGB(35, 40, 45, R2, G2, B2);
		TRIVERTEX tv3[2] = {
					{rtRight.left, rtRight.top, R*256, G*256, B*256, pa},
					{rtRight.right, rtRight.bottom, R2*256, G2*256, B2*256, pa},
				};
		memdc.GradientFill(tv3, 2, gr, 1, GRADIENT_FILL_RECT_V);
		
		memdc.SelectObject(&penPlayed4);
		memdc.MoveTo(rtRight.left, rtRight.top+1);
		memdc.LineTo(rtRight.left, rtRight.bottom);


		rtBottom.top = rtBottom.bottom - 5;
		ThemeRGB(35, 40, 45, R, G, B);
		ThemeRGB(15, 20, 25, R2, G2, B2);
		TRIVERTEX tv4[2] = {
					{rtBottom.left, rtBottom.top, R*256, G*256, B*256, pa},
					{rtBottom.right, rtBottom.bottom, R2*256, G2*256, B2*256, pa},
				};
		memdc.GradientFill(tv4, 2, gr, 1, GRADIENT_FILL_RECT_V);
		ThemeRGB(65, 70, 75, R, G, B);
		CPen penPlayed3(PS_SOLID,0,RGB(R,G,B));
		memdc.SelectObject(&penPlayed3);
		memdc.MoveTo(rtBottom.left+4, rtBottom.top);
		memdc.LineTo(rtBottom.right-4, rtBottom.top);
		memdc.SelectObject(&penPlayed4);
		memdc.MoveTo(rtBottom.left, rtBottom.top);
		memdc.LineTo(rtBottom.left, rtBottom.bottom-1);


		CFont font2;
		ThemeRGB(255, 255, 255, R, G, B);
		memdc.SetTextColor(RGB(R,G,B));
		font2.CreateFont(
						13,				// nHeight
						0,				// nWidth
						0,				// nEscapement
						0,				// nOrientation
						FW_BOLD,			// nWeight
						FALSE,				// bItalic
						FALSE,				// bUnderline
						0,				// cStrikeOut
						ANSI_CHARSET,			// nCharSet
						OUT_RASTER_PRECIS,		// nOutPrecision
						CLIP_DEFAULT_PRECIS,		// nClipPrecision
						ANTIALIASED_QUALITY,        	// nQuality
						VARIABLE_PITCH | FF_MODERN, 	// nPitchAndFamily
						_T("Tahoma")              	// lpszFacename
						);

		memdc.SelectObject(&font2);
		CRect rtime = r;
		rtime.top = r.top +4;
		rtime.left = r.left;
		memdc.DrawText(tooltipstr, tooltipstr.GetLength(), &rtime, DT_CENTER|DT_TOP);

		dc.BitBlt(r.left, r.top, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
		DeleteObject(memdc.SelectObject(bmOld));
		memdc.DeleteDC();
		m_bmPaint.DeleteObject();
	}
}
