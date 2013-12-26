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

#include "stdafx.h"
#include "PPageOutput.h"
#include "../../DSUtil/SysVersion.h"
#include <moreuuids.h>
#include "MultiMonitor.h"
#include "ComPropertyPage.h"
#include "../../filters/filters.h"

// CPPageOutput dialog

static const LPCTSTR EmptyText = L"";

static bool IsRenderTypeAvailable(UINT VideoRendererType)
{
	switch (VideoRendererType) {
		case VIDRNDT_DS_EVR:
		case VIDRNDT_DS_EVR_CUSTOM:
		case VIDRNDT_DS_SYNC:
			return IsCLSIDRegistered(CLSID_EnhancedVideoRenderer);
		case VIDRNDT_DS_DXR:
			return IsCLSIDRegistered(CLSID_DXR);
		case VIDRNDT_DS_MADVR:
			return IsCLSIDRegistered(CLSID_madVR);
		default:
		return true;
	}
}

IMPLEMENT_DYNAMIC(CPPageOutput, CPPageBase)
CPPageOutput::CPPageOutput()
	: CPPageBase(CPPageOutput::IDD, CPPageOutput::IDD)
	, m_iDSVideoRendererType(VIDRNDT_DS_DEFAULT)
	, m_iDSVideoRendererType_store(VIDRNDT_DS_DEFAULT)
	, m_iRMVideoRendererType(VIDRNDT_RM_DEFAULT)
	, m_iQTVideoRendererType(VIDRNDT_QT_DEFAULT)
	, m_iAPSurfaceUsage(0)
	, m_iAudioRendererType(0)
	, m_iSecAudioRendererType(0)
	, m_iDX9Resizer(0)
	, m_fVMRMixerMode(FALSE)
	, m_fVMRMixerYUV(FALSE)
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
	DDX_Control(pDX, IDC_COMBO1, m_iSecAudioRendererTypeCtrl);
	DDX_Control(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDeviceCtrl);
	DDX_CBIndex(pDX, IDC_RMRND_COMBO, m_iRMVideoRendererType);
	DDX_CBIndex(pDX, IDC_QTRND_COMBO, m_iQTVideoRendererType);
	DDX_CBIndex(pDX, IDC_AUDRND_COMBO, m_iAudioRendererType);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iSecAudioRendererType);
	DDX_CBIndex(pDX, IDC_DX_SURFACE, m_iAPSurfaceUsage);
	DDX_CBIndex(pDX, IDC_DX9RESIZER_COMBO, m_iDX9Resizer);
	DDX_CBIndex(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDevice);
	DDX_Check(pDX, IDC_D3D9DEVICE, m_fD3D9RenderDevice);
	DDX_Check(pDX, IDC_RESETDEVICE, m_fResetDevice);
	DDX_Check(pDX, IDC_FULLSCREEN_MONITOR_CHECK, m_fD3DFullscreen);
	DDX_Check(pDX, IDC_DSVMR9ALTERNATIVEVSYNC, m_fVMR9AlterativeVSync);
	DDX_Check(pDX, IDC_DSVMRLOADMIXER, m_fVMRMixerMode);
	DDX_Check(pDX, IDC_DSVMRYUVMIXER, m_fVMRMixerYUV);

	DDX_CBString(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
	DDX_Control(pDX, IDC_BUTTON1, m_audRendPropButton);
	DDX_Control(pDX, IDC_CHECK1, m_DualAudioOutput);
}

BEGIN_MESSAGE_MAP(CPPageOutput, CPPageBase)
	ON_CBN_SELCHANGE(IDC_VIDRND_COMBO, OnDSRendererChange)
	ON_CBN_SELCHANGE(IDC_RMRND_COMBO, OnRMRendererChange)
	ON_CBN_SELCHANGE(IDC_QTRND_COMBO, OnQTRendererChange)
	ON_CBN_SELCHANGE(IDC_DX_SURFACE, OnSurfaceChange)
	ON_BN_CLICKED(IDC_D3D9DEVICE, OnD3D9DeviceCheck)
	ON_BN_CLICKED(IDC_FULLSCREEN_MONITOR_CHECK, OnFullscreenCheck)
	ON_UPDATE_COMMAND_UI(IDC_DSVMRYUVMIXER, OnUpdateMixerYUV)
	ON_CBN_SELCHANGE(IDC_AUDRND_COMBO, OnAudioRendererChange)
	ON_BN_CLICKED(IDC_BUTTON1, OnAudioRenderPropClick)
	ON_BN_CLICKED(IDC_CHECK1, OnDualAudioOutputCheck)
