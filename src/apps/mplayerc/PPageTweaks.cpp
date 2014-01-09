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
#include "MainFrm.h"
#include "PPageTweaks.h"
#include "MiniDump.h"

// CPPageTweaks dialog

IMPLEMENT_DYNAMIC(CPPageTweaks, CPPageBase)
CPPageTweaks::CPPageTweaks()
	: CPPageBase(CPPageTweaks::IDD, CPPageTweaks::IDD)
	, m_nJumpDistS(0)
	, m_nJumpDistM(0)
	, m_nJumpDistL(0)
	, m_fFastSeek(FALSE)
	, m_fPreventMinimize(FALSE)
	, m_fDontUseSearchInFolder(FALSE)
	, m_fLCDSupport(FALSE)
{
}

CPPageTweaks::~CPPageTweaks()
{
}

void CPPageTweaks::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_nJumpDistS);
	DDX_Text(pDX, IDC_EDIT2, m_nJumpDistM);
	DDX_Text(pDX, IDC_EDIT3, m_nJumpDistL);
	DDX_Check(pDX, IDC_CHECK6, m_fPreventMinimize);
	DDX_Check(pDX, IDC_CHECK7, m_fDontUseSearchInFolder);
	DDX_Check(pDX, IDC_CHECK1, m_fFastSeek);
	DDX_Check(pDX, IDC_CHECK_LCD, m_fLCDSupport);
	DDX_Check(pDX, IDC_CHECK2, m_fMiniDump);
}

BOOL CPPageTweaks::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

	m_nJumpDistS			= s.nJumpDistS;
	m_nJumpDistM			= s.nJumpDistM;
	m_nJumpDistL			= s.nJumpDistL;

	m_fPreventMinimize			= s.fPreventMinimize;
	m_fDontUseSearchInFolder	=s.fDontUseSearchInFolder;

	m_fFastSeek		= s.fFastSeek;
	m_fLCDSupport	= s.fLCDSupport;

	m_fMiniDump		= s.fMiniDump;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageTweaks::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();
	s.nJumpDistS			= m_nJumpDistS;
	s.nJumpDistM			= m_nJumpDistM;
	s.nJumpDistL			= m_nJumpDistL;

	s.fPreventMinimize			= !!m_fPreventMinimize;
	s.fDontUseSearchInFolder	= !!m_fDontUseSearchInFolder;

	s.fFastSeek		= !!m_fFastSeek;
	s.fLCDSupport	= !!m_fLCDSupport;

	s.fMiniDump		= !!m_fMiniDump;
	CMiniDump::SetState(s.fMiniDump);

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageTweaks, CPPageBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()

// CPPageTweaks message handlers

void CPPageTweaks::OnBnClickedButton1()
{
	m_nJumpDistS = DEFAULT_JUMPDISTANCE_1;
	m_nJumpDistM = DEFAULT_JUMPDISTANCE_2;
	m_nJumpDistL = DEFAULT_JUMPDISTANCE_3;

	UpdateData(FALSE);

	SetModified();
}
