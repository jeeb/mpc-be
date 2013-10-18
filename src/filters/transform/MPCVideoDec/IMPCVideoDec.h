/*
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

// Internal codec list (use to enable/disable codec in standalone mode)

#define MPCVD_H264			(1ULL << 0)
#define MPCVD_VC1			(1ULL << 1)
#define MPCVD_XVID			(1ULL << 2)
#define MPCVD_DIVX			(1ULL << 3)
#define MPCVD_WMV			(1ULL << 4)
#define MPCVD_MSMPEG4		(1ULL << 5)
#define MPCVD_H263			(1ULL << 6)
#define MPCVD_SVQ3			(1ULL << 7)
#define MPCVD_THEORA		(1ULL << 8)
#define MPCVD_AMVV			(1ULL << 9)
#define MPCVD_FLASH			(1ULL << 10)
#define MPCVD_H264_DXVA		(1ULL << 11)
#define MPCVD_VC1_DXVA		(1ULL << 12)
#define MPCVD_VP356			(1ULL << 13)
#define MPCVD_VP8			(1ULL << 14)
#define MPCVD_MJPEG			(1ULL << 15)
#define MPCVD_INDEO			(1ULL << 16)
#define MPCVD_RV			(1ULL << 17)
#define MPCVD_WMV3_DXVA		(1ULL << 18)
#define MPCVD_MPEG2_DXVA	(1ULL << 19)
#define MPCVD_DIRAC			(1ULL << 20)
#define MPCVD_DV			(1ULL << 21)
#define MPCVD_UTVD			(1ULL << 22)
#define MPCVD_SCREC			(1ULL << 23)
#define MPCVD_LAGARITH		(1ULL << 24)
#define MPCVD_PRORES		(1ULL << 25)
#define MPCVD_BINKV			(1ULL << 26)
#define MPCVD_PNG			(1ULL << 27)
#define MPCVD_CLLC			(1ULL << 28)
#define MPCVD_V210			(1ULL << 29)
#define MPCVD_MPEG2			(1ULL << 30)
#define MPCVD_MPEG1			(1ULL << 31)
#define MPCVD_HEVC			(1ULL << 32)

typedef enum MPC_DEINTERLACING_FLAGS {
	AUTO,
	TOPFIELD,
	BOTTOMFIELD,
	PROGRESSIVE
};

interface __declspec(uuid("CDC3B5B3-A8B0-4c70-A805-9FC80CDEF262"))
IMPCVideoDecFilter :
public IUnknown {
	STDMETHOD(Apply()) = 0;

	STDMETHOD(SetThreadNumber(int nValue)) = 0;
	STDMETHOD_(int, GetThreadNumber()) = 0;

	STDMETHOD(SetDiscardMode(int nValue)) = 0;
	STDMETHOD_(int, GetDiscardMode()) = 0;

	STDMETHOD(SetDeinterlacing(MPC_DEINTERLACING_FLAGS nValue)) = 0;
	STDMETHOD_(MPC_DEINTERLACING_FLAGS, GetDeinterlacing()) = 0;

	STDMETHOD_(GUID*, GetDXVADecoderGuid()) = 0;

	STDMETHOD(SetActiveCodecs(ULONGLONG nValue)) = 0;
	STDMETHOD_(ULONGLONG, GetActiveCodecs()) = 0;

	STDMETHOD_(LPCTSTR, GetVideoCardDescription()) = 0;

	STDMETHOD(SetARMode(int nValue)) = 0;
	STDMETHOD_(int, GetARMode()) = 0;

	STDMETHOD(SetDXVACheckCompatibility(int nValue)) = 0;
	STDMETHOD_(int, GetDXVACheckCompatibility()) = 0;

	STDMETHOD(SetDXVA_SD(int nValue)) = 0;
	STDMETHOD_(int, GetDXVA_SD()) = 0;

	// === New swscaler options
	STDMETHOD(SetSwRefresh(int nValue)) = 0;

	STDMETHOD(SetSwFormats(CString SwFormatsStr)) = 0;
	//STDMETHOD(SetSwFormatState(unsigned int index, int nCheck)) = 0;
	STDMETHOD_(int, GetSwFormatState(unsigned int index)) = 0;
	STDMETHOD_(LPCTSTR, GetSwFormatName(unsigned int index)) = 0;

	STDMETHOD(SetSwPreset(int nValue)) = 0;
	STDMETHOD_(int, GetSwPreset()) = 0;

	STDMETHOD(SetSwStandard(int nValue)) = 0;
	STDMETHOD_(int, GetSwStandard()) = 0;

	STDMETHOD(SetSwInputLevels(int nValue)) = 0;
	STDMETHOD_(int, GetSwInputLevels()) = 0;

	STDMETHOD(SetSwOutputLevels(int nValue)) = 0;
	STDMETHOD_(int, GetSwOutputLevels()) = 0;
	//

	STDMETHOD(SetDialogHWND(HWND nValue)) = 0;
	STDMETHOD_(unsigned __int64, GetOutputFormat()) = 0;

	STDMETHOD(GetOutputMediaType(CMediaType* pmt)) = 0;
};

interface __declspec(uuid("F0ABC515-19ED-4D65-9D5F-59E36AE7F2AF"))
IMPCVideoDecFilter2 :
public IUnknown {
	STDMETHOD_(int, GetFrameType()) = 0;
};

interface __declspec(uuid("EAAE8911-3EB7-49F4-A255-67D84651EE8F"))
IMPCVideoDecFilterCodec :
public IUnknown {
	STDMETHOD(SetFFMpegCodec(int nCodec, bool bEnabled)) = 0;
	STDMETHOD(SetDXVACodec(int nCodec, bool bEnabled)) = 0;
};
