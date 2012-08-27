/*
 * $Id$
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

#include "stdafx.h"
#include "AviSplitterSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"

#define ResStr(id) CString(MAKEINTRESOURCE(id))

#define LEFT_SPACING		10
#define VERTICAL_SPACING	25

CAviSplitterSettingsWnd::CAviSplitterSettingsWnd(void)
{
}

bool CAviSplitterSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMSF);

	m_pMSF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMSF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMSF) {
		return false;
	}

	return true;
}

void CAviSplitterSettingsWnd::OnDisconnect()
{
	m_pMSF.Release();
}

bool CAviSplitterSettingsWnd::OnActivate()
{
	int nPosY	= 10;

	m_cbBadInterleavedSuport.Create (_T("Support \"Bad\" Interleaved files"), WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect (LEFT_SPACING,  nPosY, 305, nPosY+15), this, IDC_PP_INTERLEAVED_SUPPORT);

	nPosY += VERTICAL_SPACING;
	m_cbSetReindex.Create (_T("Reindex broken files"), WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect (LEFT_SPACING,  nPosY, 305, nPosY+15), this, IDC_PP_SET_REINDEX);

	if (m_pMSF) {
		m_cbBadInterleavedSuport.SetCheck(m_pMSF->GetBadInterleavedSuport());
		m_cbSetReindex.SetCheck(m_pMSF->GetReindex());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	return true;
}

void CAviSplitterSettingsWnd::OnDeactivate()
{
}

bool CAviSplitterSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMSF) {
		m_pMSF->SetBadInterleavedSuport(m_cbBadInterleavedSuport.GetCheck());
		m_pMSF->SetReindex(m_cbSetReindex.GetCheck());
		m_pMSF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CAviSplitterSettingsWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()
