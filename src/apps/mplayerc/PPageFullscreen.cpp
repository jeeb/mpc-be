/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "PPageFullscreen.h"
#include "../../DSUtil/WinAPIUtils.h"
#include "MultiMonitor.h"

// CPPagePlayer dialog

IMPLEMENT_DYNAMIC(CPPageFullscreen, CPPageBase)
CPPageFullscreen::CPPageFullscreen()
	: CPPageBase(CPPageFullscreen::IDD, CPPageFullscreen::IDD)
	, m_launchfullscreen(FALSE)
	, m_fSetFullscreenRes(0)
	, m_fSetDefault(FALSE)
	, m_fSetGlobal(FALSE)
	, m_iShowBarsWhenFullScreen(FALSE)
	, m_nShowBarsWhenFullScreenTimeOut(0)
	, m_fExitFullScreenAtTheEnd(FALSE)
	, m_fExitFullScreenAtFocusLost(FALSE)
	, m_fRestoreResAfterExit(TRUE)
	, m_list(0)
	, m_iSel(-1)
{
	memset(m_iSeldm, -1, sizeof(m_iSeldm));
}

CPPageFullscreen::~CPPageFullscreen()
{
}

void CPPageFullscreen::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_launchfullscreen);
	DDX_Check(pDX, IDC_CHECK2, m_fSetFullscreenRes);
	DDX_Check(pDX, IDC_CHECK3, m_fSetDefault);
	DDX_Check(pDX, IDC_CHECK7, m_fSetGlobal);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iMonitorType);
	DDX_Control(pDX, IDC_COMBO1, m_iMonitorTypeCtrl);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Check(pDX, IDC_CHECK4, m_iShowBarsWhenFullScreen);
	DDX_Text(pDX, IDC_EDIT1, m_nShowBarsWhenFullScreenTimeOut);
	DDX_Check(pDX, IDC_CHECK5, m_fExitFullScreenAtTheEnd);
	DDX_Check(pDX, IDC_CHECK6, m_fExitFullScreenAtFocusLost);
	DDX_Control(pDX, IDC_SPIN1, m_nTimeOutCtrl);
	DDX_Check(pDX, IDC_RESTORERESCHECK, m_fRestoreResAfterExit);
}

BEGIN_MESSAGE_MAP(CPPageFullscreen, CPPageBase)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnUpdateFullScrCombo)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnLvnItemchangedList1)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST1, OnBeginlabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, OnEndlabeleditList)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnNMClickList1)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST1, OnCustomdrawList)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckChangeList)
	ON_UPDATE_COMMAND_UI(IDC_LIST1, OnUpdateList)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateApplyDefault)

	ON_COMMAND(IDC_CHECK2, OnUpdateSetFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_CHECK1, OnUpdateLaunchfullscreen)
	ON_UPDATE_COMMAND_UI(IDC_CHECK4, OnUpdateShowBarsWhenFullScreen)
	ON_UPDATE_COMMAND_UI(IDC_CHECK5, OnUpdateExitFullScreenAtTheEnd)
	ON_UPDATE_COMMAND_UI(IDC_CHECK6, OnUpdateExitFullScreenAtFocusLost)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateShowBarsWhenFullScreenTimeOut)
	ON_UPDATE_COMMAND_UI(IDC_COMBO1, OnUpdateFullScrComboCtrl)
	ON_UPDATE_COMMAND_UI(IDC_CHECK7, OnUpdateSetGlobal)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateStatic1)
	ON_UPDATE_COMMAND_UI(IDC_STATIC2, OnUpdateStatic2)

	ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_RESTORERESCHECK, OnUpdateRestoreRes)
	ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateRemove)
	ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateAdd)
	ON_BN_CLICKED(IDC_BUTTON3, OnMoveUp)
	ON_BN_CLICKED(IDC_BUTTON4, OnMoveDown)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateUp)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateDown)
