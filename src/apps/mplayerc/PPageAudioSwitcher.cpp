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
#include <math.h>
#include "../../DSUtil/SysVersion.h"
#include "../../filters/renderer/MpcAudioRenderer/MpcAudioRenderer.h"
#include "ComPropertyPage.h"
#include "MainFrm.h"
#include "PPageAudioSwitcher.h"

// CPPageAudioSwitcher dialog

IMPLEMENT_DYNAMIC(CPPageAudioSwitcher, CPPageBase)
CPPageAudioSwitcher::CPPageAudioSwitcher(IFilterGraph* pFG)
	: CPPageBase(CPPageAudioSwitcher::IDD, CPPageAudioSwitcher::IDD)
	, m_iAudioRendererType(0)
	, m_iSecAudioRendererType(0)

	, m_fAutoloadAudio(FALSE)
	, m_fPrioritizeExternalAudio(FALSE)

	, m_fAudioNormalize(FALSE)
	, m_iAudioRecoverStep(20)
	, m_AudioBoostPos(0)
	, m_fAudioTimeShift(FALSE)
	, m_tAudioTimeShift(0)
{
	m_pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pFG);
}

CPPageAudioSwitcher::~CPPageAudioSwitcher()
{
}

void CPPageAudioSwitcher::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_AUDRND_COMBO, m_iAudioRendererTypeCtrl);
	DDX_Control(pDX, IDC_COMBO1, m_iSecAudioRendererTypeCtrl);
	DDX_CBIndex(pDX, IDC_AUDRND_COMBO, m_iAudioRendererType);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iSecAudioRendererType);
	DDX_Control(pDX, IDC_BUTTON1, m_audRendPropButton);
	DDX_Control(pDX, IDC_CHECK1, m_DualAudioOutput);

	DDX_Check(pDX, IDC_CHECK2, m_fAutoloadAudio);
	DDX_Text(pDX, IDC_EDIT4, m_sAudioPaths);
	DDX_Check(pDX, IDC_CHECK3, m_fPrioritizeExternalAudio);

	DDX_Check(pDX, IDC_CHECK5, m_fAudioNormalize);
	DDX_Slider(pDX, IDC_SLIDER2, m_iAudioRecoverStep);
	DDX_Control(pDX, IDC_SLIDER2, m_AudioRecoverStepCtrl);
	DDX_Slider(pDX, IDC_SLIDER1, m_AudioBoostPos);
	DDX_Control(pDX, IDC_SLIDER1, m_AudioBoostCtrl);
	DDX_Check(pDX, IDC_CHECK4, m_fAudioTimeShift);
	DDX_Control(pDX, IDC_CHECK4, m_fAudioTimeShiftCtrl);
	DDX_Text(pDX, IDC_EDIT2, m_tAudioTimeShift);
	DDX_Control(pDX, IDC_EDIT2, m_tAudioTimeShiftCtrl);
	DDX_Control(pDX, IDC_SPIN2, m_tAudioTimeShiftSpin);
}

BEGIN_MESSAGE_MAP(CPPageAudioSwitcher, CPPageBase)
	ON_CBN_SELCHANGE(IDC_AUDRND_COMBO, OnAudioRendererChange)
	ON_BN_CLICKED(IDC_BUTTON1, OnAudioRenderPropClick)
	ON_BN_CLICKED(IDC_CHECK1, OnDualAudioOutputCheck)

	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedResetAudioPaths)

	ON_UPDATE_COMMAND_UI(IDC_SLIDER2, OnUpdateNormalize)
	ON_UPDATE_COMMAND_UI(IDC_STATIC5, OnUpdateNormalize)
	ON_WM_HSCROLL()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()

// CPPageAudioSwitcher message handlers