END_MESSAGE_MAP()

// CPPageOutput message handlers

BOOL CPPageOutput::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_AUDRND_COMBO);
	SetHandCursor(m_hWnd, IDC_BUTTON1);

	AppSettings& s = AfxGetAppSettings();

	CRenderersSettings& renderersSettings = s.m_RenderersSettings;
	m_iDSVideoRendererType	= s.iDSVideoRendererType;
	m_iRMVideoRendererType	= s.iRMVideoRendererType;
	m_iQTVideoRendererType	= s.iQTVideoRendererType;
	m_iAPSurfaceUsage		= renderersSettings.iAPSurfaceUsage;
	m_iDX9Resizer			= renderersSettings.iDX9Resizer;
	m_fVMRMixerMode			= renderersSettings.fVMRMixerMode;
	m_fVMRMixerYUV			= renderersSettings.fVMRMixerYUV;
	m_fVMR9AlterativeVSync	= renderersSettings.m_AdvRendSets.fVMR9AlterativeVSync;
	m_fD3DFullscreen		= s.fD3DFullscreen;
	m_iEvrBuffers.Format(L"%d", renderersSettings.iEvrBuffers);

	m_fResetDevice = s.m_RenderersSettings.fResetDevice;

	m_AudioRendererDisplayNames.Add(_T(""));
	m_iAudioRendererTypeCtrl.SetRedraw(FALSE);
	m_iAudioRendererTypeCtrl.AddString(_T("1: ") + ResStr(IDS_PPAGE_OUTPUT_SYS_DEF));
	m_iAudioRendererType = 0;

	m_DualAudioOutput.SetCheck(s.fDualAudioOutput);
	m_iSecAudioRendererTypeCtrl.SetRedraw(FALSE);
	m_iSecAudioRendererTypeCtrl.AddString(_T("1: ") + ResStr(IDS_PPAGE_OUTPUT_SYS_DEF));
	m_iSecAudioRendererType = 0;

	OnDualAudioOutputCheck();

	int i = 2;
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
		} else {
			fstr = str;
		}
		CString Cbstr;
		Cbstr.Format(_T("%d: %s"), i++, fstr);
		m_iAudioRendererTypeCtrl.AddString(Cbstr);
		m_iSecAudioRendererTypeCtrl.AddString(Cbstr);
	}
	EndEnumSysDev;


	static CString AudioDevAddon[] = {
		AUDRNDT_NULL_COMP,
		AUDRNDT_NULL_UNCOMP,
		IsWinVistaOrLater() ? AUDRNDT_MPC : L""
	};

	for (size_t idx = 0; idx < _countof(AudioDevAddon); idx++) {
		if (AudioDevAddon[idx].GetLength() > 0) {
			m_AudioRendererDisplayNames.Add(AudioDevAddon[idx]);

			fstr.Format(_T("%d: %s"), i++, AudioDevAddon[idx]);
			m_iAudioRendererTypeCtrl.AddString(fstr);
			m_iSecAudioRendererTypeCtrl.AddString(fstr);
		}
	}

	for (INT_PTR idx = 0; idx < m_AudioRendererDisplayNames.GetCount(); idx++) {
		if (s.strAudioRendererDisplayName == m_AudioRendererDisplayNames[idx]) {
			m_iAudioRendererType = idx;
		}

		if (s.strSecondAudioRendererDisplayName == m_AudioRendererDisplayNames[idx]) {
			m_iSecAudioRendererType = idx;
		}
	}

	CorrectComboListWidth(m_iAudioRendererTypeCtrl);
	m_iAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iAudioRendererTypeCtrl.Invalidate();
	m_iAudioRendererTypeCtrl.UpdateWindow();

	CorrectComboListWidth(m_iSecAudioRendererTypeCtrl);
	m_iSecAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iSecAudioRendererTypeCtrl.Invalidate();
	m_iSecAudioRendererTypeCtrl.UpdateWindow();

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D) {
		TCHAR strGUID[50] = {0};
		CString cstrGUID;
		CString d3ddevice_str;
		CStringArray adapterList;

		D3DADAPTER_IDENTIFIER9 adapterIdentifier;

		for (UINT adp = 0; adp < pD3D->GetAdapterCount(); ++adp) {
			if (SUCCEEDED(pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier))) {
				d3ddevice_str = adapterIdentifier.Description;
				d3ddevice_str += _T(" - ");
				d3ddevice_str += adapterIdentifier.DeviceName;
				cstrGUID.Empty();
				if (::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0) {
					cstrGUID = strGUID;
				}
				if (cstrGUID.GetLength() > 0) {
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
	if (IsWinXP()) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_OVERLAYMIXER)), VIDRNDT_DS_OVERLAYMIXER);
	}
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR7WINDOWED)), VIDRNDT_DS_VMR7WINDOWED);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR9WINDOWED)), VIDRNDT_DS_VMR9WINDOWED);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR7RENDERLESS)), VIDRNDT_DS_VMR7RENDERLESS);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_VMR9RENDERLESS)), VIDRNDT_DS_VMR9RENDERLESS);
	if (IsCLSIDRegistered(CLSID_EnhancedVideoRenderer)) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_EVR)), VIDRNDT_DS_EVR);
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_EVR_CUSTOM)), VIDRNDT_DS_EVR_CUSTOM);
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_SYNC)), VIDRNDT_DS_SYNC);
	} else {
		CString str;
		str.Format(_T("%s %s"), ResStr(IDS_PPAGE_OUTPUT_EVR), ResStr(IDS_PPAGE_OUTPUT_UNAVAILABLE));
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(str), VIDRNDT_DS_EVR);
		str.Format(_T("%s %s"), ResStr(IDS_PPAGE_OUTPUT_EVR_CUSTOM), ResStr(IDS_PPAGE_OUTPUT_UNAVAILABLE));
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(str), VIDRNDT_DS_EVR_CUSTOM);
		str.Format(_T("%s %s"), ResStr(IDS_PPAGE_OUTPUT_SYNC), ResStr(IDS_PPAGE_OUTPUT_UNAVAILABLE));
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(str), VIDRNDT_DS_SYNC);
	}

	if (IsCLSIDRegistered(CLSID_DXR)) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_DXR)), VIDRNDT_DS_DXR);
	} else {
		CString str;
		str.Format(_T("%s %s"), ResStr(IDS_PPAGE_OUTPUT_DXR), ResStr(IDS_PPAGE_OUTPUT_UNAVAILABLE));
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(str), VIDRNDT_DS_DXR);
	}

	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_NULL_COMP)), VIDRNDT_DS_NULL_COMP);
	m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_NULL_UNCOMP)), VIDRNDT_DS_NULL_UNCOMP);
	if (IsCLSIDRegistered(CLSID_madVR)) {
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(ResStr(IDS_PPAGE_OUTPUT_MADVR)), VIDRNDT_DS_MADVR);
	} else {
		CString str;
		str.Format(_T("%s %s"), ResStr(IDS_PPAGE_OUTPUT_MADVR), ResStr(IDS_PPAGE_OUTPUT_UNAVAILABLE));
		m_iDSVRTC.SetItemData(m_iDSVRTC.AddString(str), VIDRNDT_DS_MADVR);
	}

	for (int i = 0; i < m_iDSVRTC.GetCount(); ++i) {
		if (m_iDSVideoRendererType == m_iDSVRTC.GetItemData(i)) {
			if (IsRenderTypeAvailable(m_iDSVideoRendererType)) {
				m_iDSVRTC.SetCurSel(i);
				m_iDSVideoRendererType_store = m_iDSVideoRendererType;
			} else {
				m_iDSVRTC.SetCurSel(0);
			}
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

	CreateToolTip();
	m_wndToolTip.AddTool(GetDlgItem(IDC_VIDRND_COMBO), EmptyText);
	m_wndToolTip.AddTool(GetDlgItem(IDC_RMRND_COMBO), EmptyText);
	m_wndToolTip.AddTool(GetDlgItem(IDC_QTRND_COMBO), EmptyText);
	m_wndToolTip.AddTool(GetDlgItem(IDC_DX_SURFACE), EmptyText);

	OnDSRendererChange();
	OnRMRendererChange();
	OnQTRendererChange();
	OnSurfaceChange();
	OnAudioRendererChange();

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

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CPPageOutput::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	CRenderersSettings& renderersSettings                   = s.m_RenderersSettings;
	s.iDSVideoRendererType		                            = m_iDSVideoRendererType = m_iDSVideoRendererType_store = m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel());
	s.iRMVideoRendererType		                            = m_iRMVideoRendererType;
	s.iQTVideoRendererType		                            = m_iQTVideoRendererType;
	renderersSettings.iAPSurfaceUsage	                    = m_iAPSurfaceUsage;
	renderersSettings.iDX9Resizer		                    = m_iDX9Resizer;
	renderersSettings.fVMRMixerMode							= !!m_fVMRMixerMode;
	renderersSettings.fVMRMixerYUV		                    = !!m_fVMRMixerYUV;

	renderersSettings.m_AdvRendSets.fVMR9AlterativeVSync	= m_fVMR9AlterativeVSync != 0;
	s.strAudioRendererDisplayName                           = m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.strSecondAudioRendererDisplayName                     = m_iSecAudioRendererType == -1 ? L"" : m_AudioRendererDisplayNames[m_iSecAudioRendererType];
	s.fDualAudioOutput                                      = !!m_DualAudioOutput.GetCheck();
	s.fD3DFullscreen			                            = m_fD3DFullscreen ? true : false;

	renderersSettings.fResetDevice = !!m_fResetDevice;

	if (!m_iEvrBuffers.IsEmpty()) {
		int Temp = 5;
		swscanf_s(m_iEvrBuffers.GetBuffer(), L"%d", &Temp);
		renderersSettings.iEvrBuffers = Temp;
	} else {
		renderersSettings.iEvrBuffers = 5;
	}

	renderersSettings.D3D9RenderDevice = m_fD3D9RenderDevice ? m_D3D9GUIDNames[m_iD3D9RenderDevice] : L"";

	return __super::OnApply();
}