END_MESSAGE_MAP()

// CPPagePlayer message handlers

BOOL CPPageFullscreen::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

	m_launchfullscreen					= s.fLaunchfullscreen;
	m_AutoChangeFullscrRes				= s.AutoChangeFullscrRes;
	m_fSetDefault						= s.AutoChangeFullscrRes.bApplyDefault;
	m_fSetGlobal						= s.AutoChangeFullscrRes.bSetGlobal;
	m_f_hmonitor						= s.strFullScreenMonitor;
	m_f_hmonitorID						= s.strFullScreenMonitorID;
	m_iShowBarsWhenFullScreen			= s.fShowBarsWhenFullScreen;
	m_nShowBarsWhenFullScreenTimeOut	= s.nShowBarsWhenFullScreenTimeOut;
	m_nTimeOutCtrl.SetRange(-1, 10);
	m_fExitFullScreenAtTheEnd			= s.fExitFullScreenAtTheEnd;
	m_fExitFullScreenAtFocusLost		= s.fExitFullScreenAtFocusLost;
	m_fRestoreResAfterExit				= s.fRestoreResAfterExit;

	CString str;
	m_iMonitorType = 0;
	CMonitor curmonitor;
	CMonitors monitors;
	CString strCurMon;

 	m_iMonitorTypeCtrl.AddString(ResStr(IDS_FULLSCREENMONITOR_CURRENT));
	m_MonitorDisplayNames.Add(_T("Current"));
	m_MonitorDeviceName.Add(_T("Current"));
	curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
	curmonitor.GetName(strCurMon);
	if(m_f_hmonitor.IsEmpty()) {
		m_f_hmonitor = _T("Current");
		m_iMonitorType = 0;
 	}

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	DWORD dev = 0; // device index
	int id = 0;
	CString str2;
	CString DeviceID;
	CString strMonID;
	while (EnumDisplayDevices(0, dev, &dd, 0)) {
        DISPLAY_DEVICE ddMon;
        ZeroMemory(&ddMon, sizeof(ddMon));
        ddMon.cb = sizeof(ddMon);
        DWORD devMon = 0;

        while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
			if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE && !(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
				DeviceID.Format (L"%s", ddMon.DeviceID);
                strMonID = DeviceID = DeviceID.Mid (8, DeviceID.Find (L"\\", 9) - 8);
				str.Format(L"%s", ddMon.DeviceName);
				str = str.Left(12);
				if (str == strCurMon) {
					m_iMonitorTypeCtrl.AddString(str.Mid(4, 7) + _T("( ") + str.Right(1) + _T(" ) ") + _T("- [id: ") + DeviceID + _T(" *") +  ResStr(IDS_FULLSCREENMONITOR_CURRENT) + _T("] - ") + ddMon.DeviceString);
					m_MonitorDisplayNames[0] = _T("Current") + strMonID;
					m_MonitorDeviceName[0] = str;

					if(m_f_hmonitor == _T("Current") && m_AutoChangeFullscrRes.bEnabled > 0) {
						m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
						m_f_hmonitor = strCurMon;
					}
					iCurMon = m_iMonitorTypeCtrl.GetCount()-1;
				} else {
					m_iMonitorTypeCtrl.AddString(str.Mid(4, 7) + _T("( ") + str.Right(1) + _T(" ) ") + _T("- [id: ") + DeviceID + _T("] - ") + ddMon.DeviceString);
				}
				m_MonitorDisplayNames.Add(str + strMonID);
				m_MonitorDeviceName.Add(str);
				if(m_iMonitorType == 0 && m_f_hmonitor == str) {
					m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
				}

				if (m_f_hmonitorID == strMonID  && m_f_hmonitor != _T("Current")) {
					id = m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
					str2 = str;
				}
            }
            devMon++;
            ZeroMemory(&ddMon, sizeof(ddMon));
            ddMon.cb = sizeof(ddMon);
        }
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
        dev++;
	}

	if(m_iMonitorTypeCtrl.GetCount() > 2) {
		if (m_MonitorDisplayNames[m_iMonitorType] != m_f_hmonitor + m_f_hmonitorID) {
			if ( id > 0 ) {
				m_iMonitorType = id;
				m_f_hmonitor = str2;
 			}
 		}
	 	GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
 	} else {
		if(m_AutoChangeFullscrRes.bEnabled == false)  {
			m_iMonitorType = 0;
		} else {
			m_iMonitorType = 1;
 		}
 		GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);
 	}

	if (m_AutoChangeFullscrRes.bEnabled == false && (m_MonitorDisplayNames[m_iMonitorType]).Left(7) == _T("Current")){
		m_f_hmonitor = _T("Current");
 	}

	m_list.SetExtendedStyle(m_list.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER
							|LVS_EX_GRIDLINES|LVS_EX_BORDERSELECT|LVS_EX_ONECLICKACTIVATE|LVS_EX_CHECKBOXES|LVS_EX_FLATSB);
	m_list.InsertColumn(COL_Z, ResStr(IDS_PPAGE_FS_CLN_ON_OFF), LVCFMT_LEFT, 60);
	m_list.InsertColumn(COL_VFR_F, ResStr(IDS_PPAGE_FS_CLN_FROM_FPS), LVCFMT_RIGHT, 60);
	m_list.InsertColumn(COL_VFR_T, ResStr(IDS_PPAGE_FS_CLN_TO_FPS), LVCFMT_RIGHT, 60);
	m_list.InsertColumn(COL_SRR, ResStr(IDS_PPAGE_FS_CLN_DISPLAY_MODE), LVCFMT_LEFT, 135);

	ModesUpdate();
	UpdateData(FALSE);

	return TRUE;
}

