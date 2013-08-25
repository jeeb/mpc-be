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
	, m_bProxyEnable(FALSE)
	, m_nProxyPort(0)
{
	CRegKey key;
	ULONG len			= MAX_PATH;
	DWORD ProxyEnable	= 0;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", KEY_READ)) {
		if (ERROR_SUCCESS == key.QueryDWORDValue(L"ProxyEnable", ProxyEnable) && ProxyEnable
				&& ERROR_SUCCESS == key.QueryStringValue(L"ProxyServer", m_sProxyServer.GetBufferSetLength(MAX_PATH), &len)) {
			m_sProxyServer.ReleaseBufferSetLength(len);

			CAtlList<CString> sl;
			m_sProxyServer = Explode(m_sProxyServer, sl, ';');
			if (sl.GetCount() > 1) {
				POSITION pos = sl.GetHeadPosition();
				while (pos) {
					CAtlList<CString> sl2;
					if (!Explode(sl.GetNext(pos), sl2, '=', 2).CompareNoCase(L"http")
							&& sl2.GetCount() == 2) {
						m_sProxyServer = sl2.GetTail();
						break;
					}
				}
			}

			m_sProxyServer = Explode(m_sProxyServer, sl, ':');
			if (sl.GetCount() > 1) {
				m_nProxyPort = _tcstol(sl.GetTail(), NULL, 10);
			}

			m_bProxyEnable = (ProxyEnable && !m_sProxyServer.IsEmpty() && m_nProxyPort);
		}

		key.Close();
	}
}

BOOL CMPCSocket::Connect(CString url, BOOL bConnectOnly)
{
	CUrl Url;
	Url.CrackUrl(url);

	return Connect(Url, bConnectOnly);
}

BOOL CMPCSocket::Connect(CUrl url, BOOL bConnectOnly)
{
	if (!__super::Connect(	
				m_bProxyEnable ? m_sProxyServer : url.GetHostName(),
				m_bProxyEnable ? m_nProxyPort : url.GetPortNumber())) {
		KillTimeOut();
		return FALSE;
	}

	m_url = url;

	CStringA host = CStringA(url.GetHostName());
	CStringA path = CStringA(url.GetUrlPath()) + CStringA(url.GetExtraInfo());

	if (m_bProxyEnable) {
		path = "http://" + host + path;
	}

	m_hdr.Format(
		"GET %s HTTP/1.0\r\n"
		"User-Agent: MPC-BE\r\n"
		"Host: %s\r\n"
		"Accept: */*\r\n"
		"\r\n", path, host);

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

void CMPCSocket::SetProxy(CString ProxyServer, DWORD ProxyPort)
{
	if (!ProxyServer.IsEmpty() && ProxyPort) {
		m_bProxyEnable	= TRUE;
		m_sProxyServer	= ProxyServer;
		m_nProxyPort	= ProxyPort;
	}
}