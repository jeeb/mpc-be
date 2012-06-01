/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
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
#include "PPageOutput.h"
#include "WinAPIUtils.h"
#include <moreuuids.h>
#include "Monitors.h"
#include "AppSettings.h"

// CPPageOutput dialog

IMPLEMENT_DYNAMIC(CPPageOutput, CPPageBase)
CPPageOutput::CPPageOutput()
	: CPPageBase(CPPageOutput::IDD, CPPageOutput::IDD)
	, m_iDSVideoRendererType(VIDRNDT_DS_DEFAULT)
	, m_iRMVideoRendererType(VIDRNDT_RM_DEFAULT)
	, m_iQTVideoRendererType(VIDRNDT_QT_DEFAULT)
	, m_iAPSurfaceUsage(0)
	, m_iAudioRendererType(0)
	, m_iDX9Resizer(0)
	, m_fVMR9MixerMode(FALSE)
	, m_fVMR9MixerYUV(FALSE)
	, m_fVMR9AlterativeVSync(FALSE)
	, m_fResetDevice(FALSE)
	, m_iEvrBuffers(L"5")
	, m_fD3DFullscreen(FALSE)
	, m_fD3D9RenderDevice(FALSE)
	, m_iD3D9RenderDevice(-1)
{
}

CPPageOutput::~CPPageOutput()
{
}

void CPPageOutput::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIDRND_COMBO, m_iDSVideoRendererTypeCtrl);
	DDX_Control(pDX, IDC_RMRND_COMBO, m_iRMVideoRendererTypeCtrl);
	DDX_Control(pDX, IDC_QTRND_COMBO, m_iQTVideoRendererTypeCtrl);
	DDX_Control(pDX, IDC_AUDRND_COMBO, m_iAudioRendererTypeCtrl);
	DDX_Control(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDeviceCtrl);
	DDX_CBIndex(pDX, IDC_RMRND_COMBO, m_iRMVideoRendererType);
	DDX_CBIndex(pDX, IDC_QTRND_COMBO, m_iQTVideoRendererType);
	DDX_CBIndex(pDX, IDC_AUDRND_COMBO, m_iAudioRendererType);
	DDX_CBIndex(pDX, IDC_DX_SURFACE, m_iAPSurfaceUsage);
	DDX_CBIndex(pDX, IDC_DX9RESIZER_COMBO, m_iDX9Resizer);
	DDX_CBIndex(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDevice);
	DDX_Check(pDX, IDC_D3D9DEVICE, m_fD3D9RenderDevice);
	DDX_Check(pDX, IDC_RESETDEVICE, m_fResetDevice);
	DDX_Check(pDX, IDC_FULLSCREEN_MONITOR_CHECK, m_fD3DFullscreen);
	DDX_Check(pDX, IDC_DSVMR9ALTERNATIVEVSYNC, m_fVMR9AlterativeVSync);
	DDX_Check(pDX, IDC_DSVMR9LOADMIXER, m_fVMR9MixerMode);
	DDX_Check(pDX, IDC_DSVMR9YUVMIXER, m_fVMR9MixerYUV);

	DDX_CBString(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
}

BEGIN_MESSAGE_MAP(CPPageOutput, CPPageBase)
	ON_CBN_SELCHANGE(IDC_VIDRND_COMBO, &CPPageOutput::OnDSRendererChange)
	ON_CBN_SELCHANGE(IDC_DX_SURFACE, &CPPageOutput::OnSurfaceChange)
	ON_BN_CLICKED(IDC_D3D9DEVICE, OnD3D9DeviceCheck)
	ON_BN_CLICKED(IDC_FULLSCREEN_MONITOR_CHECK, OnFullscreenCheck)
	ON_UPDATE_COMMAND_UI(IDC_DSVMR9YUVMIXER, OnUpdateMixerYUV)
END_MESSAGE_MAP()

// CPPageOutput message handlers

