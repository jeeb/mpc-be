/*
 * $Id$
 *
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

#pragma once

#include <afxwin.h>

#define EDIT_BUTTON_LEFTCLICKED		(WM_APP + 842)


// CEditWithButton_Base

// This is the base class from which others derive to implement specific kinds of buttons.
class CEditWithButton_Base : public CEdit
{
public:
	CEditWithButton_Base();

	// Called when a left click is detected. The default implementation will send a
	// EDIT_BUTTON_LEFTCLICKED message to the parent window. Derived classes
	// could override this to handle the event themselves, and so are not required
	// to call this base implementation.
	virtual void OnLeftClick();

protected:
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcMouseLeave();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	virtual void PreSubclassWindow();
	afx_msg LRESULT OnRecalcNcSize(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg LRESULT OnSetReadOnly(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

	// Given the rectangle of the control window, this returns the rect that the button will occupy.
	CRect GetButtonRect(const CRect& rectWindow) const;

	// Translates the current button state in a value that could be used by DrawThemeBackground.
	int GetButtonThemeState() const;

	//	Called when the button needs to be drawn.
	//	The default implementation draws the button frame, adjusts the rect if
	//	the button is pressed, and then calls DrawButtonContent to render the
	//	contents of the button.
	virtual void DrawButton(CRect rectButton);

	//	Called when the button contents (anything inside the button's border) needs to
	//	be drawn. The rect will have already been adjusted if the button is pressed.
	//	The content must be drawn using the given device context within the given rect.
	virtual void DrawButtonContent(CDC& dc, CRect rectButton, HTHEME hButtonTheme) = 0;

	//	Called when the button's width needs to be (re)calculated.
	//	This must return the width in pixels.
	virtual int CalculateButtonWidth() = 0;

private:

	int m_TopBorder;
	int m_BottomBorder;
	int m_LeftBorder;
	int m_RightBorder;

	int m_ButtonWidth;

	bool m_IsButtonPressed;
	bool m_IsMouseActive;

	bool m_IsButtonHot;
};


// CEditWithButton

// This implements a button containing plain text.
class CEditWithButton : public CEditWithButton_Base
{
public:
	// The given text will be displayed in the button.
	explicit CEditWithButton(LPCTSTR pszButtonText = _T("..."));

	// Gets/Sets the button text.
	CString GetButtonText() const;
	void SetButtonText(LPCTSTR buttonText);

private:

	virtual void DrawButtonContent(CDC& dc, CRect rectButton, HTHEME hButtonTheme);

	virtual int CalculateButtonWidth();

	CString m_ButtonText;
};
