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
#include "resource.h"
#include "MPCBEShellExt_i.h"
#include "dllmain.h"
#include "configdlg.h"

// config dialog
STDAPI DllConfig(void)
{
#ifdef _WIN64
	CConfigDlg dlg;
	dlg.DoModal();
#endif

	return S_OK;
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _AtlModule.GetLockCount()==0) ? S_OK : S_FALSE;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer - Adds entries to the system registry
#define	IS_KEY_LEN	256
STDAPI DllRegisterServer(void)
{
	OLECHAR		strWideCLSID[50];
	CRegKey		key;
	HRESULT hr = _AtlModule.DllRegisterServer();

	if (SUCCEEDED(hr)) {
		TCHAR path_buff[MAX_PATH];
		ULONG len = sizeof(path_buff);

		bool found_registry_path = false;
		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\MPC-BE\\ShellExt"))) {
			if (ERROR_SUCCESS == key.QueryStringValue(_T("MpcPath"), path_buff, &len) && !CString(path_buff).Trim().IsEmpty()) {
				found_registry_path = true;
			}
		}

		if (!found_registry_path) {
			DllConfig();
		}

		if (::StringFromGUID2(CLSID_MPCBEContextMenu, strWideCLSID, 50) > 0) {
			key.SetValue(HKEY_CLASSES_ROOT, _T("directory\\shellex\\ContextMenuHandlers\\MPCBEShellExt\\"), strWideCLSID);

			CRegKey reg;
			if (reg.Open(HKEY_CLASSES_ROOT, NULL, KEY_READ) == ERROR_SUCCESS) {
				DWORD dwIndex = 0;
				DWORD cbName = IS_KEY_LEN;
				TCHAR szSubKeyName[IS_KEY_LEN];
				LONG lRet;

				while ((lRet = reg.EnumKey(dwIndex, szSubKeyName, &cbName)) != ERROR_NO_MORE_ITEMS) {
					if (lRet == ERROR_SUCCESS) {
						CString key_name = szSubKeyName;
						if(!key_name.Find(_T("mpc-be.")))
						{
							key_name.Append(_T("\\shellex\\ContextMenuHandlers\\MPCBEShellExt\\"));
							key.SetValue(HKEY_CLASSES_ROOT, key_name, strWideCLSID);
						}
					}
					dwIndex++;
					cbName = IS_KEY_LEN;
				}
				reg.Close();
			}

		}
	}

	return hr;
}

// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	CRegKey		key;
	HRESULT hr = _AtlModule.DllUnregisterServer();

	if (SUCCEEDED(hr)) {
		if (key.Open(HKEY_CLASSES_ROOT, _T("directory\\shellex\\ContextMenuHandlers\\")) == ERROR_SUCCESS) {
			key.DeleteSubKey(_T("MPCBEShellExt"));
		}

		CRegKey reg;
		if (reg.Open(HKEY_CLASSES_ROOT, NULL, KEY_READ) == ERROR_SUCCESS) {
			DWORD dwIndex = 0;
			DWORD cbName = IS_KEY_LEN;
			TCHAR szSubKeyName[IS_KEY_LEN];
			LONG lRet;

			while ((lRet = reg.EnumKey(dwIndex, szSubKeyName, &cbName)) != ERROR_NO_MORE_ITEMS) {
				if (lRet == ERROR_SUCCESS) {
					CString key_name = szSubKeyName;
					if(!key_name.Find(_T("mpc-be."))) {
						key_name.Append(_T("\\shellex\\ContextMenuHandlers\\"));

						if (key.Open(HKEY_CLASSES_ROOT, key_name) == ERROR_SUCCESS) {
							key.DeleteValue(NULL);
							key.DeleteSubKey(_T("MPCBEShellExt"));
						}
					}
				}
				dwIndex++;
				cbName = IS_KEY_LEN;
			}
			reg.Close();
		}
	}

	return hr;
}

// DllInstall - Adds/Removes entries to the system registry per user
//              per machine.	
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = E_FAIL;
    static const wchar_t szUserSwitch[] = _T("user");

    if (pszCmdLine != NULL) {
    	if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0) {
    		AtlSetPerUserRegistration(true);
    	}
    }

    if (bInstall) {	
    	hr = DllRegisterServer();
    	if (FAILED(hr)) {	
    		DllUnregisterServer();
    	}
    } else {
    	hr = DllUnregisterServer();
    }

    return hr;
}
