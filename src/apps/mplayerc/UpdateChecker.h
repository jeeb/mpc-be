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

#pragma once

#include <afxwin.h>

struct Version
{
	UINT major;
	UINT minor;
	UINT status;
	UINT patch;
};

enum Update_Status
{
	UPDATER_ERROR = -1,
	UPDATER_LATEST_STABLE,
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