void CPPageFullscreen::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	*pResult = CDRF_DODEFAULT;

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage ) {
		*pResult = CDRF_NOTIFYITEMDRAW;
	} else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage ) {
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	} else if ( (CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage ) {
		COLORREF crText, crBkgnd;
		m_AutoChangeFullscrRes.bEnabled = !!m_fSetFullscreenRes;
		if (m_AutoChangeFullscrRes.bEnabled == false) {
			crText = RGB(128,128,128);
			crBkgnd = RGB(240, 240, 240);
 		} else {
 			crText = RGB(0,0,0);
			crBkgnd = RGB(255,255,255);
 		}
		if (m_list.GetCheck(pLVCD->nmcd.dwItemSpec) == false) {
			crText = RGB(128,128,128);
		}
 		pLVCD->clrText = crText;
		pLVCD->clrTextBk = crBkgnd;
 		*pResult = CDRF_DODEFAULT;
	}
}

BOOL CPPageFullscreen::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();
	m_AutoChangeFullscrRes.bEnabled = !!m_fSetFullscreenRes;
	if (m_fSetFullscreenRes == 2) m_AutoChangeFullscrRes.bEnabled = 2;
	for (int i = 0; i < MaxFpsCount; i++) {
		int n = m_iSeldm[i];
		if (n >= 0 && (size_t)n < m_dms.GetCount() && i < m_list.GetItemCount()) {
			m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes = m_dms[n];
			m_AutoChangeFullscrRes.dmFullscreenRes[i].fChecked = !!m_list.GetCheck(i);
			m_AutoChangeFullscrRes.dmFullscreenRes[i].fIsData = true;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_from = wcstod(m_list.GetItemText(i, COL_VFR_F), NULL);
			m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_to = wcstod(m_list.GetItemText(i, COL_VFR_T), NULL);
		} else {
			m_AutoChangeFullscrRes.dmFullscreenRes[i].fIsData = false;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_from = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_to = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].fChecked = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.bpp = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.dmDisplayFlags = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.freq = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.fValid = 0;
			m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.size = 0;
		}
	}

	m_AutoChangeFullscrRes.bApplyDefault	= !!m_fSetDefault;
	m_AutoChangeFullscrRes.bSetGlobal		= !!m_fSetGlobal;
	s.AutoChangeFullscrRes					= m_AutoChangeFullscrRes;
	s.fLaunchfullscreen						= !!m_launchfullscreen;

	CString str;
	CString strCurFSMonID;
	CString strCurMon;
	str = m_MonitorDisplayNames[m_iMonitorType];
	if (str.GetLength() == 14) { m_f_hmonitor = str.Left(7); }
	if (str.GetLength() == 19) { m_f_hmonitor = str.Left(12); }
	m_f_hmonitorID = str.Right(7);
	if (m_AutoChangeFullscrRes.bEnabled > 0 && m_f_hmonitor == _T("Current")) {
		CMonitors monitors;
		CMonitor curmonitor;
		curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
		curmonitor.GetName(strCurMon);
		m_f_hmonitor = strCurMon;
	}
	if (m_AutoChangeFullscrRes.bEnabled == false && m_f_hmonitor == _T("Current")) {
		m_f_hmonitor = _T("Current");
	}
 	s.strFullScreenMonitor					= m_f_hmonitor;
	s.strFullScreenMonitorID				= m_f_hmonitorID;

	s.fShowBarsWhenFullScreen				= !!m_iShowBarsWhenFullScreen;
	s.nShowBarsWhenFullScreenTimeOut		= m_nShowBarsWhenFullScreenTimeOut;
	s.fExitFullScreenAtTheEnd				= !!m_fExitFullScreenAtTheEnd;
	s.fExitFullScreenAtFocusLost			= !!m_fExitFullScreenAtFocusLost;
	s.fRestoreResAfterExit					= !!m_fRestoreResAfterExit;

	return __super::OnApply();
}