BOOL CPPageAudioSwitcher::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

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

	m_fAutoloadAudio           = s.fAutoloadAudio;
	m_fPrioritizeExternalAudio = s.fPrioritizeExternalAudio;
	m_sAudioPaths              = s.strAudioPaths;

	m_fAudioNormalize        = s.fAudioNormalize;
	m_iAudioRecoverStep      = s.iAudioRecoverStep;
	m_AudioRecoverStepCtrl.SetRange(10, 200, TRUE);
	m_AudioBoostPos          = (int)(s.dAudioBoost_dB*10+0.1);
	m_AudioBoostCtrl.SetRange(0, 100, TRUE);
	m_fAudioTimeShift        = s.fAudioTimeShift;
	m_tAudioTimeShift        = s.iAudioTimeShift;
	m_tAudioTimeShiftSpin.SetRange32(-1000*60*60*24, 1000*60*60*24);

	m_tooltip.Create(GetDlgItem(IDC_SLIDER1));
	m_tooltip.Activate(TRUE);

	UpdateData(FALSE);

	OnAudioRendererChange();

	return TRUE;
}

BOOL CPPageAudioSwitcher::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.strAudioRendererDisplayName       = m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.strSecondAudioRendererDisplayName = m_iSecAudioRendererType == -1 ? L"" : m_AudioRendererDisplayNames[m_iSecAudioRendererType];
	s.fDualAudioOutput                  = !!m_DualAudioOutput.GetCheck();

	s.fAutoloadAudio = !!m_fAutoloadAudio;
	s.fPrioritizeExternalAudio = !!m_fPrioritizeExternalAudio;
	s.strAudioPaths = m_sAudioPaths;

	s.fAudioNormalize        = !!m_fAudioNormalize;
	s.iAudioRecoverStep      = m_iAudioRecoverStep;
	s.dAudioBoost_dB         = (float)m_AudioBoostPos/10;
	s.fAudioTimeShift        = !!m_fAudioTimeShift;
	s.iAudioTimeShift        = m_tAudioTimeShift;

	if (m_pASF) {
		m_pASF->SetAudioTimeShift(s.fAudioTimeShift ? 10000i64*s.iAudioTimeShift : 0);
		m_pASF->SetNormalizeBoost(s.fAudioNormalize, s.iAudioRecoverStep, s.dAudioBoost_dB);
	}

	return __super::OnApply();
}

void CPPageAudioSwitcher::ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr))
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

void CPPageAudioSwitcher::OnAudioRendererChange()
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

void CPPageAudioSwitcher::OnAudioRenderPropClick()
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

void CPPageAudioSwitcher::OnDualAudioOutputCheck()
{
	m_iSecAudioRendererTypeCtrl.EnableWindow(!!m_DualAudioOutput.GetCheck());
}

void CPPageAudioSwitcher::OnBnClickedResetAudioPaths()
{
	m_sAudioPaths = DEFAULT_AUDIO_PATHS;

	UpdateData(FALSE);

	SetModified();
}

void CPPageAudioSwitcher::OnUpdateNormalize(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK5));
}

void CPPageAudioSwitcher::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (*pScrollBar == m_AudioBoostCtrl) {
		UpdateData();
		((CMainFrame*)GetParentFrame())->SetVolumeBoost((float)m_AudioBoostPos/10);
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CPPageAudioSwitcher::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	UINT_PTR nID = pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND) {
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID != IDC_SLIDER1) {
		return FALSE;
	}

	static CString strTipText;

	strTipText.Format(_T("+%.1f dB"), m_AudioBoostCtrl.GetPos()/10.0);

	pTTT->lpszText = (LPWSTR)(LPCWSTR)strTipText;

	*pResult = 0;

	return TRUE;
}

void CPPageAudioSwitcher::OnCancel()
{
	AppSettings& s = AfxGetAppSettings();

	if (m_AudioBoostPos != (int)(s.dAudioBoost_dB*10+0.1)) {
		((CMainFrame*)GetParentFrame())->SetVolumeBoost(s.dAudioBoost_dB);
	}

	__super::OnCancel();
}
