/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "FloatEdit.h"
#include "../../filters/switcher/AudioSwitcher/AudioSwitcher.h"


// CPPageAudioSwitcher dialog

class CPPageAudioSwitcher : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAudioSwitcher)

private:
	CComQIPtr<IAudioSwitcherFilter> m_pASF;

	CStringArray m_AudioRendererDisplayNames;

public:
	CPPageAudioSwitcher(IFilterGraph* pFG);
	virtual ~CPPageAudioSwitcher();

	enum { IDD = IDD_PPAGEAUDIO };

	int			m_iAudioRendererType;
	CComboBox	m_iAudioRendererTypeCtrl;
	int			m_iSecAudioRendererType;
	CComboBox	m_iSecAudioRendererTypeCtrl;
	CButton		m_audRendPropButton;
	CButton		m_DualAudioOutput;

	BOOL		m_fAutoloadAudio;
	CString		m_sAudioPaths;
	BOOL		m_fPrioritizeExternalAudio;

	BOOL		m_fAudioNormalize;
	int			m_iAudioRecoverStep;
	CSliderCtrl	m_AudioRecoverStepCtrl;
	int			m_AudioBoostPos;
	CSliderCtrl	m_AudioBoostCtrl;
	BOOL		m_fAudioTimeShift;
	CButton		m_fAudioTimeShiftCtrl;
	int			m_tAudioTimeShift;
	CIntEdit	m_tAudioTimeShiftCtrl;
	CSpinButtonCtrl m_tAudioTimeShiftSpin;

	CToolTipCtrl m_tooltip;

	void ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr));

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnAudioRendererChange();
	afx_msg void OnAudioRenderPropClick();
	afx_msg void OnDualAudioOutputCheck();

	afx_msg void OnBnClickedResetAudioPaths();

	afx_msg void OnUpdateNormalize(CCmdUI* pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
	virtual void OnCancel();
};
