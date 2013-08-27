/*
 * $Id$
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include "MPCSocket.h"
#include "DSUtil.h"

CMPCSocket::CMPCSocket()
	: m_nTimerID(0)
	, m_sUserAgent("MPC-BE")
{
}

BOOL CMPCSocket::Connect(CString url, BOOL bConnectOnly)
{
	CUrl Url;
	
	return (Url.CrackUrl(url) && Connect(Url, bConnectOnly));
}

BOOL CMPCSocket::Connect(CUrl url, BOOL bConnectOnly)
{
	if (!__super::Connect(url.GetHostName(), url.GetPortNumber())) {
		KillTimeOut();
		return FALSE;
	}

	m_url = url;

	m_hdr.Format(
		"GET %s HTTP/1.1\r\n"
		"User-Agent: %s\r\n"
		"Host: %s\r\n"
		"Accept: */*\r\n"
		"\r\n", CStringA(url.GetUrlPath()), m_sUserAgent, CStringA(url.GetHostName()));

	if (!bConnectOnly) {
		SendRequest();
	}

	return TRUE;
}

BOOL CMPCSocket::OnMessagePending()
{
	MSG msg;

	if (::PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE)) {
		if (msg.wParam == (UINT) m_nTimerID) {
			TRACE(_T("CMPCSocket::OnMessagePending(WM_TIMER) PASSED!\n"));
			// Remove the message and call CancelBlockingCall.
			::PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE);
			CancelBlockingCall();
			KillTimeOut();
			return FALSE;  // No need for idle time processing.
		};
	};

	return __super::OnMessagePending();
}

BOOL CMPCSocket::SetTimeOut(UINT uTimeOut)
{
	m_nTimerID = SetTimer(NULL, 0, uTimeOut, NULL);
	return (m_nTimerID != 0);
}

BOOL CMPCSocket::KillTimeOut()
{
	return KillTimer(NULL, m_nTimerID);
}

BOOL CMPCSocket::SendRequest()
{
	return (Send((LPCSTR)m_hdr, m_hdr.GetLength()) < m_hdr.GetLength());
}

void CMPCSocket::SetUserAgent(CStringA UserAgent)
{
	m_sUserAgent = UserAgent;
}
