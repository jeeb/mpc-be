/*
 * $Id$
 *
 * (C) 2013 see Authors.txt
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
#include "UpdateChecker.h"
#include "PlayerYouTube.h"

const Version UpdateChecker::MPC_VERSION = {MPC_VERSION_REV};

UpdateChecker::UpdateChecker(CString versionFileURL)
	: versionFileURL(versionFileURL)
{
}

UpdateChecker::~UpdateChecker(void)
{
}

Update_Status UpdateChecker::isUpdateAvailable(const Version& currentVersion)
{
	CString Str = PlayerYouTubeGetTitle(versionFileURL);

	Str.Replace(_T("WebSVN"), _T(""));
	Str.Replace(_T("MPC-BE Team"), _T(""));
	Str.Replace(_T("Log"), _T(""));
	Str.Replace(_T("Rev"), _T(""));
	Str.Replace(_T("-"), _T(""));
	Str.Replace(_T("/"), _T(""));
	Str.Replace(_T(" "), _T(""));
	Str.Replace(_T("\r"), _T(""));
	Str.Replace(_T("\n"), _T(""));

	Update_Status updateAvailable = UPDATER_NEWER_VERSION;

	if (!parseVersion(latestVersionStr)) {
		updateAvailable = UPDATER_ERROR;
	} else {
		if (compareVersion(currentVersion, latestVersion)) {
			updateAvailable = UPDATER_UPDATE_AVAILABLE;
		}
	}

	return updateAvailable;
}

Update_Status UpdateChecker::isUpdateAvailable()
{
	return isUpdateAvailable(MPC_VERSION);
}

bool UpdateChecker::parseVersion(const CString& versionStr)
{
	bool success = false;

	if (!versionStr.IsEmpty()) {

		latestVersion.rev = (UINT)_ttoi((LPCTSTR)versionStr);

		success = true;
	}

	return success;
}

int UpdateChecker::compareVersion(const Version& v1, const Version& v2) const
{
	if (v2.rev > v1.rev) {
		return 1;
	} else {
		return 0;
	}
}

IMPLEMENT_DYNAMIC(UpdateCheckerDlg, CDialog)

UpdateCheckerDlg::UpdateCheckerDlg(Update_Status updateStatus, const Version& latestVersion, CWnd* pParent)
	: CDialog(UpdateCheckerDlg::IDD, pParent), m_updateStatus(updateStatus)
{
	switch (updateStatus) {
		case UPDATER_UPDATE_AVAILABLE:
			m_text.Format(IDS_NEW_UPDATE_AVAILABLE, latestVersion.rev);
			break;
		case UPDATER_NEWER_VERSION:
			m_text.Format(IDS_USING_NEWER_VERSION, UpdateChecker::MPC_VERSION.rev);
			break;
		case UPDATER_ERROR:
			m_text.LoadString(IDS_UPDATE_ERROR);
			break;
		default:
			ASSERT(0);
	}
}

UpdateCheckerDlg::~UpdateCheckerDlg()
{
}

void UpdateCheckerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_UPDATE_DLG_TEXT, m_text);
	DDX_Control(pDX, IDC_UPDATE_ICON, m_icon);
	DDX_Control(pDX, IDOK, m_okButton);
	DDX_Control(pDX, IDCANCEL, m_cancelButton);
}

BEGIN_MESSAGE_MAP(UpdateCheckerDlg, CDialog)
END_MESSAGE_MAP()

BOOL UpdateCheckerDlg::OnInitDialog()
{
	__super::OnInitDialog();

	switch (m_updateStatus) {
		case UPDATER_UPDATE_AVAILABLE:
			m_icon.SetIcon(LoadIcon(NULL, IDI_QUESTION));
			break;
		case UPDATER_NEWER_VERSION:
		case UPDATER_ERROR:
			m_icon.SetIcon(LoadIcon(NULL, (m_updateStatus == UPDATER_ERROR) ? IDI_WARNING : IDI_INFORMATION));
			m_okButton.ShowWindow(SW_HIDE);
			m_cancelButton.SetWindowText(ResStr(IDS_UPDATE_CLOSE));
			m_cancelButton.SetFocus();
			break;
		default:
			ASSERT(0);
	}

	return TRUE;
}

void UpdateCheckerDlg::OnOK()
{
	if (m_updateStatus == UPDATER_UPDATE_AVAILABLE) {
		ShellExecute(NULL, _T("open"), _T("http://dev.mpc-next.ru/index.php?board=29.0"), NULL, NULL, SW_SHOWDEFAULT);
	}

	__super::OnOK();
}
