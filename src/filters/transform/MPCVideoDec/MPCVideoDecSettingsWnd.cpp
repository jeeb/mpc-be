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

#include <ffmpeg/PODtypes.h>
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/ffImgfmt.h>

// ==>>> Resource identifier from "resource.h" present in mplayerc project!
#define ResStr(id) CString(MAKEINTRESOURCE(id))

#define LEFT_SPACING		25
#define VERTICAL_SPACING	25

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
	int		nPosY	= 10;
	GUID*	DxvaGui = NULL;

	// === New swscaler options
	int nPosX   = 10;
	int nSizeX  = 80;
	int nSizeY  = 16;
	m_strSwVersion.Format(_T("v%d.%d.%d.%d"),MPC_VERSION_MAJOR,MPC_VERSION_MINOR,MPC_VERSION_STATUS,MPC_VERSION_PATCH);
	static	wchar_t *SwOutputFormatNames[] = {_T("NV12 (default)"), _T("YV12"), _T("YUY2"), _T("RGB32"), _T("RGB16"), _T("RGB15")};
	int nSwOF = m_pMDF ? m_pMDF->GetSwOutputFormats() : 0;
	// get the output formats order from the DWORD nibbles extracting literal values [0x00543210] = 0,1,2,3,4,5
	// get the output formats with the checked flag (8) from the DWORD nibbles [0x0054ba98] = 0,1,2,3
	for (int i=0; i<6; i++){
		if (nSwOF == 0) {
			m_nSwIndex[i] = i;
			m_nSwChecked[i]	= 1;
		} else {
			m_nSwIndex[i] = ( nSwOF & ( 0x0000000F << (4*i) ) ) >> (4*i);
			m_nSwChecked[i] = (m_nSwIndex[i] & 8) != 0;
			m_nSwIndex[i]	&= ~8;
		}
	}
	//

	m_grpFFMpeg.Create (ResStr (IDS_VDF_FFSETTINGS), WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  nPosY, 350, nPosY+150), this, (UINT)IDC_STATIC);

	// Decoding threads
	nPosY += VERTICAL_SPACING;
	m_txtThreadNumber.Create (ResStr (IDS_VDF_THREADNUMBER), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 220, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbThreadNumber.Create  (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (230,  nPosY-4, 290, nPosY+90), this, IDC_PP_THREAD_NUMBER);
	m_cbThreadNumber.AddString (ResStr (IDS_VDF_IDCT_AUTO));
	CString ThreadNumberStr;
	for (int i=0; i<16; i++) {
		ThreadNumberStr.Format		(_T("%d"), i+1);
		m_cbThreadNumber.AddString	(ThreadNumberStr);
	}

	// H264 deblocking mode
	nPosY += VERTICAL_SPACING;
	m_txtDiscardMode.Create	(ResStr (IDS_VDF_SKIPDEBLOCK), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 220, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbDiscardMode.Create  (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (230,  nPosY-4, 345, nPosY+90), this, IDC_PP_DISCARD_MODE);
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONE));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_DEFAULT));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONREF));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_BIDIR));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONKFRM));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_ALL));

	// Error recognition
	nPosY += VERTICAL_SPACING;
	m_txtErrorRecognition.Create (ResStr (IDS_VDF_ERROR_RECOGNITION), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 220, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbErrorRecognition.Create  (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (230,  nPosY-4, 345, nPosY+90), this, IDC_PP_DISCARD_MODE);
	m_cbErrorRecognition.AddString (ResStr (IDS_VDF_ERR_CAREFUL));
	m_cbErrorRecognition.AddString (ResStr (IDS_VDF_ERR_COMPLIANT));
	m_cbErrorRecognition.AddString (ResStr (IDS_VDF_ERR_AGGRESSIVE));

	// IDCT Algo
	nPosY += VERTICAL_SPACING;
	m_txtIDCTAlgo.Create (ResStr (IDS_VDF_IDCT_ALGO), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 220, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbIDCTAlgo.Create  (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (230,  nPosY-4, 345, nPosY+90), this, IDC_PP_DISCARD_MODE);
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_AUTO));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_LIBMPEG2));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_SIMPLE_MMX));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_XVID));
	m_cbIDCTAlgo.AddString (ResStr (IDS_VDF_IDCT_SIMPLE));

	nPosY += VERTICAL_SPACING;
	m_cbARMode.Create (ResStr (IDS_VDF_AR_MODE), WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect (LEFT_SPACING,  nPosY, 345, nPosY+15), this, IDC_PP_AR);
	m_cbARMode.SetCheck(FALSE);

	nPosY = 170;

	m_grpDXVA.Create   (ResStr (IDS_VDF_DXVA_SETTING),   WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10, nPosY, 350, nPosY+135), this, (UINT)IDC_STATIC);

	// DXVA Compatibility check
	nPosY += VERTICAL_SPACING;
	m_txtDXVACompatibilityCheck.Create (ResStr (IDS_VDF_DXVACOMPATIBILITY), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 225, nPosY+15), this, (UINT)IDC_STATIC);
	m_cbDXVACompatibilityCheck.Create  (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (230,  nPosY-4, 345, nPosY+90), this, IDC_PP_DXVA_CHECK);
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_FULLCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_LEVELCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_REFCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_ALLCHECK));

	nPosY += VERTICAL_SPACING;
	m_cbDXVA_SD.Create (ResStr (IDS_VDF_DXVA_SD), WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect (LEFT_SPACING,  nPosY, 345, nPosY+15), this, IDC_PP_DXVA_SD);
	m_cbDXVA_SD.SetCheck (FALSE);

	// DXVA mode
	nPosY += VERTICAL_SPACING;
	m_txtDXVAMode.Create (ResStr (IDS_VDF_DXVA_MODE), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 120, nPosY+15), this, (UINT)IDC_STATIC);
	m_edtDXVAMode.Create (WS_CHILD|WS_VISIBLE|WS_DISABLED, CRect (120,  nPosY, 345, nPosY+20), this, 0);

	// Video card description
	nPosY += VERTICAL_SPACING;
	m_txtVideoCardDescription.Create (ResStr (IDS_VDF_VIDEOCARD), WS_VISIBLE|WS_CHILD, CRect (LEFT_SPACING,  nPosY, 120, nPosY+15), this, (UINT)IDC_STATIC);
	m_edtVideoCardDescription.Create (ES_MULTILINE|WS_CHILD|WS_VISIBLE|WS_DISABLED, CRect (120,  nPosY, 345, nPosY+30), this, 0);
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

	// Software output formats
	nPosY = 10;
	nPosX = LEFT_SPACING + 340;
	nSizeX = 90; // 90
	m_txtSwOutputFormats.Create (_T("Output formats:"), WS_VISIBLE|WS_CHILD, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, (UINT)IDC_STATIC);
	nPosY += 16;
	m_lstSwOutputFormats.Create (WS_VISIBLE|WS_CHILD|WS_BORDER|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|WS_VSCROLL, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+90), this, 0);
	for (int i=0; i<6; i++) {
		m_lstSwOutputFormats.AddString (SwOutputFormatNames[m_nSwIndex[i]]);
	}
	// Software Output formats order
	nPosY += 80;
	m_cbSwOutputFormatUp.Create (_T("<"), WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, CRect (nPosX+nSizeX-32, nPosY, nPosX+nSizeX-16, nPosY+nSizeY), this, IDC_PP_SWOUTPUTFORMATUP);
	m_cbSwOutputFormatDown.Create (_T(">"), WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, CRect (nPosX+nSizeX-16, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, IDC_PP_SWOUTPUTFORMATDOWN);

	// Resize Method
	nPosY += 26; //20
	m_txtSwResizeMethodBE.Create (_T("Resize Method:"), WS_VISIBLE|WS_CHILD, CRect (nPosX,  nPosY, nPosX+nSizeX, nPosY+nSizeY), this, (UINT)IDC_STATIC);
	nPosY += 16;
	m_cbSwResizeMethodBE.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, IDC_PP_RESIZEMETHODBE);
	m_cbSwResizeMethodBE.AddString (_T("Area")); // ResStr(IDS_VDF_CHR_AREA)
	m_cbSwResizeMethodBE.AddString (_T("Bicubic")); // ResStr(IDS_VDF_CHR_BICUBIC)