void CPPageFullscreen::OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;
	if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem == COL_SRR) {
	}
	*pResult = 0;
}

void CPPageFullscreen::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem >= 0 && pNMLV->iSubItem == COL_SRR) {
	}
	*pResult = 0;
}

void CPPageFullscreen::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;
	*pResult = FALSE;
	if (pItem->iItem < 0) {
		return;
	}
	*pResult = TRUE;
}

void CPPageFullscreen::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;
	*pResult = FALSE;
	if (pItem->iItem < 0) {
		return;
	}
	CAtlList<CString> sl1;
	CMonitors monitors;
	CString strModes;
	switch (pItem->iSubItem) {
		case COL_SRR:
			sl1.RemoveAll();
			for (int i=0; (size_t)i<sl.GetCount(); i++) {
				sl1.AddTail(sl[i]);
				if (m_list.GetItemText(pItem->iItem, COL_SRR) == sl[i]) {
					m_iSel = i;
				}
			}
			m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, sl1, m_iSel);
			break;
		case COL_VFR_F:
		case COL_VFR_T:
			if (pItem->iItem >= 0) {
				m_list.ShowInPlaceFloatEdit(pItem->iItem, pItem->iSubItem);
				//CEdit* pFloatEdit = (CEdit*)m_list.GetDlgItem(IDC_EDIT1);
			}
			break;
	}
	m_list.RedrawWindow();
	*pResult = TRUE;
}

void CPPageFullscreen::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;
	*pResult = FALSE;
	if (!m_list.m_fInPlaceDirty) {
		return;
	}
	if (pItem->iItem < 0) {
		return;
	}
	switch (pItem->iSubItem) {
		case COL_SRR:
			if (pItem->lParam >= 0) {
				m_iSeldm[pItem->iItem] = m_iSel = pItem->lParam;
				m_list.SetItemText(pItem->iItem, pItem->iSubItem, pItem->pszText);
			}
			break;
		case COL_VFR_F:
		case COL_VFR_T:
			if (pItem->pszText) {
				CString str = pItem->pszText;
				int dotpos = str.Find('.');
				if (dotpos >= 0 && str.GetLength() - dotpos > 4) {
					str.Truncate(dotpos + 4);
				}
				double f = min(max(_tstof(str), 1.0), 125.999);
				str.Format(_T("%.3f"), f);
				m_list.SetItemText(pItem->iItem, pItem->iSubItem, str);
			}
			break;
	}

	*pResult = TRUE;

	if (*pResult) {
		SetModified();
	}
}

