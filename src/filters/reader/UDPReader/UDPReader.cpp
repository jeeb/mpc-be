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
#include "UDPReader.h"
#include <atlutil.h>
#include "../../../DSUtil/DSUtil.h"
#include "../../../apps/mplayerc/SettingsDefines.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CUDPReader), UDPReaderName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CUDPReader>, NULL, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(_T("udp"), 0, _T("Source Filter"), CStringFromGUID(__uuidof(CUDPReader)));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	// TODO

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

#define MAXSTORESIZE	25*MEGABYTE	// The maximum size of a buffer for storing the received information
#define MAXBUFSIZE		65536		// Max UDP Packet size is 64 Kbyte

//
// CUDPReader
//

CUDPReader::CUDPReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CUDPReader"), pUnk, &m_stream, phr, __uuidof(this))
{
	if (phr) {
		*phr = S_OK;
	}
}

CUDPReader::~CUDPReader()
{
}

STDMETHODIMP CUDPReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFileSourceFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CUDPReader::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, UDPReaderName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CUDPReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (!m_stream.Load(pszFileName)) {
		return E_FAIL;
	}

	m_fn = pszFileName;

	CMediaType mt;
	mt.majortype	= MEDIATYPE_Stream;
	mt.subtype		= MEDIASUBTYPE_MPEG2_TRANSPORT;
	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CUDPReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	if (!ppszFileName) {
		return E_POINTER;
	}

	*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR));
	if (!(*ppszFileName)) {
		return E_OUTOFMEMORY;
	}

	wcscpy_s(*ppszFileName, m_fn.GetLength() + 1, m_fn);

	return S_OK;
}

// CUDPStream

CUDPStream::CUDPStream()
{
	m_port = 0;
	m_socket = INVALID_SOCKET;

	m_WSAEvent[0] = NULL;
}

CUDPStream::~CUDPStream()
{
	Clear();
}

void CUDPStream::Clear()
{
	if (m_WSAEvent[0] != NULL) {
		WSACloseEvent(m_WSAEvent[0]);
	}

	if (m_socket != INVALID_SOCKET) {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

	if (CAMThread::ThreadExists()) {
		CAMThread::CallWorker(CMD_EXIT);
		CAMThread::Close();
	}

	WSACleanup();

	while (!m_packets.IsEmpty()) {
		delete m_packets.RemoveHead();
	}

	m_pos = m_len = 0;
}

void CUDPStream::Append(BYTE* buff, int len)
{
	CAutoLock cAutoLock(&m_csLock);

	m_packets.AddTail(DNew packet_t(buff, m_len, m_len + len));
	m_len += len;
}

bool CUDPStream::Load(const WCHAR* fnw)
{
	Clear();

	CUrl url;
	if (!url.CrackUrl(fnw)) {
		return false;
	}

	m_protocol = url.GetSchemeName();
	// if(m_protocol != L"udp") return false;

	m_host = url.GetHostName();
	m_port = url.GetPortNumber();

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin_family		= AF_INET;
	m_addr.sin_port			= htons((u_short)m_port);
	m_addr.sin_addr.s_addr	= htonl(INADDR_ANY);

	ip_mreq imr;
	imr.imr_multiaddr.s_addr	= inet_addr(CStringA(m_host));
	imr.imr_interface.s_addr	= INADDR_ANY;

	if ((m_socket = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET) {
		m_WSAEvent[0] = WSACreateEvent();
		WSAEventSelect(m_socket, m_WSAEvent[0], FD_READ);

		DWORD dw = TRUE;
		if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}

		if (bind(m_socket, (struct sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}

		if (IN_MULTICAST(htonl(imr.imr_multiaddr.s_addr))) {
			int ret = setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&imr, sizeof(imr));
			if (ret < 0) {
				closesocket(m_socket);
				m_socket = INVALID_SOCKET;
			}
		}

		dw = MAXBUFSIZE;
		if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
			;
		}

		// set non-blocking mode
		u_long param = 1;
		int res = ioctlsocket(m_socket, FIONBIO, &param);
	}

	if (m_socket == INVALID_SOCKET) {
		return false;
	}

	UINT timeout = 0;
	while (timeout < 500) {
		DWORD res = WSAWaitForMultipleEvents(1, m_WSAEvent, FALSE, 100, FALSE);
		if (res == WSA_WAIT_EVENT_0) {
			int fromlen = sizeof(m_addr);
			char buf[MAXBUFSIZE];
			int len = recvfrom(m_socket, buf, MAXBUFSIZE, 0, (SOCKADDR*)&m_addr, &fromlen);
			if (len <= 0) {
				timeout += 100;
				continue;
			}
			break;
		} else {
			timeout += 100;
			continue;
		}

		break;
	}

	if (timeout == 500) {
		return false;
	}

	CAMThread::Create();
	if (FAILED(CAMThread::CallWorker(CMD_RUN))) {
		Clear();
		return false;
	}

	clock_t start = clock();
	while (clock() - start < 3000 && m_len < MEGABYTE) {
		Sleep(100);
	}

	return true;
}

HRESULT CUDPStream::SetPointer(LONGLONG llPos)
{
	CAutoLock cAutoLock(&m_csLock);

	if (m_packets.IsEmpty() && llPos != 0
			|| !m_packets.IsEmpty() && llPos < m_packets.GetHead()->m_start
			|| !m_packets.IsEmpty() && llPos > m_packets.GetTail()->m_end) {
		TRACE(_T("CUDPStream: SetPointer error - %lld, [%I64d -> %I64d]\n"), llPos, m_packets.GetHead()->m_start, m_packets.GetTail()->m_end);
		return E_FAIL;
	}

	m_pos = llPos;

	return S_OK;
}

HRESULT CUDPStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	CAutoLock cAutoLock(&m_csLock);

	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	while (len > 0 && !m_packets.IsEmpty()) {
		POSITION pos = m_packets.GetHeadPosition();
		while (pos && len > 0) {
			packet_t* p = m_packets.GetNext(pos);

			if (p->m_start <= m_pos && m_pos < p->m_end) {
				DWORD size;

				if (m_pos < p->m_start) {
					ASSERT(0);
					size = (DWORD)min(len, p->m_start - m_pos);
					memset(ptr, 0, size);
				} else {
					size = (DWORD)min(len, p->m_end - m_pos);
					memcpy(ptr, &p->m_buff[m_pos - p->m_start], size);
				}

				m_pos += size;

				ptr += size;
				len -= size;
			}
		}
	}

	CheckBuffer();

	if (pdwBytesRead) {
		*pdwBytesRead = ptr - pbBuffer;
	}

	return S_OK;
}

