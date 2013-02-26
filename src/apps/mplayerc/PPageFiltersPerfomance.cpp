/*
 * $Id$
 *
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

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFiltersPerfomance.h"

// CPPageFiltersPerfomance dialog

IMPLEMENT_DYNAMIC(CPPageFiltersPerfomance, CPPageBase)
CPPageFiltersPerfomance::CPPageFiltersPerfomance()
	: CPPageBase(CPPageFiltersPerfomance::IDD, CPPageFiltersPerfomance::IDD)
	, m_nMinQueueSize(0)
	, m_nMaxQueueSize(0)
	, m_nCachSize(0)
	, m_nMinQueuePackets(0)
	, m_nMaxQueuePackets(0)
{
	MEMORYSTATUSEX msEx;
	msEx.dwLength = sizeof(msEx);
	::GlobalMemoryStatusEx(&msEx);
	m_halfMemMB = msEx.ullTotalPhys/0x200000;
}

CPPageFiltersPerfomance::~CPPageFiltersPerfomance()
{
}

void CPPageFiltersPerfomance::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN2, m_nMinQueueSizeCtrl);
	DDX_Control(pDX, IDC_SPIN3, m_nMaxQueueSizeCtrl);
	DDX_Control(pDX, IDC_SPIN4, m_nCachSizeCtrl);
	DDX_Text(pDX, IDC_EDIT3, m_nMinQueueSize);
	DDX_Text(pDX, IDC_EDIT4, m_nMaxQueueSize);
	DDX_Text(pDX, IDC_EDIT5, m_nCachSize);
	DDX_Text(pDX, IDC_EDIT1, m_nMinQueuePackets);
	DDX_Text(pDX, IDC_EDIT2, m_nMaxQueuePackets);
	DDX_Control(pDX, IDC_CHECK1, m_DefaultCtrl);
}

BEGIN_MESSAGE_MAP(CPPageFiltersPerfomance, CPPageBase)
	ON_BN_CLICKED(IDC_CHECK1, &CPPageFiltersPerfomance::OnBnClickedCheck1)
END_MESSAGE_MAP()

// CPPageFiltersPerfomance message handlers

BOOL CPPageFiltersPerfomance::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_CHECK1);

	AppSettings& s = AfxGetAppSettings();

	m_nMinQueueSizeCtrl.SetRange(64, KILOBYTE);
	m_nMaxQueueSizeCtrl.SetRange(10, min(512, m_halfMemMB));
	m_nCachSizeCtrl.SetRange(16, KILOBYTE);

	m_nMinQueueSize	= s.PerfomanceSettings.iMinQueueSize;
	m_nMaxQueueSize	= s.PerfomanceSettings.iMaxQueueSize;
	m_nCachSize		= s.PerfomanceSettings.iCacheLen;

	m_nMinQueuePackets = s.PerfomanceSettings.iMinQueuePackets;
	m_nMaxQueuePackets = s.PerfomanceSettings.iMaxQueuePackets;

	m_DefaultCtrl.SetCheck(s.PerfomanceSettings.fDefault);
	OnBnClickedCheck1();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageFiltersPerfomance::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.PerfomanceSettings.iMinQueueSize	= max(64, min(KILOBYTE, m_nMinQueueSize));
	s.PerfomanceSettings.iMaxQueueSize	= max(10, min(min(512, m_halfMemMB), m_nMaxQueueSize));
	s.PerfomanceSettings.iCacheLen		= max(16, min(KILOBYTE, m_nCachSize));

	s.PerfomanceSettings.iMinQueuePackets = max(10, min(MAXQUEUEPACKETS, m_nMinQueuePackets));
	s.PerfomanceSettings.iMaxQueuePackets = max(s.PerfomanceSettings.iMinQueuePackets*2, min(MAXQUEUEPACKETS*10, m_nMaxQueuePackets));

	s.PerfomanceSettings.UpdateStatus();

	return __super::OnApply();
}

void CPPageFiltersPerfomance::OnBnClickedCheck1()
{
	if (m_DefaultCtrl.GetCheck()) {
		AppSettings& s = AfxGetAppSettings();

		s.PerfomanceSettings.SetDefault();

		m_nMinQueueSize	= s.PerfomanceSettings.iMinQueueSize;
		m_nMaxQueueSize	= s.PerfomanceSettings.iMaxQueueSize;
		m_nCachSize		= s.PerfomanceSettings.iCacheLen;

		m_nMinQueuePackets = s.PerfomanceSettings.iMinQueuePackets;
		m_nMaxQueuePackets = s.PerfomanceSettings.iMaxQueuePackets;

		UpdateData(FALSE);
	}

	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild != GetDlgItem(IDC_CHECK1) && pChild != GetDlgItem(IDC_STATIC)) {
			pChild->EnableWindow(!m_DefaultCtrl.GetCheck()); 
		}
	}
}
