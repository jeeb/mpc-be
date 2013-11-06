/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
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


// CPPageOutput dialog

class CPPageOutput : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageOutput)

private:
	CStringArray m_AudioRendererDisplayNames;
	CStringArray m_D3D9GUIDNames;

	CComboBox m_iAudioRendererTypeCtrl;
	CComboBox m_iSecAudioRendererTypeCtrl;

	CComboBox m_iDSVideoRendererTypeCtrl;
	CComboBox m_iRMVideoRendererTypeCtrl;
	CComboBox m_iQTVideoRendererTypeCtrl;
	CComboBox m_iD3D9RenderDeviceCtrl;

	CButton m_audRendPropButton;
	CButton m_DualAudioOutput;

	int m_iDSVideoRendererType_store;
public:
	CPPageOutput();
	virtual ~CPPageOutput();

	enum { IDD = IDD_PPAGEOUTPUT };
	int m_iDSVideoRendererType;
	int m_iRMVideoRendererType;
	int m_iQTVideoRendererType;
	int m_iAPSurfaceUsage;
	int m_iAudioRendererType;
	int m_iSecAudioRendererType;
	int m_iDX9Resizer;
	BOOL m_fVMRMixerMode;
	BOOL m_fVMRMixerYUV;
	BOOL m_fD3DFullscreen;
	BOOL m_fVMR9AlterativeVSync;
	BOOL m_fResetDevice;
	CString m_iEvrBuffers;

	BOOL m_fD3D9RenderDevice;
	int m_iD3D9RenderDevice;

	void ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr));

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateMixerYUV(CCmdUI* pCmdUI);
	afx_msg void OnSurfaceChange();
	afx_msg void OnDSRendererChange();
	afx_msg void OnRMRendererChange();
	afx_msg void OnQTRendererChange();
	afx_msg void OnFullscreenCheck();
	afx_msg void OnD3D9DeviceCheck();
	afx_msg void OnAudioRendererChange();
	afx_msg void OnAudioRenderPropClick();
	afx_msg void OnDualAudioOutputCheck();
};