//	m_cbSwResizeMethodBE.AddString (_T("Bicublin")); // ResStr(IDS_VDF_CHR_BICUBLIN) //temp rem
	m_cbSwResizeMethodBE.AddString (_T("Bilinear")); // ResStr(IDS_VDF_CHR_BILINEAR)
	m_cbSwResizeMethodBE.AddString (_T("Fast Bilinear")); // ResStr(IDS_VDF_CHR_FAST_BILINEAR)
	m_cbSwResizeMethodBE.AddString (_T("Gauss")); // ResStr(IDS_VDF_CHR_FULL_GAUSS)
	m_cbSwResizeMethodBE.AddString (_T("Lanczos")); // ResStr(IDS_VDF_CHR_FULL_LANCZOS)
	m_cbSwResizeMethodBE.AddString (_T("Point")); // ResStr(IDS_VDF_CHR_FULL_POINT)
	m_cbSwResizeMethodBE.AddString (_T("Sinc")); // ResStr(IDS_VDF_CHR_FULL_SINC)
	m_cbSwResizeMethodBE.AddString (_T("Spline")); // ResStr(IDS_VDF_CHR_FULL_SPLINE)
	m_cbSwResizeMethodBE.AddString (_T("X")); // ResStr(IDS_VDF_CHR_FULL_X)
	
	// Chroma options
	nPosY += 26; //20
	m_txtSwChromaToRGB.Create (_T("Chroma to RGB:"), WS_VISIBLE|WS_CHILD, CRect (nPosX,  nPosY, nPosX+nSizeX, nPosY+nSizeY), this, (UINT)IDC_STATIC);
	nPosY += 16;
	m_cbSwChromaToRGB.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, IDC_PP_SWCHROMATORGB);
	m_cbSwChromaToRGB.AddString (_T("Fast")); // ResStr(IDS_VDF_CHR_FAST)
	m_cbSwChromaToRGB.AddString (_T("Normal")); // ResStr(IDS_VDF_CHR_NORMAL)
