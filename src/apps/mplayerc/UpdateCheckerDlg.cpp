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
#include "UpdateCheckerDlg.h"

IMPLEMENT_DYNAMIC(UpdateCheckerDlg, CDialog)

UpdateCheckerDlg::UpdateCheckerDlg(Update_Status updateStatus, const Version& latestVersion, CWnd* pParent)
	: CDialog(UpdateCheckerDlg::IDD, pParent), m_updateStatus(updateStatus)
{
	switch (updateStatus) {
		case UPDATER_UPDATE_AVAILABLE:
			m_text.Format(IDS_NEW_UPDATE_AVAILABLE, latestVersion.major, latestVersion.minor, latestVersion.status, latestVersion.patch);
			break;
		case UPDATER_LATEST_STABLE:
			m_text.LoadString(IDS_USING_LATEST_STABLE);
			break;
		case UPDATER_NEWER_VERSION:
			m_text.Format(IDS_USING_NEWER_VERSION, UpdateChecker::MPC_VERSION.major, UpdateChecker::MPC_VERSION.minor, UpdateChecker::MPC_VERSION.status, UpdateChecker::MPC_VERSION.patch,
						latestVersion.major, latestVersion.minor, latestVersion.status, latestVersion.patch);
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
		case UPDATER_LATEST_STABLE:
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
//		ShellExecute(NULL, _T("open"), _T("http://sourceforge.net/p/mpcbe/download-media-player-classic-hc.html"), NULL, NULL, SW_SHOWNORMAL);
		ShellExecute(NULL, _T("open"), _T("http://www.xvidvideo.ru/media-player-classic-home-cinema-x86-x64/"), NULL, NULL, SW_SHOWNORMAL);
	}

	__super::OnOK();
}
