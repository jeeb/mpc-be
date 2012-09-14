/*
 * $Id: DTSSplitter.h 425 2012-05-30 02:19:44Z Aleksoid $
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

#include <afxtaskdialog.h>

// CSaveTaskDlg dialog

class CSaveTaskDlg : public CTaskDialog
{
	DECLARE_DYNAMIC(CSaveTaskDlg)

private:
	CString m_in, m_out;
	CComPtr<IGraphBuilder> pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaEventEx> pME;
	CComQIPtr<IMediaSeeking> pMS;
	HICON	m_hIcon;
	HWND	m_TaskDlgHwnd;

public:
	CSaveTaskDlg(CString in, CString out);
	virtual ~CSaveTaskDlg();

protected:
	virtual HRESULT OnInit();
	virtual HRESULT OnTimer(_In_ long lTime);

	HRESULT InitFileCopy();
};
