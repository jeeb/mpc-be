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
#include <shlobj.h>
#include <dlgs.h>
#include "OpenFileDlg.h"

#define __DUMMY__ _T("*.*")

bool COpenFileDlg::m_fAllowDirSelection = false;
WNDPROC COpenFileDlg::m_wndProc = NULL;

// COpenFileDlg

IMPLEMENT_DYNAMIC(COpenFileDlg, CFileDialog)
COpenFileDlg::COpenFileDlg(CAtlArray<CString>& mask, bool fAllowDirSelection, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
						   DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd)
	: CFileDialog(TRUE, lpszDefExt, lpszFileName, dwFlags|OFN_NOVALIDATE, lpszFilter, pParentWnd, 0)
	, m_mask(mask)
{
	m_fAllowDirSelection = fAllowDirSelection;

	CString str(lpszFileName);

	if (str.IsEmpty()) {
		CRecentFileList& MRU = AfxGetAppSettings().MRU;
		MRU.ReadList();

		for (int i = 0; i < MRU.GetSize(); i++) {
			if (!MRU[i].IsEmpty()) {
				str = MRU[i];
				break;
			}
		}
	}

	if (str.Find(_T("://")) > 0) {
		str.Empty();
	}

	str = GetFolderOnly(str);

	m_InitialDir = DNew TCHAR[max(1000, str.GetLength() + 1)];
	memset(m_InitialDir, 0, sizeof(m_InitialDir));
	_tcscpy(m_InitialDir, str);
	m_pOFN->lpstrInitialDir = m_InitialDir;

	m_buff = DNew TCHAR[10000];
	memset(m_buff, 0, sizeof(m_buff));
	m_pOFN->lpstrFile = m_buff;
	m_pOFN->nMaxFile = 10000;
}

COpenFileDlg::~COpenFileDlg()
{
	delete [] m_InitialDir;
	delete [] m_buff;
}

BEGIN_MESSAGE_MAP(COpenFileDlg, CFileDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// COpenFileDlg message handlers

LRESULT CALLBACK COpenFileDlg::WindowProcNew(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message ==  WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK
			&& m_fAllowDirSelection) {
		CAutoVectorPtr<TCHAR> path;
		path.Allocate(_MAX_PATH+1);

		if (::GetDlgItemText(hwnd, cmb13, (TCHAR*)path, _MAX_PATH) == 0) {
			::SendMessage(hwnd, CDM_SETCONTROLTEXT, edt1, (LPARAM)__DUMMY__);
		}
	}

	return CallWindowProc(COpenFileDlg::m_wndProc, hwnd, message, wParam, lParam);
}

BOOL COpenFileDlg::OnInitDialog()
{
	CFileDialog::OnInitDialog();

	m_wndProc = (WNDPROC)SetWindowLongPtr(GetParent()->m_hWnd, GWLP_WNDPROC , (LONG_PTR)WindowProcNew);

	return TRUE;
}

void COpenFileDlg::OnDestroy()
{
	int i = GetPathName().Find(__DUMMY__);

	if (i >= 0) {
		m_pOFN->lpstrFile[i] = m_pOFN->lpstrFile[i+1] = 0;
	}

	CFileDialog::OnDestroy();
}

BOOL COpenFileDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);

	OFNOTIFY* pNotify = (OFNOTIFY*)lParam;

	if (__super::OnNotify(wParam, lParam, pResult)) {
		ASSERT(pNotify->hdr.code != CDN_INCLUDEITEM);
		return TRUE;
	}

	switch (pNotify->hdr.code) {
		case CDN_INCLUDEITEM:
			if (OnIncludeItem((OFNOTIFYEX*)lParam, pResult)) {
				return TRUE;
			}

			break;
	}

	return FALSE;
}

BOOL COpenFileDlg::OnIncludeItem(OFNOTIFYEX* pOFNEx, LRESULT* pResult)
{
	TCHAR buff[_MAX_PATH];

	if (!SHGetPathFromIDList((LPCITEMIDLIST)pOFNEx->pidl, buff)) {
		STRRET s;
		HRESULT hr = ((IShellFolder*)pOFNEx->psf)->GetDisplayNameOf((LPCITEMIDLIST)pOFNEx->pidl, SHGDN_NORMAL|SHGDN_FORPARSING, &s);

		if (S_OK != hr) {
			return FALSE;
		}

		switch (s.uType) {
			case STRRET_CSTR:
				_tcscpy_s(buff, CString(s.cStr));
				break;
			case STRRET_WSTR:
				_tcscpy_s(buff, CString(s.pOleStr));
				CoTaskMemFree(s.pOleStr);
				break;
			default:
				return FALSE;
		}
	}

	CString fn(buff);
	/*
		WIN32_FILE_ATTRIBUTE_DATA fad;
		if (GetFileAttributesEx(fn, GetFileExInfoStandard, &fad)
		&& (fad.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			return FALSE;
	*/

	int i = fn.ReverseFind('.'), j = fn.ReverseFind('\\');

	if (i < 0 || i < j) {
		return FALSE;
	}

	CString mask = m_mask[pOFNEx->lpOFN->nFilterIndex-1] + _T(";");
	CString ext = fn.Mid(i).MakeLower() + _T(";");

	*pResult = mask.Find(ext) >= 0 || mask.Find(_T("*.*")) >= 0;

	return TRUE;
}
