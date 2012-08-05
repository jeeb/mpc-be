/*
 * $Id: PreView.h 807 2012-08-03 02:32:56Z Aleksoid $
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

#pragma once

class CPreView : public CWnd
{
public:
	CPreView();
	virtual ~CPreView();

	DECLARE_DYNAMIC(CPreView)

	virtual BOOL SetWindowText(LPCWSTR lpString);

	void GetVideoRect(LPRECT lpRect);
	HWND GetVideoHWND();

protected:
	CString tooltipstr;
	CWnd	m_view;
	int wb;
	int hc;
	CRect v_rect;
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};
