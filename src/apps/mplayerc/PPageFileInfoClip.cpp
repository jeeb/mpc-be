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
#include "PPageFileInfoClip.h"
#include <atlbase.h>
#include <qnetwork.h>
#include "MainFrm.h"
#include "../../DSUtil/DSUtil.h"
#include "../../DSUtil/WinAPIUtils.h"


// CPPageFileInfoClip dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoClip, CPropertyPage)
CPPageFileInfoClip::CPPageFileInfoClip(CString fn, IFilterGraph* pFG)
	: CPropertyPage(CPPageFileInfoClip::IDD, CPPageFileInfoClip::IDD)
	, m_fn(fn)
	, m_pFG(pFG)
	, m_clip(ResStr(IDS_AG_NONE))
	, m_author(ResStr(IDS_AG_NONE))
	, m_copyright(ResStr(IDS_AG_NONE))
	, m_rating(ResStr(IDS_AG_NONE))
	, m_location_str(ResStr(IDS_AG_NONE))
	, m_hIcon(NULL)
{
}

CPPageFileInfoClip::~CPPageFileInfoClip()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

BOOL CPPageFileInfoClip::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_LBUTTONDBLCLK && pMsg->hwnd == m_location.m_hWnd && !m_location_str.IsEmpty()) {
		CString path = m_location_str;

		if (path[path.GetLength() - 1] != '\\') {
			path += _T("\\");
		}

		path += m_fn;

		if (ExploreToFile(path)) {
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CPPageFileInfoClip::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Text(pDX, IDC_EDIT4, m_clip);
	DDX_Text(pDX, IDC_EDIT3, m_author);
	DDX_Text(pDX, IDC_EDIT2, m_copyright);
	DDX_Text(pDX, IDC_EDIT5, m_rating);
	DDX_Control(pDX, IDC_EDIT6, m_location);
	DDX_Control(pDX, IDC_EDIT7, m_desc);
}

#define SETPAGEFOCUS WM_APP+252 // arbitrary number, can be changed if necessary
BEGIN_MESSAGE_MAP(CPPageFileInfoClip, CPropertyPage)
	ON_WM_SIZE()
	ON_MESSAGE(SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

// CPPageFileInfoClip message handlers

BOOL CPPageFileInfoClip::OnInitDialog()
{
	__super::OnInitDialog();

	if (m_fn == _T("")) {
		BeginEnumFilters(m_pFG, pEF, pBF) {
			CComQIPtr<IFileSourceFilter> pFSF = pBF;
			if (pFSF) {
				LPOLESTR pFN = NULL;
				AM_MEDIA_TYPE mt;

				if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
					m_fn = CStringW(pFN);
					CoTaskMemFree(pFN);
				}

				break;
			}
		}
		EndEnumFilters
	}

	m_hIcon = LoadIcon(m_fn, false);
	if (m_hIcon) {
		m_icon.SetIcon(m_hIcon);
	}

	m_fn.TrimRight('/');

	if (m_fn.Find(_T("://")) > 0) {
		if (m_fn.Find(_T("/"), m_fn.Find(_T("://")) + 3) < 0) {
			m_location_str = m_fn;
		}
	}
	
	if (m_location_str.IsEmpty() || m_location_str == ResStr(IDS_AG_NONE)) {
		int i = max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

		if (i >= 0 && i < m_fn.GetLength()-1) {
			m_location_str = m_fn.Left(i);
			m_fn = m_fn.Mid(i+1);

			if (m_location_str.GetLength() == 2 && m_location_str[1] == ':') {
				m_location_str += '\\';
			}
		}
	}

	m_location.SetWindowText(m_location_str);

	bool fEmpty = true;
	BeginEnumFilters(m_pFG, pEF, pBF) {
		if (CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF) {
			CComBSTR bstr;
			if (SUCCEEDED(pAMMC->get_Title(&bstr)) && wcslen(bstr.m_str) > 0) {
				m_clip = bstr.m_str;
				fEmpty = false;
			}
			if (SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && wcslen(bstr.m_str) > 0) {
				m_author = bstr.m_str;
				fEmpty = false;
			}
			if (SUCCEEDED(pAMMC->get_Copyright(&bstr)) && wcslen(bstr.m_str) > 0) {
				m_copyright = bstr.m_str;
				fEmpty = false;
			}
			if (SUCCEEDED(pAMMC->get_Rating(&bstr)) && wcslen(bstr.m_str) > 0) {
				m_rating = bstr.m_str;
				fEmpty = false;
			}
			if (SUCCEEDED(pAMMC->get_Description(&bstr)) && wcslen(bstr.m_str) > 0) {
				CString desc(bstr.m_str);
				desc.Replace(_T(";"), _T("\r\n"));
				m_desc.SetWindowText(desc);
				fEmpty = false;
			}
			if (!fEmpty) {
				break;
			}
		}
	}
	EndEnumFilters;

	CString strTitleAlt = ((CMainFrame*)AfxGetMyApp()->GetMainWnd())->m_strTitleAlt;
	if (!strTitleAlt.IsEmpty()) {
		m_clip = strTitleAlt.Left(strTitleAlt.GetLength() - 4);
	}

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageFileInfoClip::OnSetActive()
{
	BOOL ret = __super::OnSetActive();

	PostMessage(SETPAGEFOCUS, 0, 0L);

	return ret;
}

LRESULT CPPageFileInfoClip::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();
	psheet->GetTabControl()->SetFocus();

	return 0;
}

void CPPageFileInfoClip::OnSize(UINT nType, int cx, int cy) 
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	CRect r(0, 0, 0, 0);
	if (m_desc.m_hWnd) {
		m_desc.GetWindowRect(&r);
	}

	r.right += dx;
	r.bottom += dy;

	if (m_desc.m_hWnd) {
		m_desc.SetWindowPos(NULL, 0, 0, r.Width(), r.Height(),SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}

	HDWP hDWP = ::BeginDeferWindowPos(1);
	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild != GetDlgItem(IDC_EDIT7) && pChild != GetDlgItem(IDC_DEFAULTICON)) {
			pChild->GetWindowRect(&r); 
			ScreenToClient(&r); 
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		}
	}
	::EndDeferWindowPos(hDWP);
}