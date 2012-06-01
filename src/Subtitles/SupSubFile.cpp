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
#include "SupSubFile.h"

#define SUP_HEADER_SIZE	10

CSupSubFile::CSupSubFile(CCritSec* pLock)
	: CSubPicProviderImpl(pLock)
{
	m_name		= _T("");
	m_pSub		= DNew CHdmvSub();
	fThreadRun	= false;
}

CSupSubFile::~CSupSubFile()
{
	if (m_Thread) {
		WaitForSingleObject(m_Thread->m_hThread, (DWORD)5000); // main maximum 5 sec before terminate;
		if (fThreadRun) {
			TerminateThread(m_Thread->m_hThread, (DWORD)-1 );
		}
	}

	if (m_pSub) {
		m_pSub->Reset();
		delete m_pSub;
	}
}

static UINT64 ReadByte(CFile* mfile, size_t count = 1)
{
	if (count <= 0 || count >= 64) {
		return 0;
	}
	UINT64 ret = 0;
	BYTE buf[64];
	if (mfile->Read(buf, count) != count) {
		return 0;
	}
	for(size_t i = 0; i<count; i++) {
		ret = (ret << 8) + (buf[i] & 0xff);
	}
	
	return ret;
}

static CString StripPath(CString path)
{
	CString p = path;
	p.Replace('\\', '/');
	p = p.Mid(p.ReverseFind('/')+1);
	return (p.IsEmpty() ? path : p);
}

static UINT ThreadProc(LPVOID pParam)
{
	return (static_cast<CSupSubFile*>(pParam))->ThreadProc();
}

bool CSupSubFile::Open(CString fn)
{
	CFile f;
	if (!f.Open(fn, CFile::modeRead|CFile::typeBinary|CFile::shareDenyNone)) {
		return false;
	}

	WORD sync = 0;
	sync = (WORD)ReadByte(&f, 2);
	if (sync != 'PG') {
		f.Close();
		return false;
	}
	f.Close();
	m_name = fn;

	m_Thread = AfxBeginThread(::ThreadProc, static_cast<LPVOID>(this));

	return (m_Thread) ? true : false;
}

UINT CSupSubFile::ThreadProc()
{
	CFile f;
	if (!f.Open(m_name, CFile::modeRead|CFile::typeBinary|CFile::shareDenyNone)) {
		f.Close();
		return 1;
	}

	fThreadRun = true;
	
	m_name	= StripPath(m_name);
	m_name	= m_name.Left(m_name.GetLength() - 4);
	m_name	= _T("SUP - ") + m_name;

	CAutoLock cAutoLock(&m_csCritSec);

	f.SeekToBegin();

	CMemFile m_sub;
	m_sub.SetLength(f.GetLength());
	m_sub.SeekToBegin();

	int len;
	BYTE buff[65536];
	while ((len = f.Read(buff, sizeof(buff))) > 0) {
		m_sub.Write(buff, len);
	}
	f.Close();
	m_sub.Seek(SUP_HEADER_SIZE, CFile::begin); // Skip first block

	CAtlList<UINT64> m_idx;

	WORD sync	= 0;
	UINT64 pos	= 0;
	while (m_sub.GetPosition() < (m_sub.GetLength() - 10)) {
		pos = m_sub.GetPosition();
		sync = (WORD)ReadByte(&m_sub, 2);
		if (sync == 'PG') {
			m_idx.AddTail(pos);
			m_sub.Seek(8, CFile::current);
		} else {
			m_sub.Seek(-1, CFile::current);
		}
	}
	m_idx.AddTail(m_sub.GetLength());

	REFERENCE_TIME rtStart = 0;
	UINT64 pos_start	= 0;
	UINT64 pos_stop		= 0;
	POSITION POS		= m_idx.GetHeadPosition();
	m_sub.SeekToBegin();
	while (POS) {
		pos_stop	= m_idx.GetNext(POS);
		UINT64 size	= pos_stop - pos_start - SUP_HEADER_SIZE;

		m_sub.Seek(pos_start + 2, CFile::begin);
		rtStart = UINT64(ReadByte(&m_sub, 4) * 1000 / 9);
		m_sub.Seek(4, CFile::current);
		m_sub.Read(buff, size);
		m_pSub->ParseSample(buff, size, rtStart, 0);

		pos_start = pos_stop;
	}

	m_sub.Close();

	fThreadRun = false;
	return 0;
}

STDMETHODIMP CSupSubFile::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = NULL;

	return
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(POSITION) CSupSubFile::GetStartPosition(REFERENCE_TIME rt, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetStartPosition(rt, fps);
}

STDMETHODIMP_(POSITION) CSupSubFile::GetNext(POSITION pos)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetNext(pos);
}

STDMETHODIMP_(REFERENCE_TIME) CSupSubFile::GetStart(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetStart(pos);
}

STDMETHODIMP_(REFERENCE_TIME) CSupSubFile::GetStop(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetStop(pos);
}

STDMETHODIMP_(bool) CSupSubFile::IsAnimated(POSITION pos)
{
	return false;
}

STDMETHODIMP CSupSubFile::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CAutoLock cAutoLock(&m_csCritSec);
	m_pSub->Render (spd, rt, bbox);

	return S_OK;
}

STDMETHODIMP CSupSubFile::GetTextureSize(POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
	CAutoLock cAutoLock(&m_csCritSec);
	HRESULT hr = m_pSub->GetTextureSize(pos, MaxTextureSize, VideoSize, VideoTopLeft);
	return hr;
}

// IPersist

STDMETHODIMP CSupSubFile::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CSupSubFile::GetStreamCount()
{
	return 1;
}

STDMETHODIMP CSupSubFile::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	if (iStream != 0) {
		return E_INVALIDARG;
	}

	if (ppName) {
		*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR));
		if (!(*ppName)) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s (*ppName, m_name.GetLength()+1, CStringW(m_name));
	}

	if (pLCID) {
		*pLCID = 0;
	}

	return S_OK;
}

STDMETHODIMP_(int) CSupSubFile::GetStream()
{
	return 0;
}

STDMETHODIMP CSupSubFile::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CSupSubFile::Reload()
{
	return S_OK;
}