void CPPageOutput::OnUpdateMixerYUV(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_DSVMRLOADMIXER)
					&& (m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel()) == VIDRNDT_DS_VMR7WINDOWED
						|| m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel()) == VIDRNDT_DS_VMR9WINDOWED
						|| m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel()) == VIDRNDT_DS_VMR7RENDERLESS
						|| m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel()) == VIDRNDT_DS_VMR9RENDERLESS));
}

void CPPageOutput::OnSurfaceChange()
{
	UpdateData();

	switch (m_iAPSurfaceUsage) {
		case VIDRNDT_AP_SURFACE:
			m_wndToolTip.UpdateTipText(ResStr(IDC_REGULARSURF), GetDlgItem(IDC_DX_SURFACE));
			break;
		case VIDRNDT_AP_TEXTURE2D:
			m_wndToolTip.UpdateTipText(ResStr(IDC_TEXTURESURF2D), GetDlgItem(IDC_DX_SURFACE));
			break;
		case VIDRNDT_AP_TEXTURE3D:
			m_wndToolTip.UpdateTipText(ResStr(IDC_TEXTURESURF3D), GetDlgItem(IDC_DX_SURFACE));
			break;
	}

	SetModified();
}

void CPPageOutput::OnDSRendererChange()
{
	UINT CurrentVR = m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel());

	if (!IsRenderTypeAvailable(CurrentVR)) {
		AfxMessageBox(IDS_PPAGE_OUTPUT_UNAVAILABLEMSG, MB_ICONEXCLAMATION | MB_OK, 0);

		// revert to the last saved renderer
		m_iDSVideoRendererTypeCtrl.SetCurSel(0);
		for (int i = 0; i < m_iDSVideoRendererTypeCtrl.GetCount(); ++i) {
			if (m_iDSVideoRendererType_store == m_iDSVideoRendererTypeCtrl.GetItemData(i)) {
				m_iDSVideoRendererTypeCtrl.SetCurSel(i);
				CurrentVR = m_iDSVideoRendererTypeCtrl.GetItemData(m_iDSVideoRendererTypeCtrl.GetCurSel());
				break;
			}
		}
	}

	GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(FALSE);
	GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(FALSE);
	GetDlgItem(IDC_RESETDEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);

	switch (CurrentVR) {
		case VIDRNDT_DS_DEFAULT:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYSDEF), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_OVERLAYMIXER:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSOVERLAYMIXER), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR7WINDOWED:
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR7WIN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR9WINDOWED:
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9WIN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR7RENDERLESS:
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR7REN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR9RENDERLESS:
			if (m_iD3D9RenderDeviceCtrl.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9REN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_EVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_iD3D9RenderDeviceCtrl.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}

			GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(TRUE);

			// Force 3D surface with EVR Custom
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
			((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR_CUSTOM), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_SYNC:
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
			((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYNC), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_DXR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSDXR), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_NULL_COMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_COMP), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_NULL_UNCOMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_UNCOMP), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_MADVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSMADVR), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		default:
			m_wndToolTip.UpdateTipText(EmptyText, GetDlgItem(IDC_VIDRND_COMBO));
	}

	SetModified();
}

