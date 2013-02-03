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

#include "stdafx.h"
#include "mplayerc.h"
#include "SaveImageDialog.h"
#include "../../DSUtil/WinAPIUtils.h"

// CSaveImageDialog

IMPLEMENT_DYNAMIC(CSaveImageDialog, CFileDialog)
CSaveImageDialog::CSaveImageDialog(
	int quality,
	LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
	LPCTSTR lpszFilter, CWnd* pParentWnd) :
	CFileDialog(FALSE, lpszDefExt, lpszFileName,
				OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR,
				lpszFilter, pParentWnd, 0)
	, m_quality(quality)
{
	if (IsWinVistaOrLater()) {

		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			CString str;

			pfdc->StartVisualGroup(IDS_THUMB_IMAGE_QUALITY, ResStr(IDS_THUMB_IMAGE_QUALITY));
			pfdc->AddText(IDS_THUMB_QUALITY, ResStr(IDS_THUMB_QUALITY));
			str.Format(L"%d", max(70, min(100, m_quality)));
			pfdc->AddEditBox(IDC_EDIT1, str);
			pfdc->EndVisualGroup();

			pfdc->Release();
		}
	} else {
		SetTemplate(0, IDD_SAVEIMAGEDIALOGTEMPL);
	}
}

CSaveImageDialog::~CSaveImageDialog()
{
}

void CSaveImageDialog::DoDataExchange(CDataExchange* pDX)
{
	if (!IsWinVistaOrLater()) {

		DDX_Control(pDX, IDC_SPIN1, m_qualityctrl);
	}

	__super::DoDataExchange(pDX);
}

BOOL CSaveImageDialog::OnInitDialog()
{
	__super::OnInitDialog();

	if (!IsWinVistaOrLater()) {

		m_qualityctrl.SetRange(70, 100);
		m_qualityctrl.SetPos(m_quality);
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CSaveImageDialog, CFileDialog)
END_MESSAGE_MAP()

// CSaveImageDialog message handlers

BOOL CSaveImageDialog::OnFileNameOK()
{
	if (IsWinVistaOrLater()) {
		IFileDialogCustomize* pfdc = GetIFileDialogCustomize();
		if (pfdc) {
			WCHAR* result;

			pfdc->GetEditBoxText(IDC_EDIT1, &result);
			int quality = _wtoi(result);
			if (quality > 0 && quality <= 100) {
				m_quality = quality;
			}
			CoTaskMemFree(result);
	
			pfdc->Release();
		}
	} else {
		m_quality = m_qualityctrl.GetPos();
	}

	return __super::OnFileNameOK();
}
