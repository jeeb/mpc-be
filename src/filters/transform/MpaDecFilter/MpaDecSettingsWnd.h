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

#include "../../core/InternalPropertyPage.h"
#include "MpaDecFilter.h"
#include <afxcmn.h>

class __declspec(uuid("24103041-884B-4772-B0D3-A600E7CBFEC7"))
	CMpaDecSettingsWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IMpaDecFilter> m_pMDF;

	int m_outputformat;
	bool m_mixer;
	int  m_mixer_layout;
	bool m_drc;
	bool m_spdif_ac3;
	bool m_spdif_dts;

	enum {
		IDC_PP_COMBO1 = 10000,
		IDC_PP_COMBO2,
		IDC_PP_CHECK_MIXER,
		IDC_PP_CHECK_DRC,
		IDC_PP_CHECK_AC3_CPDIF,
		IDC_PP_CHECK_DTS_CPDIF,
	};

	CStatic m_outputformat_static;
	CComboBox m_outputformat_combo;

	CButton   m_mixer_group;
	CButton   m_mixer_check;
	CStatic   m_mixer_layout_static;
	CComboBox m_mixer_layout_combo;

	CButton   m_drc_check;

	CButton   m_spdif_group;
	CButton   m_spdif_ac3_check;
	CButton   m_spdif_dts_check;

public:
	CMpaDecSettingsWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCTSTR GetWindowTitle() { return _T("Settings"); }
	static CSize GetWindowSize() { return CSize(320, 305); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnMixerCheck();
};
