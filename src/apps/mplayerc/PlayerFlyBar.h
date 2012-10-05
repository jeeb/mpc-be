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

#pragma once

class CFlyBar : public CWnd
{
public:
	CFlyBar();
	virtual ~CFlyBar();
	void SetDefault();
	void SetDefault2();
	int bt_idx;
	//int ih;
	int iw;
	void CalcButtonsRect();

	DECLARE_DYNAMIC(CFlyBar)
	
private:
	HICON m_ExitIcon_a;
	HICON m_ExitIcon;
	HICON m_MinIcon_a;
	HICON m_MinIcon;
	HICON m_MaxIcon_a;
	HICON m_MaxIcon;
	HICON m_RestoreIcon_a;
	HICON m_RestoreIcon;
	HICON m_SettingsIcon_a;
	HICON m_SettingsIcon;
	HICON m_InfoIcon_a;
	HICON m_InfoIcon;
	HICON m_FSIcon_a;
	HICON m_FSIcon;
	HICON m_WindowIcon_a;
	HICON m_WindowIcon;
	HICON m_LockIcon;
	HICON m_UnLockIcon;
	HICON m_LockIcon_a;
	HICON m_UnLockIcon_a;

	CRect r_ExitIcon;
	CRect r_MinIcon;
	CRect r_RestoreIcon;
	CRect r_SettingsIcon;
	CRect r_InfoIcon;
	CRect r_FSIcon;
	CRect r_LockIcon;
	
	CToolTipCtrl m_tooltip;

	void Destroy();

protected:
	
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};