LONGLONG CUDPStream::Size(LONGLONG* pSizeAvailable)
{
	CAutoLock cAutoLock(&m_csLock);
	if (pSizeAvailable) {
		*pSizeAvailable = m_len;
	}

	return 0;
}

DWORD CUDPStream::Alignment()
{
	return 1;
}

void CUDPStream::Lock()
{
	m_csLock.Lock();
}

void CUDPStream::Unlock()
{
	m_csLock.Unlock();
}

void CUDPStream::CheckBuffer()
{
	CAutoLock cAutoLock(&m_csLock);

#define PacketsSize (m_packets.GetTail()->m_end - m_packets.GetHead()->m_start)
	if (!m_packets.IsEmpty()) {
		while (PacketsSize > MAXSTORESIZE) {
			delete m_packets.RemoveHead();
		}
	}
}

DWORD CUDPStream::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);

#ifdef _DEBUG
	FILE* dump = NULL;
	//dump = _tfopen(_T("c:\\udp_dump.ts"), _T("wb"));
#endif

	for (;;) {
		DWORD cmd = GetRequest();

		switch (cmd) {
			default:
			case CMD_EXIT:
				if (m_socket != INVALID_SOCKET) {
					closesocket(m_socket);
					m_socket = INVALID_SOCKET;
				}
				WSACleanup();
#ifdef _DEBUG
				if (dump) {
					fclose(dump);
				}
#endif
				Reply(S_OK);
				return 0;
			case CMD_RUN:
				Reply(m_socket != INVALID_SOCKET ? S_OK : E_FAIL);

				{
					char buff[MAXBUFSIZE * 2];
					int buffsize = 0;

					UINT timeout = 0;
					while (!CheckRequest(NULL) && timeout < 1000) {
						int len =  0;

						DWORD res = WSAWaitForMultipleEvents(1, m_WSAEvent, FALSE, 100, FALSE);
						if (res == WSA_WAIT_EVENT_0) {
							WSAResetEvent(m_WSAEvent[0]);
							int fromlen = sizeof(m_addr);
							len = recvfrom(m_socket, &buff[buffsize], MAXBUFSIZE, 0, (SOCKADDR*)&m_addr, &fromlen);
							if (len <= 0) {
								int err = WSAGetLastError();
								if (err != WSAEWOULDBLOCK) {
									timeout += 100;
								}
								continue;
							}
						} else {
							timeout += 100;
							continue;
						}

						timeout = 0;

						buffsize += len;

						if (buffsize >= MAXBUFSIZE || m_len == 0) {
#ifdef _DEBUG
							if (dump) {
								fwrite(buff, buffsize, 1, dump);
							}
#endif
							Append((BYTE*)buff, buffsize);
							buffsize = 0;
						}
					}
				}

				break;
		}
	}

	ASSERT(0);
	return (DWORD)-1;
}

CUDPStream::packet_t::packet_t(BYTE* p, __int64 start, __int64 end)
	: m_start(start)
	, m_end(end)
{
	size_t size	= (size_t)(end - start);
	m_buff		= DNew BYTE[size];
	memcpy(m_buff, p, size);
}
