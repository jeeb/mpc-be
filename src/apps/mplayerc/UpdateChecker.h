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

#pragma once

#include <afxwin.h>
#include "afxdialogex.h"

struct Version
{
	UINT rev;
	CString version;
	CString url;
};

enum Update_Status
{
	UPDATER_ERROR = -1,
	UPDATER_NEWER_VERSION,
	UPDATER_UPDATE_AVAILABLE
};

class UpdateChecker
{
public:
	static const Version MPC_VERSION;

	UpdateChecker(CString versionFileURL);
	~UpdateChecker(void);

	Update_Status isUpdateAvailable(const Version& currentVersion);
	Update_Status isUpdateAvailable();
	const Version& getLatestVersion() const { return latestVersion; };

private :
	CString versionFileURL;
	Version latestVersion;

	bool parseVersion(const CString& versionStr);
	int compareVersion(const Version& v1, const Version& v2) const;
};

class UpdateCheckerDlg : public CDialog
{
	DECLARE_DYNAMIC(UpdateCheckerDlg)

public:
	UpdateCheckerDlg(Update_Status updateStatus, const Version& latestVersion, CWnd* pParent = NULL);
	virtual ~UpdateCheckerDlg();

	enum { IDD = IDD_UPDATE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg virtual BOOL OnInitDialog();
	afx_msg virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	Update_Status m_updateStatus;
	CString m_text;
	CStatic m_icon;
	CButton m_okButton;
	CButton m_cancelButton;

	CString latestURL;
};
