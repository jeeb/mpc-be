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
#include "VolumeCtrl.h"


// CVolumeCtrl

IMPLEMENT_DYNAMIC(CVolumeCtrl, CSliderCtrl)
CVolumeCtrl::CVolumeCtrl(bool fSelfDrawn) : m_fSelfDrawn(fSelfDrawn)
{
}

CVolumeCtrl::~CVolumeCtrl()
{
	if (m_bmUnderCtrl.GetSafeHandle() != NULL) {
		m_bmUnderCtrl.DeleteObject();
	}
}

bool CVolumeCtrl::Create(CWnd* pParentWnd)
{
	if (!CSliderCtrl::Create(WS_CHILD|WS_VISIBLE|TBS_NOTICKS|TBS_HORZ|TBS_TOOLTIPS, CRect(0,0,0,0), pParentWnd, IDC_SLIDER1)) {
		return false;
	}

	SetRange(0, 100);
	SetPosInternal(AfxGetAppSettings().nVolume);
	SetPageSize(5);
	SetLineSize(0);

	iThemeBrightness = AfxGetAppSettings().nThemeBrightness;

	Invalidate();

	return true;
}

void CVolumeCtrl::SetPosInternal(int pos)
{
	SetPos(pos);
	GetParent()->PostMessage(WM_HSCROLL, MAKEWPARAM((short)pos, SB_THUMBPOSITION), (LPARAM)m_hWnd); // this will be reflected back on us
}

void CVolumeCtrl::IncreaseVolume()
{
	SetPosInternal(GetPos() + GetPageSize());
}

void CVolumeCtrl::DecreaseVolume()
{
	SetPosInternal(GetPos() - GetPageSize());
}

BEGIN_MESSAGE_MAP(CVolumeCtrl, CSliderCtrl)
	ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_HSCROLL_REFLECT()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

// CVolumeCtrl message handlers

