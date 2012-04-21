// ConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigDlg.h"
#include <winuser.h>


// CConfigDlg dialog

IMPLEMENT_DYNAMIC(CConfigDlg, CDialog)

CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
}

CConfigDlg::~CConfigDlg()
{
}

void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MPCCOMBO, m_MPCPath);
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CConfigDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CConfigDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CConfigDlg message handlers

void CConfigDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CRegKey key;
	if(ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, _T("Software\\MPC-BE\\ShellExt")))
	{
		CString path;
		m_MPCPath.GetLBText(m_MPCPath.GetCurSel(), path);
		key.SetStringValue(_T("MpcPath"), path);
	}

	OnOK();
}

void CConfigDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

BOOL CConfigDlg::OnInitDialog()
{
	__super::OnInitDialog();

	SetClassLongPtr(GetDlgItem(IDOK)->m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_HAND));
	SetClassLongPtr(GetDlgItem(IDCANCEL)->m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_HAND));
	SetClassLongPtr(GetDlgItem(IDC_MPCCOMBO)->m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_HAND));

	CRegKey key;
	TCHAR path_buff[MAX_PATH];
	ULONG len = sizeof(path_buff);

	if(ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, _T("Software\\MPC-BE")))
	{
		if(ERROR_SUCCESS == key.QueryStringValue(_T("ExePath"), path_buff, &len) && !CString(path_buff).Trim().IsEmpty())
		{
			m_MPCPath.AddString(path_buff);
		}
	}
#ifdef _WIN64
	if(ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, _T("Software\\Wow6432Node\\MPC-BE")))
	{
		len = sizeof(path_buff);
		if(ERROR_SUCCESS == key.QueryStringValue(_T("ExePath"), path_buff, &len) && !CString(path_buff).Trim().IsEmpty())
		{
			m_MPCPath.AddString(path_buff);
		}
	}
#endif

	CString mpc_path = _T("");
	if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\MPC-BE\\ShellExt")))
	{
		len = sizeof(path_buff);
		if(ERROR_SUCCESS == key.QueryStringValue(_T("MpcPath"), path_buff, &len) && !CString(path_buff).Trim().IsEmpty())
		{
			mpc_path = CString(path_buff).Trim();
		}
	}

	m_MPCPath.SetCurSel(mpc_path.IsEmpty() ? 0 : m_MPCPath.FindStringExact(0, mpc_path));

	return TRUE;
}
