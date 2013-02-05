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
#include "DialogEventHandler.h"

// CDialogEventHandler

IFACEMETHODIMP CDialogEventHandler::OnTypeChange(IFileDialog *pfd)
{
	IFileSaveDialog *pfsd;
	HRESULT hr = pfd->QueryInterface(&pfsd);
	if (SUCCEEDED(hr)) {
		UINT uIndex;
		hr = pfsd->GetFileTypeIndex(&uIndex);   // index of current file-type
		if (SUCCEEDED(hr)) {

			IFileDialogCustomize* pfdc;
			hr = pfd->QueryInterface(&pfdc);
			if (SUCCEEDED(hr)) {

				switch (uIndex)
				{
					case 1:
					case 5:
					case 6:
						pfdc->SetControlState(IDS_THUMB_IMAGE_QUALITY, CDCS_INACTIVE);
						pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_INACTIVE);
						pfdc->SetControlState(IDC_EDIT4, CDCS_INACTIVE);

						pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_INACTIVE);
						pfdc->SetControlState(IDC_EDIT5, CDCS_INACTIVE);
						break;
					case 2:
					case 4:
						pfdc->SetControlState(IDS_THUMB_IMAGE_QUALITY, CDCS_ENABLEDVISIBLE);
						pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_ENABLEDVISIBLE);
						pfdc->SetControlState(IDC_EDIT4, CDCS_ENABLEDVISIBLE);

						pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_INACTIVE);
						pfdc->SetControlState(IDC_EDIT5, CDCS_INACTIVE);
						break;
					case 3:
						pfdc->SetControlState(IDS_THUMB_IMAGE_QUALITY, CDCS_ENABLEDVISIBLE);
						pfdc->SetControlState(IDS_THUMB_QUALITY, CDCS_INACTIVE);
						pfdc->SetControlState(IDC_EDIT4, CDCS_INACTIVE);

						pfdc->SetControlState(IDS_THUMB_LEVEL, CDCS_ENABLEDVISIBLE);
						pfdc->SetControlState(IDC_EDIT5, CDCS_ENABLEDVISIBLE);
						break;
					default :
						break;
				}
				pfdc->Release();
			}
		}
		pfsd->Release();
	}
	return hr;
}
