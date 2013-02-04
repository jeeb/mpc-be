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

#pragma once

// CDialogEventHandler

class CDialogEventHandler : public IFileDialogEvents,
									 public IFileDialogControlEvents
{
public:
	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef) {
			delete this;
		}
		return cRef;
	}

	// IFileDialogEvents
	IFACEMETHODIMP OnFileOk(IFileDialog * /* pfd */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnFolderChanging(IFileDialog * /* pfd */, IShellItem * /* psi */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnFolderChange(IFileDialog * /* pfd */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnSelectionChange(IFileDialog * /* pfd */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnShareViolation(IFileDialog * /* pfd */, IShellItem * /* psi */, FDE_SHAREVIOLATION_RESPONSE * /* pResponse */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnTypeChange(IFileDialog * pfd);
	IFACEMETHODIMP OnOverwrite(IFileDialog * /* pfd */, IShellItem * /* psi */ , FDE_OVERWRITE_RESPONSE * /* pResponse */) { return E_NOTIMPL;}

	// IFileDialogControlEvents
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize * /* pfdc */, DWORD /* dwIDCtl */, DWORD /* dwIDItem */)  { return E_NOTIMPL; }
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize * /* pfdc */, DWORD /* pIDControl */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize * /* pfdc */, DWORD /* dwIDCtl */, BOOL /* bChecked */) { return E_NOTIMPL; }
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize * /* pfdc */, DWORD /* dwIDCtl */) { return E_NOTIMPL; }

	CDialogEventHandler() : _cRef(1) { };
private:
	~CDialogEventHandler() { };
	long _cRef;
};