//	m_cbSwChromaToRGB.AddString (_T("High")); // ResStr(IDS_VDF_CHR_HIGH) //temp rem
	m_cbSwChromaToRGB.AddString (_T("Full")); // ResStr(IDS_VDF_CHR_FULL)	
	
	// Software Colorspace
	nPosY += 26;
	m_txtSwColorspace.Create (_T("Colorspace:"), WS_VISIBLE|WS_CHILD, CRect (nPosX,  nPosY, nPosX+nSizeX, nPosY+nSizeY), this, (UINT)IDC_STATIC);
	nPosY += 16;
	m_cbSwColorspace.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, IDC_PP_SWCOLORSPACE);
	m_cbSwColorspace.AddString (_T("SD (BT.601)")); // ResStr(IDS_VDF_COLOR_HD)
	m_cbSwColorspace.AddString (_T("HD (BT.709)")); // ResStr(IDS_VDF_COLOR_SD)
	m_cbSwColorspace.AddString (_T("Auto")); // ResStr(IDS_VDF_COLOR_AUTO)
	// Input levels
	nPosY += 26;
	m_txtSwInputLevels.Create (_T("Input levels:"), WS_VISIBLE|WS_CHILD, CRect (nPosX,  nPosY, nPosX+nSizeX, nPosY+nSizeY), this, (UINT)IDC_STATIC);
	nPosY += 16;
	m_cbSwInputLevels.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, IDC_PP_SWINPUTLEVELS);
	m_cbSwInputLevels.AddString (_T("TV (16-235)")); // ResStr(IDS_VDF_RANGE_TV)
	m_cbSwInputLevels.AddString (_T("PC (0-255)")); // ResStr(IDS_VDF_RANGE_PC)
	m_cbSwInputLevels.AddString (_T("Auto")); // ResStr(IDS_VDF_RANGE_AUTO)
	// Output levels
	nPosY += 26;
	m_txtSwOutputLevels.Create (_T("Output levels:"), WS_VISIBLE|WS_CHILD, CRect (nPosX,  nPosY, nPosX+nSizeX, nPosY+nSizeY), this, (UINT)IDC_STATIC);
	nPosY += 16;
	m_cbSwOutputLevels.Create (WS_VISIBLE|WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, CRect (nPosX, nPosY, nPosX+nSizeX, nPosY+nSizeY), this, IDC_PP_SWOUTPUTLEVELS);
	m_cbSwOutputLevels.AddString (_T("TV (16-235)")); // ResStr(IDS_VDF_RANGE_TV)
	m_cbSwOutputLevels.AddString (_T("PC (0-255)")); // ResStr(IDS_VDF_RANGE_PC)
	m_cbSwOutputLevels.AddString (_T("Auto")); // ResStr(IDS_VDF_RANGE_AUTO)
	// Software version, useful info for stand-alone filter
	nPosY += 26;
	m_txtSwVersion.Create (m_strSwVersion, WS_DISABLED|WS_VISIBLE|WS_CHILD|SS_CENTER|SS_PATHELLIPSIS, CRect (nPosX-6,  nPosY, nPosX+nSizeX+6, nPosY+nSizeY), this, (UINT)IDC_STATIC);
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

