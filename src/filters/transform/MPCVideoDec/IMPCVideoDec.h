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

#define CODEC_H264			(1ULL << 0)
#define CODEC_MPEG1			(1ULL << 1)
#define CODEC_MPEG2			(1ULL << 2)
#define CODEC_VC1			(1ULL << 3)
#define CODEC_MSMPEG4		(1ULL << 4)
#define CODEC_XVID			(1ULL << 5)
#define CODEC_DIVX			(1ULL << 6)
#define CODEC_WMV			(1ULL << 7)
#define CODEC_HEVC			(1ULL << 8)
#define CODEC_VP356			(1ULL << 9)
#define CODEC_VP89			(1ULL << 10)
#define CODEC_THEORA		(1ULL << 11)
#define CODEC_MJPEG			(1ULL << 12)
#define CODEC_DV			(1ULL << 13)
#define CODEC_LAGARITH		(1ULL << 14)
#define CODEC_PRORES		(1ULL << 15)
#define CODEC_CLLC			(1ULL << 16)
#define CODEC_SCREC			(1ULL << 17)
#define CODEC_INDEO			(1ULL << 18)
#define CODEC_H263			(1ULL << 19)
#define CODEC_SVQ3			(1ULL << 20)
#define CODEC_REALV			(1ULL << 21)
#define CODEC_DIRAC			(1ULL << 22)
#define CODEC_BINKV			(1ULL << 23)
#define CODEC_AMVV			(1ULL << 24)
#define CODEC_FLASH			(1ULL << 25)
#define CODEC_UTVD			(1ULL << 26)
#define CODEC_PNG			(1ULL << 27)
#define CODEC_V210			(1ULL << 28)
// dxva codecs
#define CODEC_H264_DXVA		(1ULL << 56)
#define CODEC_MPEG2_DXVA	(1ULL << 57)
#define CODEC_VC1_DXVA		(1ULL << 58)
#define CODEC_WMV3_DXVA		(1ULL << 59)

#define CODECS_SOFT (CODEC_H264|CODEC_MPEG1|CODEC_MPEG2|CODEC_VC1|CODEC_MSMPEG4|CODEC_XVID|CODEC_DIVX|CODEC_WMV|CODEC_HEVC|CODEC_VP356|CODEC_VP89|CODEC_THEORA|CODEC_MJPEG|CODEC_DV|CODEC_LAGARITH|CODEC_PRORES|CODEC_CLLC|CODEC_SCREC|CODEC_INDEO|CODEC_H263|CODEC_SVQ3|CODEC_REALV|CODEC_DIRAC|CODEC_BINKV|CODEC_AMVV|CODEC_FLASH|CODEC_UTVD|CODEC_PNG|CODEC_V210)
#define CODECS_DXVA (CODEC_H264_DXVA|CODEC_MPEG2_DXVA|CODEC_VC1_DXVA|CODEC_WMV3_DXVA)
#define CODECS_ALL  (CODECS_SOFT|CODECS_DXVA)

typedef enum MPC_DEINTERLACING_FLAGS {
	AUTO,
	TOPFIELD,
	BOTTOMFIELD,
	PROGRESSIVE
};

enum MPCPixelFormat {
	// YUV formats are grouped according to luma bit depth and sorted in descending order of quality.
	PixFmt_None = -1,
	// YUV 8 bit
	PixFmt_AYUV,  // 24(32) bit, 4:4:4
	PixFmt_YUY2,  // 16 bit, 4:2:2
	PixFmt_NV12,  // 12 bit, 4:2:0
	PixFmt_YV12,  // 12 bit, 4:2:0
	// YUV 10 bit
	PixFmt_Y410,  // 30(32) bit, 4:4:4
	PixFmt_P210,  // 20(32) bit, 4:2:2
	PixFmt_P010,  // 15(24) bit, 4:2:0
	// YUV 16 bit
	PixFmt_Y416,  // 48(64) bit, 4:4:4
	PixFmt_P216,  // 32 bit, 4:2:2
	PixFmt_P016,  // 24 bit, 4:2:0
	// RGB
	PixFmt_RGB32, // 24(32) bit
	PixFmt_count
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

	STDMETHOD(SetSwPixelFormat(MPCPixelFormat pf, bool enable)) = 0;
	STDMETHOD_(bool, GetSwPixelFormat(MPCPixelFormat pf)) = 0;

	STDMETHOD(SetSwPreset(int nValue)) = 0;
	STDMETHOD_(int, GetSwPreset()) = 0;

	STDMETHOD(SetSwStandard(int nValue)) = 0;
	STDMETHOD_(int, GetSwStandard()) = 0;

	STDMETHOD(SetSwRGBLevels(int nValue)) = 0;
	STDMETHOD_(int, GetSwRGBLevels()) = 0;

	STDMETHOD_(int, GetColorSpaceConversion()) = 0;
	//

	STDMETHOD(GetOutputMediaType(CMediaType* pmt)) = 0;
};

interface __declspec(uuid("F0ABC515-19ED-4D65-9D5F-59E36AE7F2AF"))
IMPCVideoDecFilter2 :
public IUnknown {
	STDMETHOD_(int, GetFrameType()) = 0;
};
