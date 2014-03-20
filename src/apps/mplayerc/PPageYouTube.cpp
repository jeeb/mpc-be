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

#include "stdafx.h"
#include "PPageYoutube.h"

// CPPageYoutube dialog

IMPLEMENT_DYNAMIC(CPPageYoutube, CPPageBase)
CPPageYoutube::CPPageYoutube()
	: CPPageBase(CPPageYoutube::IDD, CPPageYoutube::IDD)
	, m_iYoutubeFormatType(0)
	, m_iYoutubeSourceType(0)
{
}

CPPageYoutube::~CPPageYoutube()
{
}

void CPPageYoutube::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_iYoutubeFormatCtrl);
	DDX_Radio(pDX, IDC_RADIO1, m_iYoutubeSourceType);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnBnClickedRadio12)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

	m_iYoutubeFormatCtrl.AddString(ResStr(IDS_PPAGE_FS_DEFAULT));
	m_iYoutubeFormatType = 0;

	int j = 0;
	for (size_t i = 0; i < _countof(youtubeProfiles); i++) {
		j++;
		m_YoutubeProfiles.Add(youtubeProfiles[i]);

		CString fmt;
		fmt.Format(_T("%s@%dp"), youtubeProfiles[i].Container, youtubeProfiles[i].Resolution);
		m_iYoutubeFormatCtrl.AddString(fmt);

		if (youtubeProfiles[i].iTag == s.iYoutubeTag) {
			m_iYoutubeFormatType = j;
		}
	}

	CorrectComboListWidth(m_iYoutubeFormatCtrl);
	m_iYoutubeFormatCtrl.SetCurSel(m_iYoutubeFormatType);

	m_iYoutubeSourceType = s.iYoutubeSource ? 1 : 0;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageYoutube::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	m_iYoutubeFormatType = m_iYoutubeFormatCtrl.GetCurSel();

	if (m_iYoutubeFormatType <= 0 || (m_iYoutubeFormatType - 1) >= _countof(youtubeProfiles)) {
		s.iYoutubeTag = m_iYoutubeFormatType;
	} else {
		s.iYoutubeTag = m_YoutubeProfiles[m_iYoutubeFormatType - 1].iTag;
	}

	s.iYoutubeSource = m_iYoutubeSourceType;

	return __super::OnApply();
}

void CPPageYoutube::OnBnClickedRadio12(UINT nID)
{
	SetModified();
}