BOOL CPPageOutput::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_AUDRND_COMBO);

	AppSettings& s = AfxGetAppSettings();

	CRenderersSettings& renderersSettings = s.m_RenderersSettings;
	m_iDSVideoRendererType	= s.iDSVideoRendererType;
	m_iRMVideoRendererType	= s.iRMVideoRendererType;
	m_iQTVideoRendererType	= s.iQTVideoRendererType;
	m_iAPSurfaceUsage		= renderersSettings.iAPSurfaceUsage;
	m_iDX9Resizer			= renderersSettings.iDX9Resizer;
	m_fVMR9MixerMode		= renderersSettings.fVMR9MixerMode;
	m_fVMR9MixerYUV			= renderersSettings.fVMR9MixerYUV;
	m_fVMR9AlterativeVSync	= renderersSettings.m_RenderSettings.fVMR9AlterativeVSync;
	m_fD3DFullscreen		= s.fD3DFullscreen;
	m_iEvrBuffers.Format(L"%d", renderersSettings.iEvrBuffers);

	m_fResetDevice = s.m_RenderersSettings.fResetDevice;
	m_AudioRendererDisplayNames.Add(_T(""));
	m_iAudioRendererTypeCtrl.SetRedraw(FALSE);
	m_iAudioRendererTypeCtrl.AddString(ResStr(IDS_PPAGE_OUTPUT_SYS_DEF));
	m_iAudioRendererType = 0;

	CString fstr;

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
		LPOLESTR olestr = NULL;
		if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
			continue;
		}

		CStringW str(olestr);
		CoTaskMemFree(olestr);

		m_AudioRendererDisplayNames.Add(CString(str));

		CComPtr<IPropertyBag> pPB;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
			CComVariant var;
			pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);

			fstr = var.bstrVal;

			var.Clear();
			if (SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL))) {
				BSTR* pbstr;
				if (SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pbstr))) {
					fstr.Format(_T("%s (%08x)"), CString(fstr), *((DWORD*)pbstr + 1));
					SafeArrayUnaccessData(var.parray);
				}
			}
		} else {
			fstr = str;
		}
		m_iAudioRendererTypeCtrl.AddString(fstr);

		if (s.strAudioRendererDisplayName == str && m_iAudioRendererType == 0) {
			m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
		}
	}
	EndEnumSysDev

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	fstr.Format(_T("%s"), AUDRNDT_NULL_COMP);
	m_iAudioRendererTypeCtrl.AddString(fstr);
	if (s.strAudioRendererDisplayName == AUDRNDT_NULL_COMP && m_iAudioRendererType == 0) {
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
	}

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);
	fstr.Format(_T("%s"), AUDRNDT_NULL_UNCOMP);
	m_iAudioRendererTypeCtrl.AddString(fstr);
	if (s.strAudioRendererDisplayName == AUDRNDT_NULL_UNCOMP && m_iAudioRendererType == 0) {
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
	}

	m_AudioRendererDisplayNames.Add(AUDRNDT_MPC);
	fstr.Format(_T("%s"), AUDRNDT_MPC);
	m_iAudioRendererTypeCtrl.AddString(fstr);

	if (s.strAudioRendererDisplayName == AUDRNDT_MPC && m_iAudioRendererType == 0) {
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
	}
	CorrectComboListWidth(m_iAudioRendererTypeCtrl);
	m_iAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iAudioRendererTypeCtrl.Invalidate();
	m_iAudioRendererTypeCtrl.UpdateWindow();

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D) {
		TCHAR strGUID[50];
		CString cstrGUID;
		CString d3ddevice_str = _T("");
		CStringArray adapterList;

		D3DADAPTER_IDENTIFIER9 adapterIdentifier;

		for (UINT adp = 0; adp < pD3D->GetAdapterCount(); ++adp) {
			if (SUCCEEDED(pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier))) {
				d3ddevice_str = adapterIdentifier.Description;
				d3ddevice_str += _T(" - ");
				d3ddevice_str += adapterIdentifier.DeviceName;
				cstrGUID = _T("");
				if (::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0) {
					cstrGUID = strGUID;
				}
				if ((cstrGUID != _T(""))) {
					boolean m_find = false;
					for (INT_PTR i = 0; (!m_find) && (i < m_D3D9GUIDNames.GetCount()); i++) {
						if (m_D3D9GUIDNames.GetAt(i) == cstrGUID) {
							m_find = true;
						}
					}
					if (!m_find) {
						m_iD3D9RenderDeviceCtrl.AddString(d3ddevice_str);
						m_D3D9GUIDNames.Add(cstrGUID);
						if (renderersSettings.D3D9RenderDevice == cstrGUID) {
							m_iD3D9RenderDevice = m_iD3D9RenderDeviceCtrl.GetCount() - 1;
						}
					}
				}
			}
		}
		pD3D->Release();
	}

	CorrectComboListWidth(m_iD3D9RenderDeviceCtrl);

	CComboBox& m_iDSVRTC = m_iDSVideoRendererTypeCtrl;
	m_iDSVRTC.SetRedraw(FALSE); // Do not draw the control while we are filling it with items
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_SYS_DEF)), VIDRNDT_DS_DEFAULT);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_OLDRENDERER)), VIDRNDT_DS_OLDRENDERER);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_OVERLAYMIXER)), VIDRNDT_DS_OVERLAYMIXER);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR7WINDOWED)), VIDRNDT_DS_VMR7WINDOWED);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR9WINDOWED)), VIDRNDT_DS_VMR9WINDOWED);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR7RENDERLESS)), VIDRNDT_DS_VMR7RENDERLESS);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR9RENDERLESS)), VIDRNDT_DS_VMR9RENDERLESS);
	if (IsCLSIDRegistered(CLSID_EnhancedVideoRenderer)) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_EVR)), VIDRNDT_DS_EVR);
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_EVR_CUSTOM)), VIDRNDT_DS_EVR_CUSTOM);
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_SYNC)), VIDRNDT_DS_SYNC);
 	}

	if (IsCLSIDRegistered(CLSID_DXR)) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_DXR)), VIDRNDT_DS_DXR);
	}

	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_NULL_COMP)), VIDRNDT_DS_NULL_COMP);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_NULL_UNCOMP)), VIDRNDT_DS_NULL_UNCOMP);
	if (IsCLSIDRegistered(CLSID_madVR)) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_MADVR)), VIDRNDT_DS_MADVR);
	}

	for (int i = 0; i < m_iDSVRTC.GetCount(); ++i) {
		if (m_iDSVideoRendererType == m_iDSVRTC.GetItemData(i)) {
			m_iDSVRTC.SetCurSel(i);
			break;
		}
	}
	m_iDSVRTC.SetRedraw(TRUE);
	m_iDSVRTC.Invalidate();
	m_iDSVRTC.UpdateWindow();

	CComboBox& m_iQTVRTC = m_iQTVideoRendererTypeCtrl;
	m_iQTVRTC.SetItemData(m_iQTVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_SYS_DEF)), VIDRNDT_QT_DEFAULT);
	m_iQTVRTC.SetItemData(m_iQTVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR7RENDERLESS)), VIDRNDT_QT_DX7);
	m_iQTVRTC.SetItemData(m_iQTVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR9RENDERLESS)), VIDRNDT_QT_DX9);
	m_iQTVRTC.SetCurSel(m_iQTVideoRendererType);
	CorrectComboListWidth(m_iQTVRTC);

	CComboBox& m_iRMVRTC = m_iRMVideoRendererTypeCtrl;
	m_iRMVideoRendererTypeCtrl.SetItemData(m_iRMVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_SYS_DEF)), VIDRNDT_RM_DEFAULT);
	m_iRMVRTC.SetItemData(m_iRMVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR7RENDERLESS)), VIDRNDT_RM_DX7);
	m_iRMVRTC.SetItemData(m_iRMVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR9RENDERLESS)), VIDRNDT_RM_DX9);
	m_iRMVRTC.SetCurSel(m_iRMVideoRendererType);
	CorrectComboListWidth(m_iRMVRTC);

	UpdateData(FALSE);

	OnDSRendererChange();

	// YUV mixing is incompatible with Vista+
	if (IsWinVistaOrLater()) {
		GetDlgItem(IDC_DSVMR9YUVMIXER)->ShowWindow(SW_HIDE);
	}

	CheckDlgButton(IDC_D3D9DEVICE, BST_UNCHECKED);
	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);

	switch (m_iDSVideoRendererType) {
		case VIDRNDT_DS_VMR9RENDERLESS:
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_iD3D9RenderDeviceCtrl.GetCount() > 1) {
					GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
					GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);
					CheckDlgButton(IDC_D3D9DEVICE, BST_UNCHECKED);
					if (m_iD3D9RenderDevice != -1) {
						CheckDlgButton(IDC_D3D9DEVICE, BST_CHECKED);
						GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(TRUE);
					}
			}
			break;
		default:
			GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
			GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);
 	}
	UpdateData(TRUE);

	CreateToolTip();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageOutput::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	CRenderersSettings& renderersSettings                   = s.m_RenderersSettings;
	s.iDSVideoRendererType		                            = m_iDSVideoRendererType = m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel());
	s.iRMVideoRendererType		                            = m_iRMVideoRendererType;
	s.iQTVideoRendererType		                            = m_iQTVideoRendererType;
	renderersSettings.iAPSurfaceUsage	                    = m_iAPSurfaceUsage;
	renderersSettings.iDX9Resizer		                    = m_iDX9Resizer;
	renderersSettings.fVMR9MixerMode	                    = !!m_fVMR9MixerMode;
	renderersSettings.fVMR9MixerYUV		                    = !!m_fVMR9MixerYUV;

	renderersSettings.m_RenderSettings.fVMR9AlterativeVSync	= m_fVMR9AlterativeVSync != 0;
	s.strAudioRendererDisplayName                           = m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.fD3DFullscreen			                            = m_fD3DFullscreen ? true : false;

	renderersSettings.fResetDevice = !!m_fResetDevice;

	if (!m_iEvrBuffers.IsEmpty()) {
		int Temp = 5;
		swscanf_s(m_iEvrBuffers.GetBuffer(), L"%d", &Temp);
		renderersSettings.iEvrBuffers = Temp;
	} else {
		renderersSettings.iEvrBuffers = 5;
	}

	renderersSettings.D3D9RenderDevice = m_fD3D9RenderDevice ? m_D3D9GUIDNames[m_iD3D9RenderDevice] : _T("");

	return __super::OnApply();
}