BOOL CVolumeCtrl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CVolumeCtrl::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

	LRESULT lr = CDRF_DODEFAULT;

	AppSettings& s = AfxGetAppSettings();

	if (m_fSelfDrawn) {
		switch (pNMCD->dwDrawStage) {
			case CDDS_PREPAINT:
			//TRACE(" PREPAINT ");
				if (s.fDisableXPToolbars/* && (m_bmUnderCtrl.GetSafeHandle() == NULL || iThemeBrightness != AfxGetAppSettings().nThemeBrightness)*/) {			
					CDC *dc = GetParent()->GetDC();					
					CDC memdc;
					memdc.CreateCompatibleDC(dc);
					CRect wr;
					GetWindowRect(&wr);
					GetParent()->ScreenToClient(&wr);

					if (m_bmUnderCtrl.GetSafeHandle() == NULL) {
						m_bmUnderCtrl.CreateCompatibleBitmap(dc, wr.Width(), wr.Height());
					}
					CBitmap *bmOld = memdc.SelectObject(&m_bmUnderCtrl);
					memdc.BitBlt(0, 0, wr.Width(), wr.Height(), dc, wr.left, wr.top, SRCCOPY);
					DeleteObject(memdc.SelectObject(bmOld));
					GetParent()->ReleaseDC(dc);
					memdc.DeleteDC();
				}
				lr = CDRF_NOTIFYITEMDRAW;
				break;

			case CDDS_ITEMPREPAINT:
				if (s.fDisableXPToolbars && m_bmUnderCtrl.GetSafeHandle() != NULL) {
					CDC dc;
					dc.Attach(pNMCD->hdc);
					CRect r;
					GetClientRect(&r);
					CDC memdc;
					memdc.CreateCompatibleDC(&dc);
					CBitmap *bmOld = memdc.SelectObject(&m_bmUnderCtrl);

					iThemeBrightness = AfxGetAppSettings().nThemeBrightness;
					iThemeRed = AfxGetAppSettings().nThemeRed;
					iThemeGreen = AfxGetAppSettings().nThemeGreen;
					iThemeBlue = AfxGetAppSettings().nThemeBlue;

					GRADIENT_RECT gr[1] = {{0, 1}};
					int pa = 255 * 256;
					unsigned p1 = s.clrOutlineABGR, p2 = s.clrFaceABGR;

					int fp = m_logobm.FileExists("volume");

					if (NULL != fp) {
						m_logobm.LoadExternalGradient("volume", &dc, r, 0, iThemeBrightness, iThemeRed, iThemeGreen, iThemeBlue);
					} else {
						int ir1 = p1 * iThemeRed;
						int ig1 = (p1 >> 8) * iThemeGreen;
						int ib1 = (p1 >> 16) * iThemeBlue;
						int ir2 = p2 * iThemeRed;
						int ig2 = (p2 >> 8) * iThemeGreen;
						int ib2 = (p2 >> 16) * iThemeBlue;

						TRIVERTEX tv[2] = {
							{r.left, r.top, ir1, ig1, ib1, pa},
							{r.right, 1, ir2, ig2, ib2, pa},
						};
						dc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_H);
					}

					int nVolume = GetPos();
					if (nVolume <= GetPageSize()) nVolume = 0;
					int m_nVolPos = r.left + nVolume * 0.5;

					unsigned p3 = dc.GetPixel(m_nVolPos, 0) == 0x00000000 ? dc.GetPixel(m_nVolPos - 5, 0) : dc.GetPixel(m_nVolPos + 10, 0);
					CPen penLeft(p2 == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, p3);

					dc.BitBlt(0, 0, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
					//MemDC.SelectObject(bmOld);
					DeleteObject(memdc.SelectObject(bmOld));
					memdc.DeleteDC();
					r.DeflateRect(4, 2, 9, 6);
					CopyRect(&pNMCD->rc, &r);
					//TRACE("VOLUME RECT=(%d,%d,%d,%d) state=%d",r.left,r.top,r.right,r.bottom,pNMCD->uItemState);

					CPen penRight(p1 == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, p1);
					CPen *penOld = dc.SelectObject(&penRight);

					int nposx,nposy;
					for (int i = 5 ; i <= 50;)
					{
						nposy = (int)r.bottom - r.Height() *i / ( r.Width() +10 ) -1;
						nposx = r.left -5 +i -1;
						i <= m_nVolPos ? dc.SelectObject(penLeft) : dc.SelectObject(penRight);
						dc.MoveTo(nposx, nposy);				//top_left
						dc.LineTo(nposx +2, nposy);				//top_right
						dc.LineTo(nposx +2, r.bottom);			//bottom_right
						dc.LineTo(nposx, r.bottom);				//bottom_left
						dc.LineTo(nposx, nposy);				//top_left
						if (!s.fMute)
						{
							dc.MoveTo(nposx +1, nposy -1);		//top_middle
							dc.LineTo(nposx +1, r.bottom +2);	//bottom_middle
						}
						i+=5;
					}
					dc.SelectObject(penOld);
					dc.Detach();
					lr = CDRF_SKIPDEFAULT;
				}

				if (!s.fDisableXPToolbars && pNMCD->dwItemSpec == TBCD_CHANNEL) {
					if (m_bmUnderCtrl.GetSafeHandle() != NULL) m_bmUnderCtrl.DeleteObject();
					CDC dc;
					dc.Attach(pNMCD->hdc);

					CRect r;
					GetClientRect(r);
					r.DeflateRect(8, 4, 10, 6);
					CopyRect(&pNMCD->rc, &r);
					CPen shadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
					CPen light(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
					CPen* old = dc.SelectObject(&light);
					dc.MoveTo(pNMCD->rc.right, pNMCD->rc.top);
					dc.LineTo(pNMCD->rc.right, pNMCD->rc.bottom);
					dc.LineTo(pNMCD->rc.left, pNMCD->rc.bottom);
					dc.SelectObject(&shadow);
					dc.LineTo(pNMCD->rc.right, pNMCD->rc.top);
					dc.SelectObject(old);

					dc.Detach();
					lr = CDRF_SKIPDEFAULT;
				} else if (!s.fDisableXPToolbars && pNMCD->dwItemSpec == TBCD_THUMB) {
					CDC dc;
					dc.Attach(pNMCD->hdc);
					pNMCD->rc.bottom--;
					CRect r(pNMCD->rc);
					r.DeflateRect(0, 0, 1, 0);

					COLORREF shadow = GetSysColor(COLOR_3DSHADOW);
					COLORREF light = GetSysColor(COLOR_3DHILIGHT);
					dc.Draw3dRect(&r, light, 0);
					r.DeflateRect(0, 0, 1, 1);
					dc.Draw3dRect(&r, light, shadow);
					r.DeflateRect(1, 1, 1, 1);
					dc.FillSolidRect(&r, GetSysColor(COLOR_BTNFACE));
					dc.SetPixel(r.left+7, r.top-1, GetSysColor(COLOR_BTNFACE));

					dc.Detach();
					lr = CDRF_SKIPDEFAULT;
				}
				break;
			default:
				break;
		}
	}

	pNMCD->uItemState &= ~CDIS_FOCUS;

	*pResult = lr;
}

void CVolumeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect r;
	GetChannelRect(&r);

	if (r.left >= r.right) {
		return;
	}

	int start, stop;
	GetRange(start, stop);

	r.left += 3;
	r.right -= 4;

	if (point.x < r.left) {
		SetPos(start);
	} else if (point.x >= r.right) {
		SetPos(stop);
	} else {
		int w = r.right - r.left - 6;
		if (start < stop) {
			SetPosInternal(start + ((stop - start) * (point.x - r.left) + (w/2)) / w);
		}
	}

	CSliderCtrl::OnLButtonDown(nFlags, point);
}

void CVolumeCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CSliderCtrl::OnSetFocus(pOldWnd);

	AfxGetMainWnd()->SetFocus(); // don't focus on us, parents will take care of our positioning
}

void CVolumeCtrl::HScroll(UINT nSBCode, UINT nPos)
{
	int nVolMin, nVolMax;
	GetRange(nVolMin, nVolMax);
	if ((UINT)nVolMin <= nSBCode && nSBCode <= (UINT)nVolMax) {
		CRect r;
		GetClientRect(&r);
		InvalidateRect(&r);
		UpdateWindow();

		AfxGetAppSettings().nVolume = GetPos();

		CFrameWnd* pFrame = GetParentFrame();
		if (pFrame && pFrame != GetParent()) {
			pFrame->PostMessage(WM_HSCROLL, MAKEWPARAM((short)nPos, nSBCode), (LPARAM)m_hWnd);
		}
	}
}

BOOL CVolumeCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
	return TRUE;
}
