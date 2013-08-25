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

#pragma once

#include <afxsock.h>
#include <atlutil.h>

class CMPCSocket : public CSocket
{
protected:
	int m_nTimerID;

	BOOL m_bProxyEnable;
	CString m_sProxyServer;
	DWORD m_nProxyPort;

	CUrl m_url;
	CStringA m_hdr;

	virtual BOOL OnMessagePending();

public:
	CMPCSocket();

	virtual ~CMPCSocket() {
		KillTimeOut();
	}

	BOOL Connect(CString url, BOOL bConnectOnly = FALSE);
	BOOL Connect(CUrl url, BOOL bConnectOnly = FALSE);

	BOOL SetTimeOut(UINT uTimeOut);
	BOOL KillTimeOut();

	BOOL SendRequest();
	void SetProxy(CString ProxyServer, DWORD ProxyPort);
};
