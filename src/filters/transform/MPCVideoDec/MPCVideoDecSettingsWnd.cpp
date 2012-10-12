/*
 * $Id$
 *
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
#include "MPCVideoDecSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/PODtypes.h"
#include <ffmpeg/libavcodec/avcodec.h>
#include "ffImgfmt.h"

// ==>>> Resource identifier from "resource.h" present in mplayerc project!
#define ResStr(id) CString(MAKEINTRESOURCE(id))

//
// CMPCVideoDecSettingsWnd
//

int g_AVDiscard[] = {
	AVDISCARD_NONE,
	AVDISCARD_DEFAULT,
	AVDISCARD_NONREF,
	AVDISCARD_BIDIR,
	AVDISCARD_NONKEY,
	AVDISCARD_ALL,
};

int FindDiscardIndex(int nValue)
{
	for (int i=0; i<_countof (g_AVDiscard); i++)
		if (g_AVDiscard[i] == nValue) {
			return i;
		}
	return 1;
}

int g_AVErrRecognition[] = {
	AV_EF_CAREFUL,
	AV_EF_COMPLIANT,
	AV_EF_AGGRESSIVE,
};

int FindErrRecognitionIndex(int nValue)
{
	for (int i=0; i<_countof (g_AVErrRecognition); i++)
		if (g_AVErrRecognition[i] == nValue) {
			return i;
		}
	return 1;
}

CMPCVideoDecSettingsWnd::CMPCVideoDecSettingsWnd()
{
}

bool CMPCVideoDecSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMDF);

	m_pMDF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMDF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMDF) {
		return false;
	}

	return true;
}

void CMPCVideoDecSettingsWnd::OnDisconnect()
{
	if (m_pMDF) {
		m_pMDF->SetDialogHWND(0);
	}

	m_pMDF.Release();
}

bool CMPCVideoDecSettingsWnd::OnActivate()
{
	ASSERT(IPP_FONTSIZE == 13);
	const int h16 = IPP_SCALE(16);
	const int h20 = IPP_SCALE(20);
	const int h25 = IPP_SCALE(25);
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
	CPoint p(10, 10);
	GUID* DxvaGui = NULL;

	m_grpFFMpeg.Create(ResStr(IDS_VDF_FFSETTINGS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(IPP_SCALE(350), h20 + h25 * 4 + h20)), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Decoding threads
	m_txtThreadNumber.Create(ResStr(IDS_VDF_THREADNUMBER), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbThreadNumber.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_THREAD_NUMBER);
	m_cbThreadNumber.AddString (ResStr (IDS_VDF_IDCT_AUTO));
	CString ThreadNumberStr;
	for (int i=0; i<16; i++) {
		ThreadNumberStr.Format		(_T("%d"), i+1);
		m_cbThreadNumber.AddString	(ThreadNumberStr);
	}
	p.y += h25;

	// H264 deblocking mode
	m_txtDiscardMode.Create(ResStr(IDS_VDF_SKIPDEBLOCK), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDiscardMode.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DISCARD_MODE);
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONE));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_DEFAULT));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONREF));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_BIDIR));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONKFRM));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_ALL));
	p.y += h25;

	// Error recognition
	m_txtErrorRecognition.Create(ResStr(IDS_VDF_ERROR_RECOGNITION), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbErrorRecognition.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DISCARD_MODE);
	m_cbErrorRecognition.AddString (ResStr (IDS_VDF_ERR_CAREFUL));
	m_cbErrorRecognition.AddString (ResStr (IDS_VDF_ERR_COMPLIANT));
	m_cbErrorRecognition.AddString (ResStr (IDS_VDF_ERR_AGGRESSIVE));
	p.y += h25;

	// IDCT Algorithm
	m_txtIDCTAlgo.Create(ResStr(IDS_VDF_IDCT_ALGO), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(220), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbIDCTAlgo.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DISCARD_MODE);
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_AUTO));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_LIBMPEG2));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_SIMPLE_MMX));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_XVID));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_SIMPLE));
	p.y += h25;

	m_cbARMode.Create(ResStr(IDS_VDF_AR_MODE), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(IPP_SCALE(340), m_fontheight)), this, IDC_PP_AR);
	m_cbARMode.SetCheck(FALSE);
	p.y += h25;

	m_grpDXVA.Create(ResStr(IDS_VDF_DXVA_SETTING),   WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(IPP_SCALE(350), h20 + h25 +h20 * 3 + m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;

	// DXVA Compatibility check
	m_txtDXVACompatibilityCheck.Create(ResStr(IDS_VDF_DXVACOMPATIBILITY), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(225), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDXVACompatibilityCheck.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(IPP_SCALE(230), -4), CSize(IPP_SCALE(110), 200)), this, IDC_PP_DXVA_CHECK);
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_FULLCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_LEVELCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_REFCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_ALLCHECK));
	p.y += h25;

	m_cbDXVA_SD.Create(ResStr(IDS_VDF_DXVA_SD), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(IPP_SCALE(340), m_fontheight)), this, IDC_PP_DXVA_SD);
	m_cbDXVA_SD.SetCheck (FALSE);
	p.y += h20;

	// DXVA mode
	m_txtDXVAMode.Create(ResStr(IDS_VDF_DXVA_MODE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(120), m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtDXVAMode.Create(WS_CHILD | WS_VISIBLE | WS_DISABLED, CRect(p + CPoint(IPP_SCALE(120), 0), CSize(IPP_SCALE(220), m_fontheight)), this, 0);
	p.y += h20;

	// Video card description
	m_txtVideoCardDescription.Create(ResStr(IDS_VDF_VIDEOCARD), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(120), m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtVideoCardDescription.Create(ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_DISABLED, CRect(p + CPoint(IPP_SCALE(120), 0), CSize(IPP_SCALE(220), m_fontheight * 2)), this, 0);
	m_edtVideoCardDescription.SetWindowText (m_pMDF->GetVideoCardDescription());

	DxvaGui = m_pMDF->GetDXVADecoderGuid();
	if (DxvaGui != NULL) {
		CString DXVAMode = GetDXVAMode (DxvaGui);
		m_edtDXVAMode.SetWindowText (DXVAMode);
	} else {
		m_txtDXVAMode.ShowWindow (SW_HIDE);
		m_edtDXVAMode.ShowWindow (SW_HIDE);
	}

	// === New swscaler options
	p = CPoint(IPP_SCALE(360), 10);
	int width_s  = IPP_SCALE(88);

	// Software output formats
	static wchar_t *SwOutputFormatNames[] = { _T("NV12 (default)"), _T("YV12"), _T("YUY2"), _T("RGB32"), _T("RGB16"), _T("RGB15") };
	ASSERT(_countof(SwOutputFormatNames) == _countof(m_nSwIndex));
	int nSwOF = m_pMDF ? m_pMDF->GetSwOutputFormats() : 0;
	// get the output formats order from the DWORD nibbles extracting literal values [0x00543210] = 0,1,2,3,4,5
	// get the output formats with the checked flag (8) from the DWORD nibbles [0x0054ba98] = 0,1,2,3
	for (int i = 0; i < _countof(m_nSwIndex); i++){
		if (nSwOF == 0) {
			m_nSwIndex[i]   = i;
			m_nSwChecked[i] = 1;
		} else {
			m_nSwIndex[i]   = ( nSwOF & ( 0x0000000F << (4*i) ) ) >> (4*i);
			m_nSwChecked[i] = (m_nSwIndex[i] & 8) != 0;
			m_nSwIndex[i]  &= ~8;
		}
	}
	m_txtSwOutputFormats.Create(_T("Output formats:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_lstSwOutputFormats.Create(dwStyle|WS_BORDER|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS, CRect(p, CSize(width_s, IPP_SCALE(82))), this, 0);
	for (int i = 0; i < _countof(m_nSwIndex); i++) {
		m_lstSwOutputFormats.AddString(SwOutputFormatNames[m_nSwIndex[i]]);
	}
	p.y += IPP_SCALE(82);

	// Software Output formats order
	int btn_size = m_fontheight + 4;
	m_cbSwOutputFormatUp.Create(_T("<"), dwStyle|BS_PUSHBUTTON, CRect(p + CPoint(width_s - btn_size * 2, 0), CSize(btn_size, btn_size)), this, IDC_PP_SWOUTPUTFORMATUP);
	m_cbSwOutputFormatDown.Create(_T(">"), dwStyle|BS_PUSHBUTTON, CRect(p + CPoint(width_s - btn_size, 0), CSize(btn_size, btn_size)), this, IDC_PP_SWOUTPUTFORMATDOWN);
	p.y += h20;

	// Resize Method
	m_txtSwResizeMethodBE.Create(_T("Resize Method:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_cbSwResizeMethodBE.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p, CSize(width_s, 200)), this, IDC_PP_RESIZEMETHODBE);
	m_cbSwResizeMethodBE.AddString(_T("Area"));          // ResStr(IDS_VDF_CHR_AREA)
	m_cbSwResizeMethodBE.AddString(_T("Bicubic"));       // ResStr(IDS_VDF_CHR_BICUBIC)
//	m_cbSwResizeMethodBE.AddString(_T("Bicublin"));      // ResStr(IDS_VDF_CHR_BICUBLIN) //temp rem
	m_cbSwResizeMethodBE.AddString(_T("Bilinear"));      // ResStr(IDS_VDF_CHR_BILINEAR)
	m_cbSwResizeMethodBE.AddString(_T("Fast Bilinear")); // ResStr(IDS_VDF_CHR_FAST_BILINEAR)
	m_cbSwResizeMethodBE.AddString(_T("Gauss"));         // ResStr(IDS_VDF_CHR_FULL_GAUSS)
	m_cbSwResizeMethodBE.AddString(_T("Lanczos"));       // ResStr(IDS_VDF_CHR_FULL_LANCZOS)
	m_cbSwResizeMethodBE.AddString(_T("Point"));         // ResStr(IDS_VDF_CHR_FULL_POINT)
	m_cbSwResizeMethodBE.AddString(_T("Sinc"));          // ResStr(IDS_VDF_CHR_FULL_SINC)
	m_cbSwResizeMethodBE.AddString(_T("Spline"));        // ResStr(IDS_VDF_CHR_FULL_SPLINE)
	m_cbSwResizeMethodBE.AddString(_T("X"));             // ResStr(IDS_VDF_CHR_FULL_X)
	p.y += h25;
	
	// Chroma options
	m_txtSwChromaToRGB.Create(_T("Chroma to RGB:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_cbSwChromaToRGB.Create (dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p, CSize(width_s, 200)), this, IDC_PP_SWCHROMATORGB);
	m_cbSwChromaToRGB.AddString(_T("Fast"));   // ResStr(IDS_VDF_CHR_FAST)
	m_cbSwChromaToRGB.AddString(_T("Normal")); // ResStr(IDS_VDF_CHR_NORMAL)
//	m_cbSwChromaToRGB.AddString(_T("High"));   // ResStr(IDS_VDF_CHR_HIGH) //temp rem
	m_cbSwChromaToRGB.AddString(_T("Full"));   // ResStr(IDS_VDF_CHR_FULL)
	p.y += h25;
	
	// Software Colorspace
	m_txtSwColorspace.Create(_T("Colorspace:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_cbSwColorspace.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p, CSize(width_s, 200)), this, IDC_PP_SWCOLORSPACE);
	m_cbSwColorspace.AddString(_T("SD (BT.601)")); // ResStr(IDS_VDF_COLOR_HD)
	m_cbSwColorspace.AddString(_T("HD (BT.709)")); // ResStr(IDS_VDF_COLOR_SD)
	m_cbSwColorspace.AddString(_T("Auto"));        // ResStr(IDS_VDF_COLOR_AUTO)
	p.y += h25;

	// Input levels
	m_txtSwInputLevels.Create(_T("Input levels:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_cbSwInputLevels.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p, CSize(width_s, 200)), this, IDC_PP_SWINPUTLEVELS);
	m_cbSwInputLevels.AddString(_T("TV (16-235)")); // ResStr(IDS_VDF_RANGE_TV)
	m_cbSwInputLevels.AddString(_T("PC (0-255)"));  // ResStr(IDS_VDF_RANGE_PC)
	m_cbSwInputLevels.AddString(_T("Auto"));        // ResStr(IDS_VDF_RANGE_AUTO)
	p.y += h25;

	// Output levels
	m_txtSwOutputLevels.Create(_T("Output levels:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h16;
	m_cbSwOutputLevels.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p, CSize(width_s, 200)), this, IDC_PP_SWOUTPUTLEVELS);
	m_cbSwOutputLevels.AddString(_T("TV (16-235)")); // ResStr(IDS_VDF_RANGE_TV)
	m_cbSwOutputLevels.AddString(_T("PC (0-255)"));  // ResStr(IDS_VDF_RANGE_PC)
	m_cbSwOutputLevels.AddString(_T("Auto"));        // ResStr(IDS_VDF_RANGE_AUTO)
	p.y += h25;

	// Software version, useful info for stand-alone filter
	m_strSwVersion.Format(_T("v%d.%d.%d.%d"),MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_STATUS,MPC_VERSION_PATCH);
	m_txtSwVersion.Create(m_strSwVersion, WS_DISABLED|WS_VISIBLE|WS_CHILD|SS_RIGHT|SS_PATHELLIPSIS, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	//

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	CorrectComboListWidth(m_cbDXVACompatibilityCheck);
	CorrectComboListWidth(m_cbDiscardMode);

	if (m_pMDF) {
		m_pMDF->SetDialogHWND(this->GetSafeHwnd());

		m_cbThreadNumber.SetCurSel		(m_pMDF->GetThreadNumber());
		m_cbDiscardMode.SetCurSel		(FindDiscardIndex (m_pMDF->GetDiscardMode()));
		m_cbErrorRecognition.SetCurSel	(FindErrRecognitionIndex (m_pMDF->GetErrorRecognition()));
		m_cbIDCTAlgo.SetCurSel			(m_pMDF->GetIDCTAlgo());

		m_cbARMode.SetCheck(m_pMDF->GetARMode());

		m_cbDXVACompatibilityCheck.SetCurSel(m_pMDF->GetDXVACheckCompatibility());
		m_cbDXVA_SD.SetCheck(m_pMDF->GetDXVA_SD());

		// === New swscaler options
		for (int i=0; i<6; i++) {
			m_lstSwOutputFormats.SetCheck(i, m_nSwChecked[i]);
		}
		m_cbSwChromaToRGB.SetCurSel(m_pMDF->GetSwChromaToRGB());
		m_cbSwResizeMethodBE.SetCurSel(m_pMDF->GetSwResizeMethodBE());
		m_cbSwColorspace.SetCurSel(m_pMDF->GetSwColorspace());
		m_cbSwInputLevels.SetCurSel(m_pMDF->GetSwInputLevels());
		m_cbSwOutputLevels.SetCurSel(m_pMDF->GetSwOutputLevels());

		unsigned __int64 m_nOutCsp = m_pMDF->GetOutputFormat();

		m_lstSwOutputFormats.EnableWindow(m_nOutCsp != FF_CSP_UNSUPPORTED);
		m_cbSwOutputFormatUp.EnableWindow(m_nOutCsp != FF_CSP_UNSUPPORTED);
		m_cbSwOutputFormatDown.EnableWindow(m_nOutCsp != FF_CSP_UNSUPPORTED);

		m_cbSwResizeMethodBE.EnableWindow(m_nOutCsp == 0 || m_nOutCsp != FF_CSP_UNSUPPORTED);
		m_cbSwChromaToRGB.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		m_cbSwColorspace.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		m_cbSwInputLevels.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		m_cbSwOutputLevels.EnableWindow(m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp));
		//
	}

	return true;
}

void CMPCVideoDecSettingsWnd::OnDeactivate()
{
}

bool CMPCVideoDecSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
		m_pMDF->SetThreadNumber		(m_cbThreadNumber.GetCurSel());
		m_pMDF->SetDiscardMode		(g_AVDiscard[m_cbDiscardMode.GetCurSel()]);
		m_pMDF->SetErrorRecognition  (g_AVErrRecognition[m_cbErrorRecognition.GetCurSel()]);
		m_pMDF->SetIDCTAlgo			(m_cbIDCTAlgo.GetCurSel());

		m_pMDF->SetARMode(m_cbARMode.GetCheck());

		m_pMDF->SetDXVACheckCompatibility(m_cbDXVACompatibilityCheck.GetCurSel());

		m_pMDF->SetDXVA_SD(m_cbDXVA_SD.GetCheck());

		// === New swscaler options
		int m_nSwRefresh = 0; // no refresh

		if (m_cbSwChromaToRGB.GetCurSel() != m_pMDF->GetSwChromaToRGB() || 
		m_cbSwResizeMethodBE.GetCurSel() != m_pMDF->GetSwResizeMethodBE() || 
		m_cbSwColorspace.GetCurSel() != m_pMDF->GetSwColorspace() || 
		m_cbSwInputLevels.GetCurSel() != m_pMDF->GetSwInputLevels() || 
		m_cbSwOutputLevels.GetCurSel() != m_pMDF->GetSwOutputLevels()) {
			m_nSwRefresh = 1;	// soft refresh - signal new swscaler colorspace details
		}

		int	m_nSwOldIndex[6];
		int	m_nSwOldChecked[6];
		int nSwOldOF = m_pMDF ? m_pMDF->GetSwOutputFormats() : 0;
		// get the output formats order from the DWORD nibbles extracting literal values [0x00543210] = 0,1,2,3,4,5
		// get the output formats with the checked flag (8) from the DWORD nibbles [0x0054ba98] = 0,1,2,3
		for (int i=0; i<6; i++){
			if (nSwOldOF == 0) {
				m_nSwOldIndex[i] = i;
				m_nSwOldChecked[i]	= 1;
			} else {
				m_nSwOldIndex[i] = ( nSwOldOF & ( 0x0000000F << (4*i) ) ) >> (4*i);
				m_nSwOldChecked[i] = (m_nSwOldIndex[i] & 8) != 0;
				m_nSwOldIndex[i]	&= ~8;
			}
			if (m_nSwIndex[i] != m_nSwOldIndex[i] || m_lstSwOutputFormats.GetCheck(i) != m_nSwOldChecked[i])
				m_nSwRefresh = 2; // hard refresh - signal new output format
		}
 		m_pMDF->SetSwRefresh(m_nSwRefresh);

		// set the output formats order in the DWORD nibbles with literal values [0x00543210] = 0,1,2,3,4,5
		// set the output formats with the checked flag (8) in the DWORD nibbles [0x0054ba98] = 0,1,2,3
		int nSwOF = 0;
		for (int i=0; i<6; i++) {
			m_nSwChecked[i] = m_lstSwOutputFormats.GetCheck(i);
		}
		for (int i=0; i<6; i++) {
			nSwOF = (nSwOF<<4) | (m_nSwIndex[5-i] + m_nSwChecked[5-i]*8);
		}
   		m_pMDF->SetSwOutputFormats(nSwOF);

		m_pMDF->SetSwChromaToRGB(m_cbSwChromaToRGB.GetCurSel());
		m_pMDF->SetSwResizeMethodBE(m_cbSwResizeMethodBE.GetCurSel());
		m_pMDF->SetSwColorspace(m_cbSwColorspace.GetCurSel());
		m_pMDF->SetSwInputLevels(m_cbSwInputLevels.GetCurSel());
		m_pMDF->SetSwOutputLevels(m_cbSwOutputLevels.GetCurSel());
		//

		m_pMDF->Apply();
	}

	return true;
}


BEGIN_MESSAGE_MAP(CMPCVideoDecSettingsWnd, CInternalPropertyPageWnd)

	// === New swscaler options
	ON_BN_CLICKED( IDC_PP_SWOUTPUTFORMATUP, OnClickedSwOutputFormatUp)
	ON_BN_CLICKED( IDC_PP_SWOUTPUTFORMATDOWN, OnClickedSwOutputFormatDown)
	//

END_MESSAGE_MAP()

// === New swscaler options
void CMPCVideoDecSettingsWnd::OnClickedSwOutputFormatUp()
{
	int pos = m_lstSwOutputFormats.GetCurSel();
	int count = m_lstSwOutputFormats.GetCount();
	int selected;	int selectedUp; 
	CString text;
	if ((pos != LB_ERR) && (count > 1) && (pos-1 >= 0)) {
		selected = m_lstSwOutputFormats.GetCheck(pos);
		selectedUp = m_lstSwOutputFormats.GetCheck(pos-1);
		m_lstSwOutputFormats.GetText(pos, text);        
		m_lstSwOutputFormats.DeleteString(pos);
		m_lstSwOutputFormats.InsertString(pos-1, text);
		m_lstSwOutputFormats.SetCheck(pos-1, selected);
		m_lstSwOutputFormats.SetCheck(pos, selectedUp);
		m_lstSwOutputFormats.SetCurSel(pos-1);
		std::swap(m_nSwIndex[pos],m_nSwIndex[pos-1]);
	}
}

void CMPCVideoDecSettingsWnd::OnClickedSwOutputFormatDown()
{
	int pos		= m_lstSwOutputFormats.GetCurSel();
	int count	= m_lstSwOutputFormats.GetCount();
	int selected;
	int selectedDown; 
	CString text;

	if ((pos != LB_ERR) && (count > 1) && (pos+1 < count)) {
		selected = m_lstSwOutputFormats.GetCheck(pos);
		selectedDown = m_lstSwOutputFormats.GetCheck(pos+1);
		m_lstSwOutputFormats.GetText(pos,text);
		m_lstSwOutputFormats.DeleteString(pos);
		m_lstSwOutputFormats.InsertString(pos+1,text);
		m_lstSwOutputFormats.SetCheck(pos+1,selected);
		m_lstSwOutputFormats.SetCheck(pos,selectedDown);
		m_lstSwOutputFormats.SetCurSel(pos+1);
		std::swap(m_nSwIndex[pos],m_nSwIndex[pos+1]);
	}
}
//

// ====== Codec filter property page (for standalone filter only)

CMPCVideoDecCodecWnd::CMPCVideoDecCodecWnd()
{
}

bool CMPCVideoDecCodecWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMDF);

	m_pMDF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMDF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMDF) {
		return false;
	}

	return true;
}

void CMPCVideoDecCodecWnd::OnDisconnect()
{
	m_pMDF.Release();
}

typedef struct {
	MPC_VIDEO_CODEC	CodecId;
	LPCTSTR			CodeName;
} MPCFILTER_VIDEO_CODECS;

MPCFILTER_VIDEO_CODECS mpc_codecs[] = {
	{MPCVD_H264_DXVA,	_T("H.264/AVC (DXVA)")},
	{MPCVD_MPEG2_DXVA,	_T("MPEG2 (DXVA)")},
	{MPCVD_VC1_DXVA,	_T("VC1 (DXVA)")},
	{MPCVD_WMV3_DXVA,	_T("WMV3 (DXVA)")},
	{MPCVD_AMVV,		_T("AMV video")},
	{MPCVD_PRORES,		_T("Apple ProRes")},
	{MPCVD_BINKV,		_T("Bink video")},
	{MPCVD_CLLC,		_T("Canopus Lossless")},
	{MPCVD_DIRAC,		_T("Dirac")},
	{MPCVD_DIVX,		_T("DivX")},
	{MPCVD_DV,			_T("DV video")},
	{MPCVD_FLASH,		_T("FLV1/4")},
	{MPCVD_H263,		_T("H.263")},
	{MPCVD_H264,		_T("H.264/AVC (FFmpeg)")},
	{MPCVD_INDEO,		_T("Indeo 3/4/5")},
	{MPCVD_LAGARITH,	_T("Lagarith")},
	{MPCVD_MJPEG,		_T("MJPEG")},
	{MPCVD_MSMPEG4,		_T("MS-MPEG4")},
	{MPCVD_PNG,			_T("PNG")},
	{MPCVD_SCREC,		_T("Screen Recorder (CSCD/TSCC/QTRle)")},
	{MPCVD_SVQ3,		_T("SVQ1/3")},
	{MPCVD_THEORA,		_T("Theora")},
	{MPCVD_UTVD,		_T("Ut video")},
	{MPCVD_VC1,			_T("VC1 (FFmpeg)")},
	{MPCVD_VP356,		_T("VP3/5/6")},
	{MPCVD_VP8,			_T("VP8")},
	{MPCVD_WMV,			_T("WMV1/2/3")},
	{MPCVD_XVID,		_T("Xvid/MPEG-4")},
	{MPCVD_RV,			_T("Real Video")}
};

bool CMPCVideoDecCodecWnd::OnActivate()
{
	DWORD				dwStyle	= WS_VISIBLE|WS_CHILD|WS_BORDER;
	int					nPos	= 0;
	MPC_VIDEO_CODEC		nActiveCodecs = (MPC_VIDEO_CODEC)(m_pMDF ? m_pMDF->GetActiveCodecs() : 0);

	m_grpSelectedCodec.Create (_T("Selected codecs"), WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  10, 330, 280), this, (UINT)IDC_STATIC);

	m_lstCodecs.Create (dwStyle | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP, CRect (20,30, 320, 270), this, 0);

	for (size_t i = 0; i < _countof(mpc_codecs); i++) {
		m_lstCodecs.AddString(mpc_codecs[i].CodeName);
		m_lstCodecs.SetCheck(nPos++, (nActiveCodecs & mpc_codecs[i].CodecId) != 0);
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	return true;
}

void CMPCVideoDecCodecWnd::OnDeactivate()
{
}

bool CMPCVideoDecCodecWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
		int nActiveCodecs = 0;
		int nPos		  = 0;

		for (size_t i = 0; i < _countof(mpc_codecs); i++) {
			if (m_lstCodecs.GetCheck(nPos++)) {
				nActiveCodecs |= mpc_codecs[i].CodecId;
			}
		}

		m_pMDF->SetActiveCodecs ((MPC_VIDEO_CODEC)nActiveCodecs);

		m_pMDF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMPCVideoDecCodecWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()
