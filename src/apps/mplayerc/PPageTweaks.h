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

#pragma once

#include "PPageBase.h"
#include "StaticLink.h"


// CPPageTweaks dialog

class CPPageTweaks : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageTweaks)

public:
	CPPageTweaks();
	virtual ~CPPageTweaks();

	BOOL m_fDisableXPToolbars;
	CButton m_fDisableXPToolbarsCtrl;

	enum { IDD = IDD_PPAGETWEAKS };
	int m_nThemeBrightness;
	int m_nThemeBrightness_Old;
	int m_nThemeRed;
	int m_nThemeGreen;
	int m_nThemeBlue;
	int m_nThemeRed_Old;
	int m_nThemeGreen_Old;
	int m_nThemeBlue_Old;
	BOOL m_fFileNameOnSeekBar;
	CSliderCtrl m_ThemeBrightnessCtrl;
	CSliderCtrl m_ThemeRedCtrl;
	CSliderCtrl m_ThemeGreenCtrl;
	CSliderCtrl m_ThemeBlueCtrl;
	int m_clrFaceABGR;
	int m_clrOutlineABGR;
	int m_nJumpDistS;
	int m_nJumpDistM;
	int m_nJumpDistL;
	BOOL m_fNotifyMSN;

	BOOL m_fPreventMinimize;
	BOOL m_fUseWin7TaskBar;
	BOOL m_fDontUseSearchInFolder;
	BOOL m_fUseTimeTooltip;
	CComboBox m_TimeTooltipPosition;
	CComboBox m_FontSize;
	CComboBox m_FontType;
	int m_OSD_Size;
	CString	m_OSD_Font;

	BOOL m_fFastSeek;
	BOOL m_fLCDSupport;
	BOOL m_fSmartSeek;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	void OnCancel();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMH, LRESULT* pResult);
	afx_msg void OnUpdateCheck3(CCmdUI* pCmdUI);
	afx_msg void OnClickClrDefault();
	afx_msg void OnClickClrFace();
	afx_msg void OnClickClrOutline();
	afx_msg void OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUseTimeTooltipClicked();
	afx_msg void OnChngOSDCombo();
	afx_msg void OnUpdateThemeBrightness(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeRed(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeGreen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeBlue(CCmdUI* pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnThemeChange();
};
