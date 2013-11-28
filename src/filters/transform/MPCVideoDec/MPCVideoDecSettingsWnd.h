/*
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

#include "../../filters/InternalPropertyPage.h"
#include "IMPCVideoDec.h"
#include "resource.h"

// === New swscaler options
#include "Version.h"
//

enum {
	IDC_PP_THREAD_NUMBER = 10000,
	IDC_PP_DISCARD_MODE,
	IDC_PP_DEINTERLACING,
	IDC_PP_AR,
	IDC_PP_DXVA_CHECK,
	IDC_PP_DXVA_SD,
	IDC_PP_SW_NV12,
	IDC_PP_SW_YV12,
	IDC_PP_SW_YUY2,
#if ENABLE_AYUV
	IDC_PP_SW_AYUV,
#endif
	IDC_PP_SW_P010,
	IDC_PP_SW_P210,
	IDC_PP_SW_P016,
	IDC_PP_SW_P216,
	IDC_PP_SW_RGB32,
	IDC_PP_SWPRESET,
	IDC_PP_SWSTANDARD,
	IDC_PP_SWRGBLEVELS,
	
};

class __declspec(uuid("D5AA0389-D274-48e1-BF50-ACB05A56DDE0"))
	CMPCVideoDecSettingsWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IMPCVideoDecFilter> m_pMDF;

	CFont		m_arrowsFont;

	CButton		m_grpFFMpeg;
	CStatic		m_txtThreadNumber;
	CComboBox	m_cbThreadNumber;
	CStatic		m_txtDiscardMode;
	CComboBox	m_cbDiscardMode;
	CStatic		m_txtDeinterlacing;
	CComboBox	m_cbDeinterlacing;

	CButton		m_grpDXVA;
	CStatic		m_txtDXVAMode;
	CEdit		m_edtDXVAMode;
	CStatic		m_txtVideoCardDescription;
	CEdit		m_edtVideoCardDescription;

	CButton		m_cbARMode;

	CStatic		m_txtDXVACompatibilityCheck;
	CComboBox	m_cbDXVACompatibilityCheck;

	CButton		m_cbDXVA_SD;

	// === New swscaler options
	CButton		m_grpFmtConv;
	CStatic		m_txtSwOutputFormats;
	CStatic     m_txt420;
	CStatic     m_txt422;
	CStatic     m_txt444;
	CStatic     m_txtRGB;
	CButton		m_cbNV12;
	CButton		m_cbYV12;
	CButton		m_cbYUY2;
#if ENABLE_AYUV
	CButton		m_cbAYUV;
#endif
	CButton		m_cbP010;
	CButton		m_cbP210;
	CButton		m_cbP016;
	CButton		m_cbP216;
	CButton		m_cbRGB32;

	CStatic     m_txtSwPreset;
	CComboBox   m_cbSwPreset;

	CStatic     m_txtSwStandard;
	CComboBox   m_cbSwStandard;

	CStatic     m_txtSwRGBLevels;
	CComboBox   m_cbSwRGBLevels;

	CStatic     m_txtSwVersion;
	CString     m_strSwVersion;

public:
	CMPCVideoDecSettingsWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCTSTR GetWindowTitle() { return MAKEINTRESOURCE(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(580, 260); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedYUY2();
	afx_msg void OnBnClickedRGB32();
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
};

class __declspec(uuid("3C395D46-8B0F-440d-B962-2F4A97355453"))
	CMPCVideoDecCodecWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IMPCVideoDecFilter> m_pMDF;

	CButton			m_grpSelectedCodec;
	CCheckListBox	m_lstCodecs;
	CImageList		m_onoff;

public:
	CMPCVideoDecCodecWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCTSTR GetWindowTitle() { return _T("Codecs");    }
	static CSize GetWindowSize()    { return CSize(340, 290); }

	DECLARE_MESSAGE_MAP()
};