void CPPageOutput::OnRMRendererChange()
{
	UpdateData();

	switch (m_iRMVideoRendererType) {
		case VIDRNDT_RM_DEFAULT:
			m_wndToolTip.UpdateTipText(ResStr(IDC_RMSYSDEF), GetDlgItem(IDC_RMRND_COMBO));
			break;
		case VIDRNDT_RM_DX7:
			m_wndToolTip.UpdateTipText(ResStr(IDC_RMDX7), GetDlgItem(IDC_RMRND_COMBO));
			break;
		case VIDRNDT_RM_DX9:
			m_wndToolTip.UpdateTipText(ResStr(IDC_RMDX9), GetDlgItem(IDC_RMRND_COMBO));
			break;
	}

	SetModified();
}

void CPPageOutput::OnQTRendererChange()
{
	UpdateData();

	switch (m_iQTVideoRendererType) {
		case VIDRNDT_QT_DEFAULT:
			m_wndToolTip.UpdateTipText(ResStr(IDC_QTSYSDEF), GetDlgItem(IDC_QTRND_COMBO));
			break;
		case VIDRNDT_QT_DX7:
			m_wndToolTip.UpdateTipText(ResStr(IDC_QTDX7), GetDlgItem(IDC_QTRND_COMBO));
			break;
		case VIDRNDT_QT_DX9:
			m_wndToolTip.UpdateTipText(ResStr(IDC_QTDX9), GetDlgItem(IDC_QTRND_COMBO));
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

void CPPageOutput::OnAudioRendererChange()
{
	UpdateData();

	BOOL flag = FALSE;
	CString str_audio = m_AudioRendererDisplayNames[m_iAudioRendererType];
	if (str_audio == AUDRNDT_MPC) {
		flag = TRUE;
	} else {
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			LPOLESTR olestr = NULL;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			if (str == m_AudioRendererDisplayNames[m_iAudioRendererType]) {
				CComPtr<IBaseFilter> pBF;
				HRESULT hr = pMoniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&pBF);
				if (SUCCEEDED(hr)) {
					if (CComQIPtr<ISpecifyPropertyPages> pSPP = pBF) {
						flag = TRUE;
						break;
					}
				}
			}
		}
		EndEnumSysDev
	}

	m_audRendPropButton.EnableWindow(flag);

	SetModified();
}

