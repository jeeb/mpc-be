/*
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

// CSaveImageDialog

class CSaveImageDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CSaveImageDialog)

public:
	CSaveImageDialog(
		int quality, int levelPNG,
		LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL,
		LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL);
	virtual ~CSaveImageDialog();

protected:
	DECLARE_MESSAGE_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();

	HRESULT _CDialogEventHandler_CreateInstance(REFIID riid, void **ppv);

public:
	int m_quality, m_levelPNG;
	CSpinButtonCtrl m_qualityctrl;
};

// CSaveThumbnailsDialog

class CSaveThumbnailsDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CSaveThumbnailsDialog)

public:
	CSaveThumbnailsDialog(
		int rows, int cols, int width, int quality, int levelPNG,
		LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL,
		LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL);
	virtual ~CSaveThumbnailsDialog();

protected:
	DECLARE_MESSAGE_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();

	HRESULT _CDialogEventHandler_CreateInstance(REFIID riid, void **ppv);

public:
	int m_rows, m_cols, m_width, m_quality, m_levelPNG;
	CSpinButtonCtrl m_rowsctrl, m_colsctrl, m_widthctrl, m_qualityctrl;
};

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