void CPPageFullscreen::OnCheckChangeList()
{
	SetModified();
}

void CPPageFullscreen::OnUpdateList(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnUpdateApplyDefault(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2)  && m_fSetFullscreenRes != 2);
}

void CPPageFullscreen::OnUpdateSetFullscreenRes()
{
	CMonitor curmonitor;
	CMonitors monitors;
	CString strCurMon;
	curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
	curmonitor.GetName(strCurMon);
	if (m_iMonitorTypeCtrl.GetCurSel() == 0) {
		m_iMonitorTypeCtrl.SetCurSel(iCurMon);
		m_f_hmonitor = strCurMon;
	}

	SetModified();
}

void CPPageFullscreen::OnUpdateRestoreRes(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && m_fSetFullscreenRes != 2);
}

void CPPageFullscreen::OnUpdateFullScrComboCtrl(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnUpdateSetGlobal(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_fSetFullscreenRes == 2);
}

void CPPageFullscreen::OnUpdateLaunchfullscreen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateShowBarsWhenFullScreen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateShowBarsWhenFullScreenTimeOut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateExitFullScreenAtTheEnd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateExitFullScreenAtFocusLost(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateStatic1(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateStatic2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateFullScrCombo()
{
	CMonitors monitors;
	CMonitor curmonitor;
	CString str, strCurMonID, strCurMon;
	str = m_MonitorDisplayNames[m_iMonitorTypeCtrl.GetCurSel()];
	if (str.GetLength() == 14) { m_f_hmonitor = str.Left(7); }
	if (str.GetLength() == 19) { m_f_hmonitor = str.Left(12); }
	strCurMonID = str.Right(7);
	curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
	curmonitor.GetName(strCurMon);

	if (m_f_hmonitor == _T("Current") && m_AutoChangeFullscrRes.bEnabled > 0) {
		m_f_hmonitor = strCurMon;
	}

	if (m_f_hmonitor != _T("Current") && m_f_hmonitor != strCurMon) {
 		m_AutoChangeFullscrRes.bEnabled = false;
 	}

	ModesUpdate();
	SetModified();
}

void CPPageFullscreen::OnUpdateTimeout(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_iShowBarsWhenFullScreen);
}

void CPPageFullscreen::ModesUpdate()
{
	AppSettings& s = AfxGetAppSettings();
	CMonitors monitors;

	m_fSetFullscreenRes = m_AutoChangeFullscrRes.bEnabled;
	if (m_AutoChangeFullscrRes.bEnabled == 2) m_fSetFullscreenRes = 2;
	CString sl2[MaxFpsCount];
	dispmode dm,  dmtoset[MaxFpsCount];

	int i0;

	CString str, strCurMon, strModes;
	CString strCur;
	GetCurDispModeString(strCur);

	CString strDevice = m_MonitorDeviceName[m_iMonitorType];
	CString MonitorName;
	UINT16 MonitorHorRes, MonitorVerRes;
	ReadDisplay(strDevice, &MonitorName, &MonitorHorRes, &MonitorVerRes);

	str = m_MonitorDisplayNames[m_iMonitorType];
	if (str.GetLength() == 14) { m_f_hmonitor = str.Left(7); }
	if (str.GetLength() == 19) { m_f_hmonitor = str.Left(12); }
	m_f_hmonitorID = str.Right(7);
	if ( s.strFullScreenMonitorID != m_f_hmonitorID) {
		m_AutoChangeFullscrRes.bEnabled = false;
	}

	int iNoData = 0;
	for (int i=0; i<MaxFpsCount; i++) {
		dmtoset[i] = m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes;
		if (m_AutoChangeFullscrRes.dmFullscreenRes[i].fIsData == true) {
			iNoData++;
		}
	}

	//if (!m_AutoChangeFullscrRes.bEnabled
	if (s.strFullScreenMonitorID != m_f_hmonitorID
			|| m_AutoChangeFullscrRes.dmFullscreenRes[0].dmFSRes.freq <0
			|| m_AutoChangeFullscrRes.dmFullscreenRes[0].fIsData == false) {
		GetCurDispMode(dmtoset[0],m_f_hmonitor);
		for (int i=1; i<MaxFpsCount; i++) {
			dmtoset[i] = dmtoset[0];
		}
	}
	m_list.DeleteAllItems();
	m_dms.RemoveAll();
	sl.RemoveAll();
	for (int i=1; i<MaxFpsCount; i++) {
		sl2[i] = _T("");
	}
	memset(m_iSeldm, -1, sizeof(m_iSeldm));
	m_iSel=-1;

	for (int i = 0, m = 0, ModeExist = true;  ; i++) {
		ModeExist = GetDispMode(i, dm, m_f_hmonitor);
		if (!ModeExist) {
			break;
		}
		if (dm.bpp != 32 || dm.size.cx < 640) {
			continue; // skip low resolution and non 32bpp mode
		}

		if ((MonitorHorRes && MonitorVerRes)
				&& (MonitorHorRes != dm.size.cx)
				&& (MonitorVerRes != dm.size.cy)) {
			continue;
		}

		int j = 0;
		while (j < m) {
			if (dm.bpp                == m_dms[j].bpp &&
					dm.dmDisplayFlags == m_dms[j].dmDisplayFlags &&
					dm.freq           == m_dms[j].freq &&
					dm.fValid         == m_dms[j].fValid &&
					dm.size           == m_dms[j].size) {
				break;
			}
			j++;
		}
		if (j < m) {
			continue;
		}
		m_dms.Add(dm);
		m++;
	}

	// sort display modes
	for (unsigned int j, i = 1; i < m_dms.GetCount(); i++) {
		dm = m_dms[i];
		j = i - 1;
		while (j != -1 && m_dms[j].size.cx >= dm.size.cx &&
				m_dms[j].size.cy >= dm.size.cy &&
				m_dms[j].freq > dm.freq) {
			m_dms[j+1] = m_dms[j];
			j--;
		}
		m_dms[j+1] = dm;
	}

	for (int i=0;  (size_t) i<m_dms.GetCount(); i++) {
		strModes.Format(_T("[ %d ]  @ %dx%d "), m_dms[i].freq, m_dms[i].size.cx, m_dms[i].size.cy);
		if (m_dms[i].dmDisplayFlags == DM_INTERLACED) {
			strModes += _T("i");
		} else {
			strModes += _T("p");
		}

		sl.Add(strModes);
		for (int n=0; n<MaxFpsCount; n++) {
			if (m_iSeldm[n] < 0
					&& dmtoset[n].fValid
					&& m_dms[i].size            == dmtoset[n].size
					&& m_dms[i].bpp             == dmtoset[n].bpp
					&& m_dms[i].freq            == dmtoset[n].freq
					&& m_dms[i].dmDisplayFlags  == dmtoset[n].dmDisplayFlags) {
				m_iSeldm[n]=i;
				sl2[n] = sl[i];
				if (strCur == strModes) {
					i0 = i;
				}
			}
		}
	}

	for (int n=0; n<MaxFpsCount; n++) {
		if (m_AutoChangeFullscrRes.dmFullscreenRes[n].fIsData == true) {
			m_list.InsertItem(n, _T(""));
			CString ss = sl2[n];
			m_list.SetItemText(n, COL_SRR, ss);
			m_list.SetCheck(n, m_AutoChangeFullscrRes.dmFullscreenRes[n].fChecked);
			n+1>9 ? ss.Format(_T("%d"), n+1) : ss.Format(_T("0%d"), n+1);
			m_list.SetItemText(n, COL_Z, ss);
			ss.Format(_T("%.3f"), m_AutoChangeFullscrRes.dmFullscreenRes[n].vfr_from) ;
			m_list.SetItemText(n, COL_VFR_F, ss);
			ss.Format(_T("%.3f"), m_AutoChangeFullscrRes.dmFullscreenRes[n].vfr_to) ;
			m_list.SetItemText(n, COL_VFR_T, ss);
		}
	}
	if (m_list.GetItemCount() < 1 || iNoData == 0) {
		strModes.Format(_T("[ %d ]  @ %dx%d "), dmtoset[0].freq, dmtoset[0].size.cx, dmtoset[0].size.cy);
		(dmtoset[0].dmDisplayFlags == DM_INTERLACED) ? strModes += _T("i") : strModes += _T("p");

		int idx = 0;
		m_list.InsertItem(idx, _T("01"));
		m_list.SetItemText(idx, COL_VFR_F, _T("23.500"));
		m_list.SetItemText(idx, COL_VFR_T, _T("23.981"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("02"));
		m_list.SetItemText(idx, COL_VFR_F, _T("23.982"));
		m_list.SetItemText(idx, COL_VFR_T, _T("24.499"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("03"));
		m_list.SetItemText(idx, COL_VFR_F, _T("24.500"));
		m_list.SetItemText(idx, COL_VFR_T, _T("25.499"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("04"));
		m_list.SetItemText(idx, COL_VFR_F, _T("29.500"));
		m_list.SetItemText(idx, COL_VFR_T, _T("29.981"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("05"));
		m_list.SetItemText(idx, COL_VFR_F, _T("29.982"));
		m_list.SetItemText(idx, COL_VFR_T, _T("30.499"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("06"));
		m_list.SetItemText(idx, COL_VFR_F, _T("49.500"));
		m_list.SetItemText(idx, COL_VFR_T, _T("50.499"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("07"));
		m_list.SetItemText(idx, COL_VFR_F, _T("59.500"));
		m_list.SetItemText(idx, COL_VFR_T, _T("59.945"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
		idx++;
		m_list.InsertItem(idx, _T("08"));
		m_list.SetItemText(idx, COL_VFR_F, _T("59.946"));
		m_list.SetItemText(idx, COL_VFR_T, _T("60.499"));
		m_list.SetItemText(idx, COL_SRR, strModes);
		m_iSeldm[idx] = i0;
		m_list.SetCheck(idx, 1);
	}
	ReindexListSubItem();
}

void CPPageFullscreen::OnRemove()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem < 0 || nItem >= m_list.GetItemCount()) {
			return;
		}
		m_list.DeleteItem(nItem);
		nItem = min(nItem, m_list.GetItemCount()-1);
		m_list.SetSelectionMark(nItem);
		m_list.SetFocus();
		m_list.SetItemState(nItem,LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		ReindexList();
		ReindexListSubItem();

		SetModified();
	}
}

void CPPageFullscreen::OnUpdateRemove(CCmdUI* pCmdUI)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int i = m_list.GetNextSelectedItem(pos);
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && m_list.GetItemCount() > 1 && (i >= 0 || pos != NULL));
}

void CPPageFullscreen::OnAdd()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int i = m_list.GetNextSelectedItem(pos)+1;
	if (i<=0) {
		i = m_list.GetItemCount();
	}
	if (m_list.GetItemCount() <= MaxFpsCount) {
		CString str, strCur;
		(i<10) ? str.Format(_T("0%d"), i) : str.Format(_T("%d"), i);
		m_list.InsertItem(i, str);
		m_list.SetItemText(i, COL_VFR_F, _T("1.000"));
		m_list.SetItemText(i, COL_VFR_T, _T("1.000"));
		GetCurDispModeString(strCur);
		m_list.SetItemText(i, COL_SRR, strCur);
		m_list.SetCheck(i,0);
		m_list.SetFocus();
		m_list.SetItemState(i,LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		ReindexList();
		ReindexListSubItem();

		SetModified();
	}
}

void CPPageFullscreen::OnUpdateAdd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnMoveUp()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem <= 0) {
			return;
		}

		DWORD_PTR data = m_list.GetItemData(nItem);
		int nCheckCur = m_list.GetCheck(nItem);
		CString strN = m_list.GetItemText(nItem, 0);
		CString strF = m_list.GetItemText(nItem, 1);
		CString strT = m_list.GetItemText(nItem, 2);
		CString strDM = m_list.GetItemText(nItem, 3);
		m_list.DeleteItem(nItem);

		nItem--;
		m_list.InsertItem(nItem, strN);
		m_list.SetItemData(nItem, data);
		m_list.SetItemText(nItem, 1, strF);
		m_list.SetItemText(nItem, 2, strT);
		m_list.SetItemText(nItem, 3, strDM);
		m_list.SetCheck(nItem, nCheckCur);
		m_list.SetFocus();
		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		ReindexList();
		ReindexListSubItem();

		SetModified();
	}
}

void CPPageFullscreen::OnMoveDown()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem < 0 || nItem >= m_list.GetItemCount()-1) {
			return;
		}

		DWORD_PTR data = m_list.GetItemData(nItem);
		int nCheckCur = m_list.GetCheck(nItem);
		CString strN = m_list.GetItemText(nItem, 0);
		CString strF = m_list.GetItemText(nItem, 1);
		CString strT = m_list.GetItemText(nItem, 2);
		CString strDM = m_list.GetItemText(nItem, 3);
		m_list.DeleteItem(nItem);

		nItem++;

		m_list.InsertItem(nItem, strN);
		m_list.SetItemData(nItem, data);
		m_list.SetItemText(nItem, 1, strF);
		m_list.SetItemText(nItem, 2, strT);
		m_list.SetItemText(nItem, 3, strDM);
		m_list.SetCheck(nItem, nCheckCur);
		m_list.SetFocus();
		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		ReindexList();
		ReindexListSubItem();

		SetModified();
	}
}

