/*
 * Copyright (C) 2013 Sergey "Exodus8" (rusguy6@gmail.com)
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
	char* final = NULL;

	HINTERNET f, s = InternetOpen(L"MPC-BE Update Checker", 0, NULL, NULL, 0);
	if (s) {
		f = InternetOpenUrl(s, versionFileURL, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
		if (f) {
			char *out			= NULL;
			DWORD dwBytesRead	= 0;
			DWORD dataSize		= 0;

			do {
				char buffer[4096];
				if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
					break;
				}

				char *tempData = DNew char[dataSize + dwBytesRead];
				memcpy(tempData, out, dataSize);
				memcpy(tempData + dataSize, buffer, dwBytesRead);
				delete[] out;
				out = tempData;
				dataSize += dwBytesRead;

				if (strstr(out, "dev build ")) {
					break;
				}
			} while (dwBytesRead);

			final = DNew char[dataSize + 1];
			memset(final, 0, dataSize + 1);
			memcpy(final, out, dataSize);
			delete [] out;

			InternetCloseHandle(f);
		}
		InternetCloseHandle(s);
	}

	Update_Status updateAvailable = UPDATER_NEWER_VERSION;

	if (!final || !f || !s) {
		updateAvailable = UPDATER_ERROR;
		return updateAvailable;
	}

	char *str = NULL;
	int t_start = 0, t_stop = 0;

	t_start = strpos(final, "MPC-BE v") + strlen("MPC-BE ");
	final += t_start;
	t_stop = strpos(final, "<");
	str = DNew char[t_stop + 1];
	memset(str, 0, t_stop + 1);
	memcpy(str, final, t_stop);

	CString versionStr = CString(str);

	t_start = strpos(final, "build") + strlen("build ");
	final += t_start;
	t_stop = strpos(final, "<");
	str = DNew char[t_stop + 1];
	memset(str, 0, t_stop + 1);
	memcpy(str, final, t_stop);

	latestVersion.rev = (UINT)atoi(str);

	final -= 160;
	t_start = strpos(final, "http");
	final += t_start;
	t_stop = strpos(final, "\"");
	str = DNew char[t_stop + 1];
	memset(str, 0, t_stop + 1);
	memcpy(str, final, t_stop);

	latestVersion.url = CString(str);

	delete [] str;

	if (!parseVersion(versionStr)) {
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

		latestVersion.version = versionStr;

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
	CString VersionStr;

	switch (updateStatus) {
		case UPDATER_UPDATE_AVAILABLE:
			latestURL = latestVersion.url;
			m_text.Format(IDS_NEW_UPDATE_AVAILABLE, latestVersion.version);
			break;
		case UPDATER_NEWER_VERSION:
			VersionStr.Format(_T("v%u.%u.%u.%u -dev build %u"), MPC_VERSION_MAJOR, MPC_VERSION_MINOR, MPC_VERSION_PATCH, MPC_VERSION_STATUS, MPC_VERSION_REV);
			m_text.Format(IDS_USING_NEWER_VERSION, VersionStr);
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
		ShellExecute(NULL, _T("open"), latestURL, NULL, NULL, SW_SHOWDEFAULT);
	}

	__super::OnOK();
}