bool CMPCVideoDecCodecWnd::OnActivate()
{
	DWORD				dwStyle = WS_VISIBLE|WS_CHILD|WS_BORDER;
	int					nPos	= 0;
	MPC_VIDEO_CODEC		nActiveCodecs = (MPC_VIDEO_CODEC)(m_pMDF ? m_pMDF->GetActiveCodecs() : 0);

	m_grpSelectedCodec.Create (_T("Selected codecs"), WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  10, 330, 280), this, (UINT)IDC_STATIC);

	m_lstCodecs.Create (dwStyle | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP, CRect (20,30, 320, 270), this, 0);

	m_lstCodecs.AddString (_T("H.264/AVC (DXVA)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_H264_DXVA) != 0);

	m_lstCodecs.AddString (_T("MPEG2 (DXVA)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_MPEG2_DXVA) != 0);

	m_lstCodecs.AddString (_T("VC1 (DXVA)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_VC1_DXVA) != 0);

	m_lstCodecs.AddString (_T("WMV3 (DXVA)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_WMV3_DXVA) != 0);


	m_lstCodecs.AddString (_T("Bink video"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_BINKV) != 0);

	m_lstCodecs.AddString (_T("AMV video"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_AMVV) != 0);

	m_lstCodecs.AddString (_T("Apple ProRes"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_PRORES) != 0);

	m_lstCodecs.AddString (_T("Dirac"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_DIRAC) != 0);

	m_lstCodecs.AddString (_T("DivX"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_DIVX) != 0);

	m_lstCodecs.AddString (_T("DV video"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_DV) != 0);

	m_lstCodecs.AddString (_T("FLV1/4"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_FLASH) != 0);

	m_lstCodecs.AddString (_T("H.263"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_H263) != 0);

	m_lstCodecs.AddString (_T("H.264/AVC (FFmpeg)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_H264) != 0);

	m_lstCodecs.AddString (_T("Indeo 3/4/5"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_INDEO) != 0);

	m_lstCodecs.AddString (_T("Lagarith"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_LAGARITH) != 0);

	m_lstCodecs.AddString (_T("MJPEG"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_MJPEG) != 0);

	m_lstCodecs.AddString (_T("MS-MPEG4"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_MSMPEG4) != 0);

	m_lstCodecs.AddString (_T("Screen Recorder (CSCD/TSCC/QTRle)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_SCREC) != 0);

	m_lstCodecs.AddString (_T("SVQ1/3"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_SVQ3) != 0);

	m_lstCodecs.AddString (_T("Theora"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_THEORA) != 0);

	m_lstCodecs.AddString (_T("Ut video"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_UTVD) != 0);

	m_lstCodecs.AddString (_T("VC1 (FFmpeg)"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_VC1) != 0);

	m_lstCodecs.AddString (_T("VP3/5/6"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_VP356) != 0);

	m_lstCodecs.AddString (_T("VP8"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_VP8) != 0);

	m_lstCodecs.AddString (_T("WMV1/2/3"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_WMV) != 0);

	m_lstCodecs.AddString (_T("Xvid/MPEG-4"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_XVID) != 0);

	m_lstCodecs.AddString (_T("Real Video"));
	m_lstCodecs.SetCheck  (nPos++, (nActiveCodecs & MPCVD_RV) != 0);

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

		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_H264_DXVA;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_MPEG2_DXVA;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_VC1_DXVA;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_WMV3_DXVA;
		}

		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_AMVV;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_BINKV;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_PRORES;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_DIRAC;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_DIVX;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_DV;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_FLASH;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_H263;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_H264;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_INDEO;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_LAGARITH;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_MJPEG;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_MSMPEG4;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_SCREC;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_SVQ3;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_THEORA;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_UTVD;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_VC1;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_VP356;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_VP8;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_WMV;
		}		
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_XVID;
		}
		if (m_lstCodecs.GetCheck  (nPos++)) {
			nActiveCodecs |= MPCVD_RV;
		}

		m_pMDF->SetActiveCodecs ((MPC_VIDEO_CODEC)nActiveCodecs);

		m_pMDF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMPCVideoDecCodecWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()
