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
#include <WinInet.h> //TODO: remove it
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/MPCSocket.h"
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
	SetRegKeyValue(_T("http"), 0, _T("Source Filter"), CStringFromGUID(__uuidof(CUDPReader)));

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

#ifndef REGISTER_FILTER
	AfxSocketInit();
#endif
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
	mt.majortype = MEDIATYPE_Stream;
	mt.subtype   = m_stream.GetSubtype();
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
	m_protocol = PR_NONE;
	m_socket   = INVALID_SOCKET;
	m_subtype  = MEDIASUBTYPE_NULL;

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

	if (m_protocol == PR_UDP) { // ?
		WSACleanup();
	}

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

	m_url_str = CString(fnw);

	if (!m_url.CrackUrl(m_url_str)) {
		return false;
	}

	CString str_protocol = m_url.GetSchemeName();
	str_protocol.MakeLower();

	if (str_protocol == L"udp") {
		m_protocol = PR_UDP;

		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family      = AF_INET;
		m_addr.sin_port        = htons((u_short)m_url.GetPortNumber());
		m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ip_mreq imr;
		imr.imr_multiaddr.s_addr = inet_addr(CStringA(m_url.GetHostName()));
		imr.imr_interface.s_addr = INADDR_ANY;

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
			ioctlsocket(m_socket, FIONBIO, &param);
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

		// set non-blocking mode // ?
		//u_long param = 0;
		//ioctlsocket(m_socket, FIONBIO, &param);

	} else if (str_protocol == L"http") {
		m_protocol = PR_HTTP;

		if (m_url.GetUrlPathLength() == 0) {
			m_url.SetUrlPath(_T("/"));
			m_url_str += '/';
		}

		BOOL connected = FALSE;
		for (int i = 1; i <=5; i++) {
			bool redir = false;

			if (m_url.GetPortNumber() == ATL_URL_INVALID_PORT_NUMBER) {
				m_url.SetPortNumber(ATL_URL_DEFAULT_HTTP_PORT);
			}

			CMPCSocket socket;
			if (!socket.Create()) {
				return false;
			}

			CStringA hdr;

			socket.SetUserAgent("MPC UDP/HTTP Reader");
			socket.SetTimeOut(3000);
			connected = socket.Connect(m_url);
			socket.KillTimeOut();
			if (connected) {
				while (hdr.GetLength() < 4096) {
					CStringA str;
					str.ReleaseBuffer(socket.Receive(str.GetBuffer(4096), 4096)); // SOCKET_ERROR == -1, also suitable for ReleaseBuffer
					if (str.IsEmpty()) {
						break;
					}
					hdr += str;
					int hdrend = hdr.Find("\r\n\r\n");
					if (hdrend >= 0) {
						hdr.Truncate(hdrend);
						break;
					}
				}
				TRACE("CUDPStream: hdr:\n%s\n", hdr);

				connected = !hdr.IsEmpty();
				hdr.MakeLower();
			}
			socket.Close();

			if (!connected) {
				return false;
			}

			CAtlList<CStringA> sl;
			Explode(hdr, sl, "\n");
			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				CStringA& str = sl.GetNext(pos);
				str.Trim();
				int k = str.Find(":");
				if (k < 1) {
					continue;
				}
				CStringA param = str.Left(k);
				CStringA value = str.Mid(k + 1).TrimLeft();

				if (param == "content-type") {
					if (value == "application/octet-stream") {
						// TODO: make real stream type detector
						m_subtype = MEDIASUBTYPE_MPEG2_TRANSPORT;
					} else if (value == "application/ogg") {
						m_subtype = MEDIASUBTYPE_Ogg;
					} else if (value == "video/webm") {
						m_subtype = MEDIASUBTYPE_Matroska;
					} else if (value == "video/mp4" || value == "video/x-flv" || value == "video/3gpp") {
						m_subtype = MEDIASUBTYPE_NULL;
					} else { // "text/html..." and other
						 // not supported content-type
						connected = FALSE;
					}
				} else if (param == "content-length") {
					// Not stream. This downloadable file. Here it is better to use "File Source (URL)".
					connected = FALSE; 
				} else if (param == "location") {
					if (value.Left(7) == "http://") {
						if (value.Find('/', 8) == -1) { // looking for the beginning of URL path (find '/' after 'http://x')
							value += '/'; // if not found, set minimum URL path
						}
						if (m_url_str != CString(value)) {
							m_url_str = value;
							m_url.CrackUrl(m_url_str);
							redir = true;
						}
					}
				}
			}

			if (!redir) {
				break;
			}
		}

		if (!connected) {
			return false;
		}
	} else {
		return false; // not supported protocol
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

	HINTERNET s = NULL;
	HINTERNET f = NULL;
	if (m_protocol == PR_HTTP) {
		s = InternetOpen(L"MPC UDP/HTTP Reader", 0, NULL, NULL, 0);
		if (s) {
			f = InternetOpenUrl(s, m_url_str, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (f) {
				DWORD cb = sizeof(DWORD);
				DWORD len = 0;
				if (!HttpQueryInfo(f, HTTP_QUERY_CONTENT_LENGTH|HTTP_QUERY_FLAG_NUMBER, &len, &cb, 0)) {
					len = 0;
				}
			}
		}
	}

#ifdef _DEBUG
	FILE* dump = NULL;
	//dump = _tfopen(_T("c:\\http.ts"), _T("wb"));
	FILE* log = NULL;
	//log = _tfopen(_T("c:\\http.txt"), _T("wt"));
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
				//WSACleanup();
#ifdef _DEBUG
				if (dump) {
					fclose(dump);
				}
				if (log) {
					fclose(log);
				}
#endif
				Reply(S_OK);

				InternetCloseHandle(f);
				InternetCloseHandle(s);

				return 0;
			case CMD_RUN:
				//Reply(m_socket != INVALID_SOCKET ? S_OK : E_FAIL);
				Reply(S_OK);

				{
					char buff[MAXBUFSIZE * 2];
					int buffsize = 0;

					UINT timeout = 0;
					while (!CheckRequest(NULL) && timeout < 1000) {
						int len =  0;

						if (m_protocol == PR_UDP) {
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

						} else if (m_protocol == PR_HTTP) {
							DWORD dwBytesRead = 0;
							if (!InternetReadFile(f, (LPVOID)&buff[buffsize], MAXBUFSIZE, &dwBytesRead) || !dwBytesRead) {
								Sleep(50);
								timeout += 50;
								continue;
							}
							len = dwBytesRead;

							timeout = 0;
						}

#ifdef _DEBUG
						if (log) {
							if (buffsize >= len && !memcmp(&buff[buffsize-len], &buff[buffsize], len)) {
								DWORD pid = ((buff[buffsize+1]<<8)|buff[buffsize+2])&0x1fff;
								DWORD counter = buff[buffsize+3]&0xf;
								_ftprintf_s(log, _T("%04d %2d DUP\n"), pid, counter);
							}
						}
#endif

						buffsize += len;

						if (buffsize >= MAXBUFSIZE || m_len == 0) {
#ifdef _DEBUG
							if (dump) {
								fwrite(buff, buffsize, 1, dump);
							}

							if (log) {
								static BYTE pid2counter[0x2000];
								static bool init = false;
								if (!init) {
									memset(pid2counter, 0, sizeof(pid2counter));
									init = true;
								}

								for (int i = 0; i < buffsize; i += 188) {
									DWORD pid = ((buff[i+1]<<8)|buff[i+2])&0x1fff;
									BYTE counter = buff[i+3]&0xf;
									if (pid2counter[pid] != ((counter-1+16)&15)) {
										_ftprintf_s(log, _T("%04x %2d -> %2d\n"), pid, pid2counter[pid], counter);
									}
									pid2counter[pid] = counter;
								}
							}
#endif
							Append((BYTE*)buff, buffsize);
							buffsize = 0;
						}
					}
				}
				if (m_protocol == PR_UDP) { // ?
					WSAResetEvent(m_WSAEvent[0]);
				}

				break;
		}
	}

	ASSERT(0);

	InternetCloseHandle(f);
	InternetCloseHandle(s);

	return (DWORD)-1;
}

CUDPStream::packet_t::packet_t(BYTE* p, __int64 start, __int64 end)
	: m_start(start)
	, m_end(end)
{
	size_t size = (size_t)(end - start);
	m_buff      = DNew BYTE[size];
	memcpy(m_buff, p, size);
}