void CPPageOutput::OnUpdateMixerYUV(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_DSVMR9LOADMIXER)
				   && (m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel()) == VIDRNDT_DS_VMR9RENDERLESS));
}

void CPPageOutput::OnSurfaceChange()
{
	SetModified();
}

void CPPageOutput::OnDSRendererChange()
{
	const UINT CURRENT_VR = m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel());

	GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(FALSE);
	GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9LOADMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9YUVMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(FALSE);
	GetDlgItem(IDC_RESETDEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(CURRENT_VR == VIDRNDT_DS_EVR_CUSTOM);
	GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(CURRENT_VR == VIDRNDT_DS_EVR_CUSTOM);

	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);

	switch (CURRENT_VR) {
		case VIDRNDT_DS_VMR7RENDERLESS:
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
			break;
		case VIDRNDT_DS_VMR9RENDERLESS:
			GetDlgItem(IDC_DSVMR9LOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9YUVMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_iD3D9RenderDeviceCtrl.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}

			GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);

			// Force 3D surface with EVR Custom
			if (CURRENT_VR == VIDRNDT_DS_EVR_CUSTOM) {
				GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
				((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);
			} else {
				GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
			}
			break;
		case VIDRNDT_DS_MADVR:
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			break;
		case VIDRNDT_DS_SYNC:
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
			((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);
			break;
	}

	SetModified();
}

void CPPageOutput::OnFullscreenCheck()
{
	UpdateData();
	if (m_fD3DFullscreen &&
			(MessageBox(ResStr(IDS_D3DFS_WARNING), NULL, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDNO)) {
		m_fD3DFullscreen = false;
		UpdateData(FALSE);
	} else {
		SetModified();
	}
}

void CPPageOutput::OnD3D9DeviceCheck()
{
	UpdateData();
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(m_fD3D9RenderDevice);
	SetModified();
}