void CPPageFullscreen::OnUpdateUp(CCmdUI* pCmdUI)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int i = m_list.GetNextSelectedItem(pos);
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && (i > 0 || pos != NULL));
}

void CPPageFullscreen::OnUpdateDown(CCmdUI* pCmdUI)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int i = m_list.GetNextSelectedItem(pos);
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && i < m_list.GetItemCount()-1);
}

void CPPageFullscreen::ReindexList()
{
	if (m_list.GetItemCount() > 0 ) {
		CString str;
		for (int i = 0; i < m_list.GetItemCount(); i++) {
			(i+1<10) ? str.Format(_T("0%d"), i+1) : str.Format(_T("%d"), i+1);
			m_list.SetItemText(i,0,str);
		}
	}
}

void CPPageFullscreen::ReindexListSubItem()
{
	for (int i=0; (size_t) i< sl.GetCount(); i++) {
		for (int n=0; n<m_list.GetItemCount(); n++) {
			if (m_list.GetItemText(n, COL_SRR) == sl[i]) {
				m_iSeldm[n]=i;
			}
		}
	}
}

void CPPageFullscreen::GetCurDispModeString(CString& strCur)
{
	dispmode dmod;
	GetCurDispMode(dmod, m_f_hmonitor);
	strCur.Format(_T("[ %d ]  @ %dx%d "), dmod.freq, dmod.size.cx, dmod.size.cy);
	(dmod.dmDisplayFlags == DM_INTERLACED) ? strCur += _T("i") : strCur += _T("p");
}