void CPPageOutput::OnAudioRenderPropClick()
{
	CString str_audio = m_AudioRendererDisplayNames[m_iAudioRendererType];

	if (str_audio == AUDRNDT_MPC) {
		ShowPPage(CreateInstance<CMpcAudioRenderer>);
	} else {
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			LPOLESTR olestr = NULL;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			if (str == str_audio) {
				CComPtr<IBaseFilter> pBF;
				HRESULT hr = pMoniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&pBF);
				if (SUCCEEDED(hr)) {
					ISpecifyPropertyPages *pProp = NULL;
					hr = pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
					if (SUCCEEDED(hr)) {
						// Get the filter's name and IUnknown pointer.
						FILTER_INFO FilterInfo;
						hr = pBF->QueryFilterInfo(&FilterInfo);
						if (SUCCEEDED(hr)) {
							IUnknown *pFilterUnk;
							hr = pBF->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);
							if (SUCCEEDED(hr)) {

								// Show the page.
								CAUUID caGUID;
								pProp->GetPages(&caGUID);
								pProp->Release();

								OleCreatePropertyFrame(
									this->m_hWnd,			// Parent window
									0, 0,					// Reserved
									FilterInfo.achName,		// Caption for the dialog box
									1,						// Number of objects (just the filter)
									&pFilterUnk,			// Array of object pointers.
									caGUID.cElems,			// Number of property pages
									caGUID.pElems,			// Array of property page CLSIDs
									0,						// Locale identifier
									0, NULL					// Reserved
								);

								// Clean up.
								CoTaskMemFree(caGUID.pElems);
								pFilterUnk->Release();
							}
							if (FilterInfo.pGraph) {
								FilterInfo.pGraph->Release();
							}
						}
					}
				}
				break;
			}
		}
		EndEnumSysDev
	}
}

void CPPageOutput::ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr))
{
	if (!CreateInstance) {
		return;
	}

	HRESULT hr;
	CUnknown* pObj = CreateInstance(NULL, &hr);

	if (!pObj) {
		return;
	}

	CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pObj;

	if (SUCCEEDED(hr)) {
		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk) {
			CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);
			ps.AddPages(pSPP);
			ps.DoModal();
		}
	}
}

void CPPageOutput::OnDualAudioOutputCheck()
{
	m_iSecAudioRendererTypeCtrl.EnableWindow(!!m_DualAudioOutput.GetCheck());
}
