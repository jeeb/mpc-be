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
#include "PPageFileInfoSheet.h"
#include "PPageFileMediaInfo.h"

IMPLEMENT_DYNAMIC(CMPCPropertySheet, CPropertySheet)
CMPCPropertySheet::CMPCPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

// CPPageFileInfoSheet

IMPLEMENT_DYNAMIC(CPPageFileInfoSheet, CMPCPropertySheet)
CPPageFileInfoSheet::CPPageFileInfoSheet(CString fn, CMainFrame* pMainFrame, CWnd* pParentWnd)
	: CMPCPropertySheet(ResStr(IDS_PROPSHEET_PROPERTIES), pParentWnd, 0)
	, m_clip(fn, pMainFrame->m_pGB)
	, m_details(fn, pMainFrame->m_pGB, pMainFrame->m_pCAP)
	, m_res(fn, pMainFrame->m_pGB)
	, m_mi(fn)
	, m_fn(fn)
	, m_bNeedInit(TRUE)
	, m_nMinCX(0)
	, m_nMinCY(0)
{
	AddPage(&m_details);
	AddPage(&m_clip);

	BeginEnumFilters(pMainFrame->m_pGB, pEF, pBF) {
		if (CComQIPtr<IDSMResourceBag> pRB = pBF)
			if (pRB && pRB->ResGetCount() > 0) {
				AddPage(&m_res);
				break;
			}
	}
	EndEnumFilters;

	if (m_fn.Find(_T("://")) < 0) {
		AddPage(&m_mi);
	}
}

CPPageFileInfoSheet::~CPPageFileInfoSheet()
{
	AppSettings& s = AfxGetAppSettings();

	s.iDlgPropX = m_rWnd.Width() - m_nMinCX;
	s.iDlgPropY = m_rWnd.Height() - m_nMinCY;
}

BEGIN_MESSAGE_MAP(CPPageFileInfoSheet, CMPCPropertySheet)
	ON_WM_DESTROY()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_MI, OnSaveAs)
END_MESSAGE_MAP()

// CPPageFileInfoSheet message handlers

BOOL CPPageFileInfoSheet::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	m_fn.TrimRight('/');
	int i = max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

	if (i >= 0 && i < m_fn.GetLength()-1) {
		m_fn = m_fn.Mid(i+1);
	}

	m_fn = m_fn + _T(".MediaInfo.txt");

	GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_APPLY_NOW)->ShowWindow(SW_HIDE);
	GetDlgItem(IDOK)->SetWindowText(ResStr(IDS_AG_CLOSE));

	CRect r;
	GetDlgItem(ID_APPLY_NOW)->GetWindowRect(&r);
	ScreenToClient(r);
	GetDlgItem(IDOK)->MoveWindow(r);

	r.MoveToX(5);
	r.right += 10;
	m_Button_MI.Create(ResStr(IDS_AG_SAVE_AS), WS_CHILD|BS_PUSHBUTTON|WS_VISIBLE, r, this, IDC_BUTTON_MI);
	m_Button_MI.SetFont(GetFont());
	m_Button_MI.ShowWindow(SW_HIDE);

	GetTabControl()->SetFocus();

	GetWindowRect(&m_rWnd);
	m_nMinCX = m_rWnd.Width();
	m_nMinCY = m_rWnd.Height();

	m_bNeedInit = FALSE;
	GetClientRect(&m_rCrt);
	ScreenToClient(&m_rCrt);

	GetWindowRect (&r);
	ScreenToClient (&r);
	r.right		+= AfxGetAppSettings().iDlgPropX;
	r.bottom	+= AfxGetAppSettings().iDlgPropY;
	MoveWindow (&r);

	CenterWindow();

	ModifyStyle(0, WS_MAXIMIZEBOX);

	const AppSettings& s = AfxGetAppSettings();
	for (int i = 0; i < GetPageCount(); i++) {
		DWORD nID = GetResourceId(i);
		if (nID == s.nLastFileInfoPage) {
			SetActivePage(i);
			break;
		}
	}

	return bResult;
}

void CPPageFileInfoSheet::OnSaveAs()
{
	CFileDialog filedlg (FALSE, _T("*.txt"), m_fn,
						 OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR,
						 _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"), NULL);

	if (filedlg.DoModal() == IDOK) {
		TCHAR bom = (TCHAR)0xFEFF;
		CFile mFile;

		if (mFile.Open(filedlg.GetPathName(), CFile::modeCreate | CFile::modeWrite)) {
			mFile.Write(&bom, sizeof(TCHAR));
			mFile.Write(LPCTSTR(m_mi.MI_Text), m_mi.MI_Text.GetLength()*sizeof(TCHAR));
			mFile.Close();
		}
	}
}

int CALLBACK CPPageFileInfoSheet::XmnPropSheetCallback(HWND hWnd, UINT message, LPARAM lParam)
{
	extern int CALLBACK AfxPropSheetCallback(HWND, UINT message, LPARAM lParam);
	int nRes = AfxPropSheetCallback(hWnd, message, lParam);

	switch (message) {
		case PSCB_PRECREATE:
			((LPDLGTEMPLATE)lParam)->style |= (DS_3DLOOK | DS_SETFONT | WS_THICKFRAME | WS_SYSMENU | DS_MODALFRAME | WS_VISIBLE | WS_CAPTION);
			break;
	}

	return nRes;
}

INT_PTR CPPageFileInfoSheet::DoModal()
{
	m_psh.dwFlags |= PSH_USECALLBACK;
	m_psh.pfnCallback = XmnPropSheetCallback;

	return CPropertySheet::DoModal();
}

void CPPageFileInfoSheet::OnSize(UINT nType, int cx, int cy)
{
	CPropertySheet::OnSize(nType, cx, cy);

	CRect r;

	if (m_bNeedInit) return;

	CTabCtrl *pTab = GetTabControl();
	ASSERT(NULL != pTab && IsWindow(pTab->m_hWnd));

	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	HDWP hDWP = ::BeginDeferWindowPos(5);

	pTab->GetClientRect(&r);
	r.right += dx;
	r.bottom += dy;
	::DeferWindowPos(hDWP, pTab->m_hWnd, NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);

	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if ((pChild->SendMessage(WM_GETDLGCODE) & DLGC_BUTTON) && pChild == GetDlgItem(IDOK)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.top += dy;
			r.bottom += dy;
			r.left+= dx;
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, r.left, r.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
		} else if ((pChild->SendMessage(WM_GETDLGCODE) & DLGC_BUTTON) && pChild == GetDlgItem(IDC_BUTTON_MI)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.top += dy;
			r.bottom += dy;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, r.left, r.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
		} else {
			pChild->GetClientRect(&r);
			r.right += dx;
			r.bottom += dy;
			::DeferWindowPos(hDWP, pChild->m_hWnd, NULL, 0, 0, r.Width(), r.Height(),SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
		}
	}

	::EndDeferWindowPos(hDWP);

	GetWindowRect(&m_rWnd);
}

void CPPageFileInfoSheet::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CPropertySheet::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize.x = m_nMinCX;
	lpMMI->ptMinTrackSize.y = m_nMinCY;
}

void CPPageFileInfoSheet::OnDestroy()
{
	AfxGetAppSettings().nLastFileInfoPage = GetResourceId(GetActiveIndex());

	CPropertySheet::OnDestroy();
}
