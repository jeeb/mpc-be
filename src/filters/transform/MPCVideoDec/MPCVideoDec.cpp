/*
 * $Id$
 *
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
#include <math.h>
#include <atlbase.h>
#include <MMReg.h>
#include "../../../DSUtil/PODtypes.h"
#include "../../../DSUtil/ff_log.h"

#ifdef REGISTER_FILTER
	#include <InitGuid.h>
#endif

#include "MPCVideoDec.h"
#include "MPCVideoDecOutputPin.h"
#include "CpuId.h"

#include "ffImgfmt.h"
#include "FfmpegContext.h"

#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/SysVersion.h"
#include "../../../DSUtil/MediaTypes.h"
#include "../../parser/MpegSplitter/MpegSplitter.h"
#include "../../parser/OggSplitter/OggSplitter.h"
#include "../../parser/RealMediaSplitter/RealMediaSplitter.h"
#include "DXVADecoderH264.h"
#include <moreuuids.h>

#include "../../../DSUtil/WinAPIUtils.h"

#include "Version.h"

#pragma warning(disable: 4005)
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libswscale/swscale.h>
}
#pragma warning(default: 4005)

#define MAX_SUPPORTED_MODE 5

typedef struct {
	const int		PicEntryNumber_DXVA1;
	const int		PicEntryNumber_DXVA2;
	const UINT		PreferedConfigBitstream;
	const GUID*		Decoder[MAX_SUPPORTED_MODE];
	const WORD		RestrictedMode[MAX_SUPPORTED_MODE];
} DXVA_PARAMS;

typedef struct {
	const CLSID*			clsMinorType;
	const enum AVCodecID	nFFCodec;
	const DXVA_PARAMS*		DXVAModes;

	const int				FFMPEGCode;
	const int				DXVACode;

	int DXVAModeCount() {
		if (!DXVAModes) {
			return 0;
		}
		for (int i=0; i<MAX_SUPPORTED_MODE; i++) {
			if (DXVAModes->Decoder[i] == &GUID_NULL) {
				return i;
			}
		}
		return MAX_SUPPORTED_MODE;
	}
} FFMPEG_CODECS;

// DXVA modes supported for Mpeg2
DXVA_PARAMS		DXVA_Mpeg2 = {
	16,		// PicEntryNumber - DXVA1
	24,		// PicEntryNumber - DXVA2
	1,		// PreferedConfigBitstream
	{ &DXVA2_ModeMPEG2_VLD, &GUID_NULL },
	{ DXVA_RESTRICTED_MODE_UNRESTRICTED, 0 } // Restricted mode for DXVA1?
};

// DXVA modes supported for H264
DXVA_PARAMS		DXVA_H264 = {
	16,		// PicEntryNumber - DXVA1
	24,		// PicEntryNumber - DXVA2
	2,		// PreferedConfigBitstream
	{ &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
	{ DXVA_RESTRICTED_MODE_H264_E, 0}
};

// DXVA modes supported for VC1
DXVA_PARAMS		DXVA_VC1 = {
	16,		// PicEntryNumber - DXVA1
	24,		// PicEntryNumber - DXVA2
	1,		// PreferedConfigBitstream
	{ &DXVA2_ModeVC1_D, &GUID_NULL },
	{ DXVA_RESTRICTED_MODE_VC1_D, 0}
};

FFMPEG_CODECS		ffCodecs[] = {
	// Flash video
	{ &MEDIASUBTYPE_FLV1, AV_CODEC_ID_FLV1, NULL, FFM_FLV4, -1 },
	{ &MEDIASUBTYPE_flv1, AV_CODEC_ID_FLV1, NULL, FFM_FLV4, -1 },
	{ &MEDIASUBTYPE_FLV4, AV_CODEC_ID_VP6F, NULL, FFM_FLV4, -1 },
	{ &MEDIASUBTYPE_flv4, AV_CODEC_ID_VP6F, NULL, FFM_FLV4, -1 },
	{ &MEDIASUBTYPE_VP6F, AV_CODEC_ID_VP6F, NULL, FFM_FLV4, -1 },
	{ &MEDIASUBTYPE_vp6f, AV_CODEC_ID_VP6F, NULL, FFM_FLV4, -1 },

	// VP3
	{ &MEDIASUBTYPE_VP30, AV_CODEC_ID_VP3,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_VP31, AV_CODEC_ID_VP3,  NULL, FFM_VP356, -1 },

	// VP5
	{ &MEDIASUBTYPE_VP50, AV_CODEC_ID_VP5,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_vp50, AV_CODEC_ID_VP5,  NULL, FFM_VP356, -1 },

	// VP6
	{ &MEDIASUBTYPE_VP60, AV_CODEC_ID_VP6,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_vp60, AV_CODEC_ID_VP6,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_VP61, AV_CODEC_ID_VP6,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_vp61, AV_CODEC_ID_VP6,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_VP62, AV_CODEC_ID_VP6,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_vp62, AV_CODEC_ID_VP6,  NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_VP6A, AV_CODEC_ID_VP6A, NULL, FFM_VP356, -1 },
	{ &MEDIASUBTYPE_vp6a, AV_CODEC_ID_VP6A, NULL, FFM_VP356, -1 },

	// VP8
	{ &MEDIASUBTYPE_VP80, AV_CODEC_ID_VP8, NULL, FFM_VP8, -1 },

	// Xvid
	{ &MEDIASUBTYPE_XVID, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_xvid, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_XVIX, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_xvix, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },

	// DivX
	{ &MEDIASUBTYPE_DX50, AV_CODEC_ID_MPEG4, NULL, FFM_DIVX, -1 },
	{ &MEDIASUBTYPE_dx50, AV_CODEC_ID_MPEG4, NULL, FFM_DIVX, -1 },
	{ &MEDIASUBTYPE_DIVX, AV_CODEC_ID_MPEG4, NULL, FFM_DIVX, -1 },
	{ &MEDIASUBTYPE_divx, AV_CODEC_ID_MPEG4, NULL, FFM_DIVX, -1 },
	{ &MEDIASUBTYPE_Divx, AV_CODEC_ID_MPEG4, NULL, FFM_DIVX, -1 },

	// WMV1/2/3
	{ &MEDIASUBTYPE_WMV1, AV_CODEC_ID_WMV1, NULL, FFM_WMV, -1 },
	{ &MEDIASUBTYPE_wmv1, AV_CODEC_ID_WMV1, NULL, FFM_WMV, -1 },
	{ &MEDIASUBTYPE_WMV2, AV_CODEC_ID_WMV2, NULL, FFM_WMV, -1 },
	{ &MEDIASUBTYPE_wmv2, AV_CODEC_ID_WMV2, NULL, FFM_WMV, -1 },
	{ &MEDIASUBTYPE_WMV3, AV_CODEC_ID_WMV3, &DXVA_VC1, FFM_WMV, TRA_DXVA_WMV3 },
	{ &MEDIASUBTYPE_wmv3, AV_CODEC_ID_WMV3, &DXVA_VC1, FFM_WMV, TRA_DXVA_WMV3 },

	// MPEG-2
	{ &MEDIASUBTYPE_MPEG2_VIDEO, AV_CODEC_ID_MPEG2VIDEO, &DXVA_Mpeg2, FFM_MPEG2, TRA_DXVA_MPEG2 },
	{ &MEDIASUBTYPE_MPG2,		 AV_CODEC_ID_MPEG2VIDEO, &DXVA_Mpeg2, FFM_MPEG2, TRA_DXVA_MPEG2 },

	// MPEG-1
	{ &MEDIASUBTYPE_MPEG1Packet,  AV_CODEC_ID_MPEG1VIDEO, NULL, FFM_MPEG1, -1 },
	{ &MEDIASUBTYPE_MPEG1Payload, AV_CODEC_ID_MPEG1VIDEO, NULL, FFM_MPEG1, -1 },

	// MSMPEG-4
	{ &MEDIASUBTYPE_DIV3, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div3, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DVX3, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_dvx3, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MP43, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mp43, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_COL1, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_col1, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV4, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div4, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV5, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div5, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV6, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div6, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_AP41, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_ap41, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MPG3, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mpg3, AV_CODEC_ID_MSMPEG4V3, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV2, AV_CODEC_ID_MSMPEG4V2, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div2, AV_CODEC_ID_MSMPEG4V2, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MP42, AV_CODEC_ID_MSMPEG4V2, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mp42, AV_CODEC_ID_MSMPEG4V2, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MPG4, AV_CODEC_ID_MSMPEG4V1, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mpg4, AV_CODEC_ID_MSMPEG4V1, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV1, AV_CODEC_ID_MSMPEG4V1, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div1, AV_CODEC_ID_MSMPEG4V1, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MP41, AV_CODEC_ID_MSMPEG4V1, NULL, FFM_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mp41, AV_CODEC_ID_MSMPEG4V1, NULL, FFM_MSMPEG4, -1 },

	// AMV Video
	{ &MEDIASUBTYPE_AMVV, AV_CODEC_ID_AMV, NULL, FFM_AMVV, -1 },

	// MJPEG
	{ &MEDIASUBTYPE_MJPG,   AV_CODEC_ID_MJPEG,    NULL, FFM_MJPEG, -1 },
	{ &MEDIASUBTYPE_QTJpeg, AV_CODEC_ID_MJPEG,    NULL, FFM_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJPA,   AV_CODEC_ID_MJPEG,    NULL, FFM_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJPB,   AV_CODEC_ID_MJPEGB,	  NULL, FFM_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJP2,   AV_CODEC_ID_JPEG2000, NULL, FFM_MJPEG, -1 },
	
	// DV VIDEO
	{ &MEDIASUBTYPE_dvsl, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_dvsd, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_dvhd, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_dv25, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_dv50, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_dvh1, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_CDVH, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	// Quicktime DV sybtypes (used in LAV Splitter)
	{ &MEDIASUBTYPE_DVCP, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_DVPP, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_DV5P, AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },
	{ &MEDIASUBTYPE_DVC,  AV_CODEC_ID_DVVIDEO, NULL, FFM_DV, -1 },

	// CSCD
	{ &MEDIASUBTYPE_CSCD, AV_CODEC_ID_CSCD,		   NULL, FFM_SCREC, -1 },
	// TSCC
	{ &MEDIASUBTYPE_TSCC, AV_CODEC_ID_TSCC,		   NULL, FFM_SCREC, -1 },
	// TSCC2
	{ &MEDIASUBTYPE_TSCC2, AV_CODEC_ID_TSCC2,	   NULL, FFM_SCREC, -1 },
	// VMnc
	{ &MEDIASUBTYPE_VMnc, AV_CODEC_ID_VMNC,		   NULL, FFM_SCREC, -1 },
	// QTRLE
	{ &MEDIASUBTYPE_QTRle, AV_CODEC_ID_QTRLE,	   NULL, FFM_SCREC, -1 },
	// CINEPAK
	{ &MEDIASUBTYPE_CVID, AV_CODEC_ID_CINEPAK,	   NULL, FFM_SCREC, -1 },
	// FLASHSV1
	{ &MEDIASUBTYPE_FLASHSV1, AV_CODEC_ID_FLASHSV, NULL, FFM_SCREC, -1 },
	// FLASHSV2
	//	{ &MEDIASUBTYPE_FLASHSV2,  CODEC_ID_FLASHSV2, NULL, FFM_SCREC, -1 },
	// FRAPS
	{ &MEDIASUBTYPE_FPS1, AV_CODEC_ID_FRAPS,	   NULL, FFM_SCREC, -1 },
	// MSS1
	{ &MEDIASUBTYPE_MSS1, AV_CODEC_ID_MSS1,		   NULL, FFM_SCREC, -1 },
	// MSS2
	{ &MEDIASUBTYPE_MSS2, AV_CODEC_ID_MSS2,		   NULL, FFM_SCREC, -1 },
	// MSA1
	{ &MEDIASUBTYPE_MSA1, AV_CODEC_ID_MSA1,		   NULL, FFM_SCREC, -1 },
	// MTS2
	{ &MEDIASUBTYPE_MTS2, AV_CODEC_ID_MTS2,		   NULL, FFM_SCREC, -1 },

	// UtVideo
	{ &MEDIASUBTYPE_UTVD_ULRG, AV_CODEC_ID_UTVIDEO, NULL, FFM_UTVD, -1 },
	{ &MEDIASUBTYPE_UTVD_ULRA, AV_CODEC_ID_UTVIDEO, NULL, FFM_UTVD, -1 },
	{ &MEDIASUBTYPE_UTVD_ULY0, AV_CODEC_ID_UTVIDEO, NULL, FFM_UTVD, -1 },
	{ &MEDIASUBTYPE_UTVD_ULY2, AV_CODEC_ID_UTVIDEO, NULL, FFM_UTVD, -1 },

	// DIRAC
	{ &MEDIASUBTYPE_DRAC, AV_CODEC_ID_DIRAC, NULL, FFM_DIRAC, -1 },

	// LAGARITH
	{ &MEDIASUBTYPE_Lagarith, AV_CODEC_ID_LAGARITH, NULL, FFM_LAGARITH, -1 },

	// Indeo 3/4/5
	{ &MEDIASUBTYPE_IV31, AV_CODEC_ID_INDEO3, NULL, FFM_INDEO, -1 },
	{ &MEDIASUBTYPE_IV32, AV_CODEC_ID_INDEO3, NULL, FFM_INDEO, -1 },
	{ &MEDIASUBTYPE_IV41, AV_CODEC_ID_INDEO4, NULL, FFM_INDEO, -1 },
	{ &MEDIASUBTYPE_IV50, AV_CODEC_ID_INDEO5, NULL, FFM_INDEO, -1 },

	// v210 (QT video)
	{ &MEDIASUBTYPE_v210, AV_CODEC_ID_V210, NULL, FFM_V210, -1 },

	// H264/AVC
	{ &MEDIASUBTYPE_H264,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_h264,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_X264,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_x264,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_VSSH,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_vssh,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_DAVC,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_davc,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_PAVC,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_pavc,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_AVC1,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_avc1,	  AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },
	{ &MEDIASUBTYPE_H264_bis, AV_CODEC_ID_H264, &DXVA_H264, FFM_H264, TRA_DXVA_H264 },

	// SVQ3
	{ &MEDIASUBTYPE_SVQ3, AV_CODEC_ID_SVQ3, NULL, FFM_SVQ3, -1 },
	// SVQ1
	{ &MEDIASUBTYPE_SVQ1, AV_CODEC_ID_SVQ1, NULL, FFM_SVQ3, -1 },

	// H263
	{ &MEDIASUBTYPE_H263, AV_CODEC_ID_H263, NULL, FFM_H263, -1 },
	{ &MEDIASUBTYPE_h263, AV_CODEC_ID_H263, NULL, FFM_H263, -1 },
	{ &MEDIASUBTYPE_S263, AV_CODEC_ID_H263, NULL, FFM_H263, -1 },
	{ &MEDIASUBTYPE_s263, AV_CODEC_ID_H263, NULL, FFM_H263, -1 },

	// Real Video
	{ &MEDIASUBTYPE_RV10, AV_CODEC_ID_RV10, NULL, FFM_RV, -1 },
	{ &MEDIASUBTYPE_RV20, AV_CODEC_ID_RV20, NULL, FFM_RV, -1 },
	{ &MEDIASUBTYPE_RV30, AV_CODEC_ID_RV30, NULL, FFM_RV, -1 },
	{ &MEDIASUBTYPE_RV40, AV_CODEC_ID_RV40, NULL, FFM_RV, -1 },

	// Theora
	{ &MEDIASUBTYPE_THEORA, AV_CODEC_ID_THEORA, NULL, FFM_THEORA, -1 },
	{ &MEDIASUBTYPE_theora, AV_CODEC_ID_THEORA, NULL, FFM_THEORA, -1 },

	// WVC1
	{ &MEDIASUBTYPE_WVC1, AV_CODEC_ID_VC1, &DXVA_VC1, FFM_VC1, TRA_DXVA_VC1 },
	{ &MEDIASUBTYPE_wvc1, AV_CODEC_ID_VC1, &DXVA_VC1, FFM_VC1, TRA_DXVA_VC1 },

	// Apple ProRes
	{ &MEDIASUBTYPE_apch, AV_CODEC_ID_PRORES, NULL, FFM_PRORES, -1 },
	{ &MEDIASUBTYPE_apcn, AV_CODEC_ID_PRORES, NULL, FFM_PRORES, -1 },
	{ &MEDIASUBTYPE_apcs, AV_CODEC_ID_PRORES, NULL, FFM_PRORES, -1 },
	{ &MEDIASUBTYPE_apco, AV_CODEC_ID_PRORES, NULL, FFM_PRORES, -1 },
	{ &MEDIASUBTYPE_ap4h, AV_CODEC_ID_PRORES, NULL, FFM_PRORES, -1 },

	// Bink Video
	{ &MEDIASUBTYPE_BINKVI, AV_CODEC_ID_BINKVIDEO, NULL, FFM_BINKV, -1 },
	{ &MEDIASUBTYPE_BINKVB, AV_CODEC_ID_BINKVIDEO, NULL, FFM_BINKV, -1 },

	// PNG
	{ &MEDIASUBTYPE_PNG, AV_CODEC_ID_PNG, NULL, FFM_PNG, -1 },

	// Canopus Lossless
	{ &MEDIASUBTYPE_CLLC, AV_CODEC_ID_CLLC, NULL, FFM_CLLC, -1 },

	// Other MPEG-4
	{ &MEDIASUBTYPE_MP4V, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_mp4v, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_M4S2, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_m4s2, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_MP4S, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_mp4s, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_3IV1, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_3iv1, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_3IV2, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_3iv2, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_3IVX, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_3ivx, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_BLZ0, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_blz0, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_DM4V, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_dm4v, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_FFDS, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_ffds, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_FVFW, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_fvfw, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_DXGM, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_dxgm, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_FMP4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_fmp4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_HDX4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_hdx4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_LMP4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_lmp4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_NDIG, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_ndig, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_RMP4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_rmp4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_SMP4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_smp4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_SEDG, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_sedg, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_UMP4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_ump4, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_WV1F, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
	{ &MEDIASUBTYPE_wv1f, AV_CODEC_ID_MPEG4, NULL, FFM_XVID, -1 },
};

/* Important: the order should be exactly the same as in ffCodecs[] */
const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	// Flash video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_flv1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLV4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_flv4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP6F },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp6f },

	// VP3
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP30 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP31 },

	// VP5
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp50 },

	// VP6
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP60 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp60 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP61 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp61 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP62 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp62 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP6A },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp6a },

	// VP8
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP80 },

	// Xvid
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_XVID },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_xvid },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_XVIX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_xvix },

	// DivX
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DX50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dx50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIVX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_divx },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Divx },

	// WMV1/2/3
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmv1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmv2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMV3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmv3 },

	// MPEG-2
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPG2        },

	// MPEG-1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet  },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload },

	// MSMPEG-4
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVX3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvx3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP43 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp43 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_COL1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_col1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV5 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div5 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV6 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div6 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AP41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ap41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPG3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mpg3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP42 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp42 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPG4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mpg4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp41 },

	// AMV Video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AMVV },

	// MJPEG
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJPG   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_QTJpeg },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJPA   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJPB   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJP2   },

	// DV VIDEO
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvsl },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvsd },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvhd },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dv25 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dv50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvh1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CDVH },
	// Quicktime DV sybtypes (used in LAV Splitter)
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVCP },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVPP },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DV5P },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVC  },

	// CSCD
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CSCD },

	// TSCC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_TSCC },

	// TSCC2
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_TSCC2 },

	// VMnc
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VMnc },

	// QTRLE
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_QTRle },

	// CINEPAK
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CVID },

	// FLASHSV1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLASHSV1 },

	// FLASHSV2
//	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLASHSV2 },

	// Fraps
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FPS1 },

	// MSS1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MSS1 },

	// MSS2
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MSS2 },

	// MSA1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MSA1 },

	// MTS2
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MTS2 },

	// UtVideo
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UTVD_ULRG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UTVD_ULRA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UTVD_ULY0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UTVD_ULY2 },

	// DIRAC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DRAC },

	// LAGARITH
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Lagarith },

	// Indeo 3/4/5
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV31 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV32 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV50 },

	// v210 (QT video)
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_v210 },

	// H264/AVC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_h264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_X264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_x264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VSSH     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vssh     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DAVC     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_davc     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PAVC     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_pavc     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AVC1     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_avc1     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264_bis },

	// SVQ3
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SVQ3 },

	// SVQ1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SVQ1 },

	// H263
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H263 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_h263 },

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_S263 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_s263 },

	// Real video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV10 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV20 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV30 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV40 },

	// Theora
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_THEORA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_theora },

	// VC1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WVC1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wvc1 },

	// Apple ProRes
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apch },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apcn },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apcs },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apco },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ap4h },

	// Bink Video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_BINKVI },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_BINKVB },

	// PNG
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PNG },

	// Canopus Lossless
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CLLC },

	// Other MPEG-4
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP4V },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp4v },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_M4S2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_m4s2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP4S },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp4s },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3IV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3iv1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3IV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3iv2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3IVX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3ivx },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_BLZ0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_blz0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DM4V },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dm4v },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FFDS },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ffds },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FVFW },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_fvfw },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DXGM },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dxgm },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_fmp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HDX4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_hdx4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_LMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_lmp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_NDIG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ndig },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_rmp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_smp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SEDG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_sedg },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ump4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WV1F },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wv1f }
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NV12},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YV12},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YUY2},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB32},
};

#ifdef REGISTER_FILTER

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn),  sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilters[] = {
	{&__uuidof(CMPCVideoDecFilter), MPCVideoDecName, MERIT_NORMAL + 1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilters[0].strName, &__uuidof(CMPCVideoDecFilter), CreateInstance<CMPCVideoDecFilter>, NULL, &sudFilters[0]},
	{L"CMPCVideoDecPropertyPage", &__uuidof(CMPCVideoDecSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMPCVideoDecSettingsWnd> >},
	{L"CMPCVideoDecPropertyPage2", &__uuidof(CMPCVideoDecCodecWnd), CreateInstance<CInternalPropertyPageTempl<CMPCVideoDecCodecWnd> >},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../core/FilterApp.h"

CFilterApp theApp;

#endif

BOOL CALLBACK EnumFindProcessWnd (HWND hwnd, LPARAM lParam)
{
	DWORD	procid = 0;
	TCHAR	WindowClass [40];
	GetWindowThreadProcessId (hwnd, &procid);
	GetClassName (hwnd, WindowClass, _countof(WindowClass));

	if (procid == GetCurrentProcessId() && _tcscmp (WindowClass, _T(MPC_WND_CLASS_NAME)) == 0) {
		HWND* pWnd = (HWND*) lParam;
		*pWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

CMPCVideoDecFilter::CMPCVideoDecFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseVideoFilter(NAME("MPC - Video decoder"), lpunk, phr, __uuidof(this))
{
	HWND hWnd = NULL;

	if (phr) {
		*phr = S_OK;
	}

	if (m_pOutput)	{
		delete m_pOutput;
	}
	m_pOutput = DNew CVideoDecOutputPin(NAME("CVideoDecOutputPin"), this, phr, L"Output");
	if (!m_pOutput) {
		*phr = E_OUTOFMEMORY;
	}

	memset(&m_DXVAFilters, false, sizeof(m_DXVAFilters));
	memset(&m_FFmpegFilters, false, sizeof(m_FFmpegFilters));
	
	m_pCpuId				= DNew CCpuId();
	m_pAVCodec				= NULL;
	m_pAVCtx				= NULL;
	m_pFrame				= NULL;
	m_pParser				= NULL;
	m_nCodecNb				= -1;
	m_nCodecId				= AV_CODEC_ID_NONE;
	m_bReorderBFrame		= true;
	m_DXVADecoderGUID		= GUID_NULL;
	m_nActiveCodecs			= MPCVD_H264|MPCVD_VC1|MPCVD_XVID|MPCVD_DIVX|MPCVD_MSMPEG4|MPCVD_FLASH|MPCVD_WMV|MPCVD_H263|MPCVD_SVQ3|MPCVD_AMVV|MPCVD_THEORA|MPCVD_H264_DXVA|MPCVD_VC1_DXVA|MPCVD_VP356|MPCVD_VP8|MPCVD_MJPEG|MPCVD_INDEO|MPCVD_RV|MPCVD_WMV3_DXVA|MPCVD_MPEG2_DXVA|MPCVD_DIRAC|MPCVD_DV|MPCVD_UTVD|MPCVD_SCREC|MPCVD_LAGARITH|MPCVD_PRORES|MPCVD_BINKV|MPCVD_PNG|MPCVD_CLLC|MPCVD_V210|MPCVD_MPEG2|MPCVD_MPEG1;

	m_rtAvrTimePerFrame		= 0;
	m_rtLastStop			= 0;
	m_rtPrevStop			= 0;

	m_nWorkaroundBug		= FF_BUG_AUTODETECT;
	m_nErrorConcealment		= FF_EC_DEBLOCK | FF_EC_GUESS_MVS;

	m_nThreadNumber			= 0;
	m_nDiscardMode			= AVDISCARD_DEFAULT;
	m_nDeinterlacing		= AUTO;
	m_bDXVACompatible		= true;
	m_pFFBuffer				= NULL;
	m_nFFBufferSize			= 0;
	m_pFFBuffer2			= NULL;
	m_nFFBufferSize2		= 0;
	m_pAlignedFFBuffer		= NULL;
	m_nAlignedFFBufferSize	= 0;

	m_nOutputWidth			= 0;
	m_nOutputHeight			= 0;

	m_nARX					= 0;
	m_nARY					= 0;
	m_pSwsContext			= NULL;

	m_bUseDXVA				= true;
	m_bUseFFmpeg			= true;

	m_nDXVAMode				= MODE_SOFTWARE;
	m_pDXVADecoder			= NULL;
	m_pVideoOutputFormat	= NULL;
	m_nVideoOutputCount		= 0;
	m_hDevice				= INVALID_HANDLE_VALUE;

	m_nARMode					= 2; // default state - 3rd
	m_nDXVACheckCompatibility	= 1; // skip level check by default
	m_nDXVA_SD					= 0;

	m_bWaitingForKeyFrame	= TRUE;
	m_nPosB					= 1;
	m_bIsEVO				= false;

	m_nFrameType			= PICT_FRAME;
	m_nOutCsp				= 0;
	
	// === New swscaler options
	m_nSwRefresh			= 0;
	m_nSwOutputFormats		= 0;
	// set the output formats order in the DWORD nibbles with literal values [0x00543210] = 0,1,2,3,4,5
	// set the output formats count in the DWORD nibbles adding checked flag (8) [0x0054ba98] = 4
	for (int i=0; i<6; i++) {
		m_nSwOutputFormats = (m_nSwOutputFormats<<4) | (5-i + (i>1 ? 8 : 0));
	}
	m_nSwChromaToRGB		= 1;
	m_nSwResizeMethodBE		= 2;
	m_nSwColorspace			= 2;
	m_nSwInputLevels		= 2;
	m_nSwOutputLevels		= 2;
	//

	m_PixFmt				= AV_PIX_FMT_NB;

	m_nDialogHWND			= 0;

	m_rtStartCache			= INVALID_TIME;
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\MPC-BE Filters\\MPC Video Decoder"), KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("ThreadNumber"), dw)) {
			m_nThreadNumber = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("DiscardMode"), dw)) {
			m_nDiscardMode = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("Deinterlacing"), dw)) {
			m_nDeinterlacing = (MPC_DEINTERLACING_FLAGS)dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("ActiveCodecs"), dw)) {
			m_nActiveCodecs = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("ARMode"), dw)) {
			m_nARMode = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("DXVACheckCompatibility"), dw)) {
			m_nDXVACheckCompatibility = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("DisableDXVA_SD"), dw)) {
			m_nDXVA_SD = dw;
		}

		// === New swscaler options
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("SwOutputFormats"), dw)) {
			m_nSwOutputFormats = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("SwChromaToRGB"), dw)) {
			m_nSwChromaToRGB = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("SwResizeMethodBE"), dw)) {
			m_nSwResizeMethodBE = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("SwColorspace"), dw)) {
			m_nSwColorspace = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("SwInputLevels"), dw)) {
			m_nSwInputLevels = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(_T("SwOutputLevels"), dw)) {
			m_nSwOutputLevels = dw;
		}
		//
	}
#else
	m_nThreadNumber				= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("ThreadNumber"), m_nThreadNumber);
	m_nDiscardMode				= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("DiscardMode"), m_nDiscardMode);
	m_nDeinterlacing			= (MPC_DEINTERLACING_FLAGS)AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("Deinterlacing"), m_nDeinterlacing);
	m_nARMode					= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("ARMode"), m_nARMode);
	m_nDXVACheckCompatibility	= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("DXVACheckCompatibility"), m_nDXVACheckCompatibility);
	m_nDXVA_SD					= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("DisableDXVA_SD"), m_nDXVA_SD);

	// === New swscaler options
	m_nSwOutputFormats			= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwOutputFormats"), m_nSwOutputFormats);
	m_nSwChromaToRGB			= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwChromaToRGB"), m_nSwChromaToRGB);
	m_nSwResizeMethodBE			= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwResizeMethodBE"), m_nSwResizeMethodBE);
	m_nSwColorspace				= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwColorspace"), m_nSwColorspace);
	m_nSwInputLevels			= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwInputLevels"), m_nSwInputLevels);
	m_nSwOutputLevels			= AfxGetApp()->GetProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwOutputLevels"), m_nSwOutputLevels);
	//
#endif

	m_nDXVACheckCompatibility = max(0, min(m_nDXVACheckCompatibility, 3));

	if (m_nDeinterlacing > PROGRESSIVE) {
		m_nDeinterlacing = AUTO;
	}

	avcodec_register_all();
	av_log_set_callback(ff_log);

	EnumWindows(EnumFindProcessWnd, (LPARAM)&hWnd);
	DetectVideoCard(hWnd);

#ifdef _DEBUG
	// Check codec definition table
	int nCodecs	  = _countof(ffCodecs);
	int nPinTypes = _countof(sudPinTypesIn);
	ASSERT (nCodecs == nPinTypes);
	for (int i=0; i<nPinTypes; i++) {
		ASSERT (ffCodecs[i].clsMinorType == sudPinTypesIn[i].clsMinorType);
	}
#endif
}

void CMPCVideoDecFilter::DetectVideoCard(HWND hWnd)
{
	IDirect3D9* pD3D9;
	m_nPCIVendor = 0;
	m_nPCIDevice = 0;
	m_VideoDriverVersion.HighPart = 0;
	m_VideoDriverVersion.LowPart = 0;

	pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D9) {
		D3DADAPTER_IDENTIFIER9 adapterIdentifier;
		if (pD3D9->GetAdapterIdentifier(GetAdapter(pD3D9, hWnd), 0, &adapterIdentifier) == S_OK) {
			m_nPCIVendor = adapterIdentifier.VendorId;
			m_nPCIDevice = adapterIdentifier.DeviceId;
			m_VideoDriverVersion = adapterIdentifier.DriverVersion;
			m_strDeviceDescription = adapterIdentifier.Description;
			m_strDeviceDescription.AppendFormat (_T(" (%04X:%04X)"), m_nPCIVendor, m_nPCIDevice);
		}
		pD3D9->Release();
	}
}

CMPCVideoDecFilter::~CMPCVideoDecFilter()
{
	Cleanup();

	SAFE_DELETE(m_pCpuId);
}

bool CMPCVideoDecFilter::IsVideoInterlaced()
{
	// NOT A BUG : always tell DirectShow it's interlaced (progressive flags set in
	// SetTypeSpecificFlags function)
	return true;
};

REFERENCE_TIME CMPCVideoDecFilter::GetDuration()
{
	REFERENCE_TIME AvgTimePerFrame = m_rtAvrTimePerFrame;
	if ((m_nCodecId == AV_CODEC_ID_MPEG2VIDEO || m_nCodecId == AV_CODEC_ID_MPEG1VIDEO)
		|| m_rtAvrTimePerFrame < 166666) // fps > 60 ... try to get fps value from ffmpeg
	{
		if (m_pAVCtx->time_base.den && m_pAVCtx->time_base.num) {
			AvgTimePerFrame = (UNITS * m_pAVCtx->time_base.num / m_pAVCtx->time_base.den) * m_pAVCtx->ticks_per_frame;
		}
	}

	return AvgTimePerFrame;
}

#define AVRTIMEPERFRAME_PULLDOWN 417083
void CMPCVideoDecFilter::UpdateFrameTime(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop, bool pulldown_flag)
{
	REFERENCE_TIME AvgTimePerFrame = GetDuration();
	bool m_PullDownFlag = pulldown_flag && AvgTimePerFrame == 333666;
	REFERENCE_TIME m_rtFrameDuration = m_PullDownFlag ? AVRTIMEPERFRAME_PULLDOWN : (AvgTimePerFrame * (m_pFrame->repeat_pict ? 3 : 2)  / 2);

	if ((rtStart == INVALID_TIME) || (m_PullDownFlag && m_rtPrevStop && (rtStart <= m_rtPrevStop))) {
		rtStart = m_rtLastStop;
	}

	rtStop			= rtStart + (m_rtFrameDuration / m_dRate);
	m_rtLastStop	= rtStop;
}

void CMPCVideoDecFilter::GetOutputSize(int& w, int& h, int& arx, int& ary, int& RealWidth, int& RealHeight)
{
	RealWidth	= PictWidth();
	RealHeight	= PictHeight();
	w = PictWidthRounded();
	h = PictHeightRounded();
}

int CMPCVideoDecFilter::PictWidth()
{
	return m_pAVCtx->width;
}

int CMPCVideoDecFilter::PictHeight()
{
	return m_pAVCtx->height;
}

int CMPCVideoDecFilter::PictWidthRounded()
{
	// Picture height should be rounded to 16 for DXVA
	if (!m_nOutputWidth || m_nOutputWidth < m_pAVCtx->coded_width) {
		m_nOutputWidth = m_pAVCtx->coded_width;
	}
	return ((m_nOutputWidth + 15) / 16) * 16;
}

int CMPCVideoDecFilter::PictHeightRounded()
{
	// Picture height should be rounded to 16 for DXVA
	if (!m_nOutputHeight || m_nOutputHeight < m_pAVCtx->coded_height) {
		m_nOutputHeight = m_pAVCtx->coded_height;
	}
	return ((m_nOutputHeight + 15) / 16) * 16;
}

static bool IsFFMPEGEnabled(FFMPEG_CODECS ffcodec, const bool FFmpegFilters[FFM_LAST + !FFM_LAST])
{
	if (ffcodec.FFMPEGCode < 0 || ffcodec.FFMPEGCode >= FFM_LAST) {
		return false;
	}

	return FFmpegFilters[ffcodec.FFMPEGCode];

	return false;
}

static bool IsDXVAEnabled(FFMPEG_CODECS ffcodec, const bool DXVAFilters[TRA_DXVA_LAST + !TRA_DXVA_LAST])
{
	if (ffcodec.DXVACode < 0 || ffcodec.DXVACode >= TRA_DXVA_LAST) {
		return false;
	}

	return DXVAFilters[ffcodec.DXVACode];

	return false;
}

int CMPCVideoDecFilter::FindCodec(const CMediaType* mtIn, bool bForced)
{
	m_bUseFFmpeg	= false;
	m_bUseDXVA		= false;
	for (int i=0; i<_countof(ffCodecs); i++)
		if (mtIn->subtype == *ffCodecs[i].clsMinorType) {
#ifndef REGISTER_FILTER
			m_bUseFFmpeg	= bForced || IsFFMPEGEnabled(ffCodecs[i], m_FFmpegFilters);
			m_bUseDXVA		= bForced || IsDXVAEnabled(ffCodecs[i], m_DXVAFilters);
			return ((m_bUseDXVA || m_bUseFFmpeg) ? i : -1);
#else
			bool	bCodecActivated = false;
			m_bUseFFmpeg			= true;
			switch (ffCodecs[i].nFFCodec) {
				case AV_CODEC_ID_FLV1 :
				case AV_CODEC_ID_VP6F :
					bCodecActivated = (m_nActiveCodecs & MPCVD_FLASH) != 0;
					break;
				case AV_CODEC_ID_MPEG4 :
					if ((*ffCodecs[i].clsMinorType == MEDIASUBTYPE_DX50) ||		// DivX
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_dx50) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_DIVX) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_divx) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_Divx) ) {
						bCodecActivated = (m_nActiveCodecs & MPCVD_DIVX) != 0;
					} else {
						bCodecActivated = (m_nActiveCodecs & MPCVD_XVID) != 0;	// Xvid/MPEG-4
					}
					break;
				case AV_CODEC_ID_WMV1 :
				case AV_CODEC_ID_WMV2 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_WMV) != 0;
					break;
				case AV_CODEC_ID_WMV3 :
					m_bUseDXVA = (m_nActiveCodecs & MPCVD_WMV3_DXVA) != 0;
					m_bUseFFmpeg = (m_nActiveCodecs & MPCVD_WMV) != 0;
					bCodecActivated = m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_MSMPEG4V3 :
				case AV_CODEC_ID_MSMPEG4V2 :
				case AV_CODEC_ID_MSMPEG4V1 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_MSMPEG4) != 0;
					break;
				case AV_CODEC_ID_H264 :
					m_bUseDXVA = (m_nActiveCodecs & MPCVD_H264_DXVA) != 0;
					m_bUseFFmpeg = (m_nActiveCodecs & MPCVD_H264) != 0;
					bCodecActivated = m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_SVQ3 :
				case AV_CODEC_ID_SVQ1 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_SVQ3) != 0;
					break;
				case AV_CODEC_ID_H263 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_H263) != 0;
					break;
				case AV_CODEC_ID_DIRAC  :
					bCodecActivated = (m_nActiveCodecs & MPCVD_DIRAC) != 0;
					break;
				case AV_CODEC_ID_DVVIDEO  :
					bCodecActivated = (m_nActiveCodecs & MPCVD_DV) != 0;
					break;
				case AV_CODEC_ID_THEORA :
					bCodecActivated = (m_nActiveCodecs & MPCVD_THEORA) != 0;
					break;
				case AV_CODEC_ID_VC1 :
					m_bUseDXVA = (m_nActiveCodecs & MPCVD_VC1_DXVA) != 0;
					m_bUseFFmpeg = (m_nActiveCodecs & MPCVD_VC1) != 0;
					bCodecActivated = m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_AMV :
					bCodecActivated = (m_nActiveCodecs & MPCVD_AMVV) != 0;
					break;
				case AV_CODEC_ID_LAGARITH :
					bCodecActivated = (m_nActiveCodecs & MPCVD_LAGARITH) != 0;
					break;
				case AV_CODEC_ID_VP3  :
				case AV_CODEC_ID_VP5  :
				case AV_CODEC_ID_VP6  :
				case AV_CODEC_ID_VP6A :
					bCodecActivated = (m_nActiveCodecs & MPCVD_VP356) != 0;
					break;
				case AV_CODEC_ID_VP8  :
					bCodecActivated = (m_nActiveCodecs & MPCVD_VP8) != 0;
					break;
				case AV_CODEC_ID_MJPEG  :
				case AV_CODEC_ID_MJPEGB :
					bCodecActivated = (m_nActiveCodecs & MPCVD_MJPEG) != 0;
					break;
				case AV_CODEC_ID_INDEO3 :
				case AV_CODEC_ID_INDEO4 :
				case AV_CODEC_ID_INDEO5 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_INDEO) != 0;
					break;
				case AV_CODEC_ID_UTVIDEO :
					bCodecActivated = (m_nActiveCodecs & MPCVD_UTVD) != 0;
					break;
				case AV_CODEC_ID_CSCD    :
				case AV_CODEC_ID_QTRLE   :
				case AV_CODEC_ID_TSCC    :
				case AV_CODEC_ID_TSCC2   :
				case AV_CODEC_ID_VMNC    :
				case AV_CODEC_ID_CINEPAK :
					bCodecActivated = (m_nActiveCodecs & MPCVD_SCREC) != 0;
					break;
				case AV_CODEC_ID_RV10 :
				case AV_CODEC_ID_RV20 :
				case AV_CODEC_ID_RV30 :
				case AV_CODEC_ID_RV40 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_RV) != 0;
					break;
				case AV_CODEC_ID_MPEG2VIDEO :
					m_bUseDXVA = (m_nActiveCodecs & MPCVD_MPEG2_DXVA) != 0;
					m_bUseFFmpeg = (m_nActiveCodecs & MPCVD_MPEG2) != 0;
					bCodecActivated = m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_MPEG1VIDEO :
					bCodecActivated = (m_nActiveCodecs & MPCVD_MPEG1) != 0;
					break;
				case AV_CODEC_ID_PRORES :
					bCodecActivated = (m_nActiveCodecs & MPCVD_PRORES) != 0;
					break;
				case AV_CODEC_ID_BINKVIDEO :
					bCodecActivated = (m_nActiveCodecs & MPCVD_BINKV) != 0;
					break;
				case AV_CODEC_ID_PNG :
					bCodecActivated = (m_nActiveCodecs & MPCVD_PNG) != 0;
					break;
				case AV_CODEC_ID_CLLC :
					bCodecActivated = (m_nActiveCodecs & MPCVD_CLLC) != 0;
					break;
				case AV_CODEC_ID_V210 :
					bCodecActivated = (m_nActiveCodecs & MPCVD_V210) != 0;
					break;
			}

			if (!bCodecActivated && !bForced) {
				m_bUseFFmpeg = false;
			}
			return ((bForced || bCodecActivated) ? i : -1);
#endif
		}

	return -1;
}

void CMPCVideoDecFilter::Cleanup()
{
	SAFE_DELETE (m_pDXVADecoder);

	ffmpegCleanup();

	SAFE_DELETE_ARRAY (m_pVideoOutputFormat);

	// Release DXVA ressources
	if (m_hDevice != INVALID_HANDLE_VALUE) {
		m_pDeviceManager->CloseDeviceHandle(m_hDevice);
		m_hDevice = INVALID_HANDLE_VALUE;
	}

	m_pDeviceManager		= NULL;
	m_pDecoderService		= NULL;
	m_pDecoderRenderTarget	= NULL;
}

void CMPCVideoDecFilter::ffmpegCleanup()
{
	// Release FFMpeg
	if (m_pParser) {
		av_parser_close(m_pParser);
	}

	if (m_pAVCtx) {
		avcodec_close(m_pAVCtx);
		if (m_pAVCtx->extradata) {
			av_freep(&m_pAVCtx->extradata);
		}
		av_freep(&m_pAVCtx);
	}

	if (m_pFFBuffer) {
		av_freep(&m_pFFBuffer);
	}
	if (m_pFFBuffer2) {
		av_freep(&m_pFFBuffer2);
	}
	if (m_pAlignedFFBuffer) {
		av_freep(&m_pAlignedFFBuffer);
	}

	if (m_pFrame) {
		av_freep(&m_pFrame);
	}

	if (m_pSwsContext) {
		sws_freeContext(m_pSwsContext);
	}
	
	m_pSwsContext	= NULL;
	m_PixFmt		= AV_PIX_FMT_NB;

	m_pAVCodec		= NULL;
	m_pAVCtx		= NULL;
	m_pFrame		= NULL;
	m_pParser		= NULL;
	
	m_pFFBuffer				= NULL;
	m_nFFBufferSize			= 0;
	m_pFFBuffer2			= NULL;
	m_nFFBufferSize2		= 0;
	m_pAlignedFFBuffer		= NULL;
	m_nAlignedFFBufferSize	= 0;
	
	m_nCodecNb		= -1;
	m_nCodecId		= AV_CODEC_ID_NONE;
}

STDMETHODIMP CMPCVideoDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMPCVideoDecFilter)
		QI(IMPCVideoDecFilter2)
		QI(IMPCVideoDecFilterCodec)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPCVideoDecFilter::CheckInputType(const CMediaType* mtIn)
{
	for (int i=0; i<_countof(sudPinTypesIn); i++) {
		if ((mtIn->majortype == *sudPinTypesIn[i].clsMajorType) &&
				(mtIn->subtype == *sudPinTypesIn[i].clsMinorType)) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

bool CMPCVideoDecFilter::IsAVI()
{
	static DWORD SYNC = 0;
	if (SYNC == MAKEFOURCC('R','I','F','F')) {
		return true;
	} else if (SYNC != 0) {
		return false;
	}

	CString fname;

	BeginEnumFilters(m_pGraph, pEF, pBF) {
		CComQIPtr<IFileSourceFilter> pFSF = pBF;
		if (pFSF) {
			LPOLESTR pFN = NULL;
			AM_MEDIA_TYPE mt;
			if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
				fname = CString(pFN);
				CoTaskMemFree(pFN);
			}
			break;
		}
	}
	EndEnumFilters

	if (!fname.IsEmpty() && ::PathFileExists(fname)) {
		CFile f;
		CFileException fileException;
		if (!f.Open(fname, CFile::modeRead|CFile::typeBinary|CFile::shareDenyNone, &fileException)) {
			DbgLog((LOG_TRACE, 3, _T("CMPCVideoDecFilter::IsAVI() : Can't open file '%s', error = %u"), fname, fileException.m_cause));
			return false;
		}

		if (f.Read(&SYNC, sizeof(SYNC)) != sizeof(SYNC)) {
			DbgLog((LOG_TRACE, 3, _T("CMPCVideoDecFilter::IsAVI() : Can't read SYNC from file '%s'"), fname));
			return false;
		}

		if (SYNC == MAKEFOURCC('R','I','F','F')) {
			DbgLog((LOG_TRACE, 3, _T("CMPCVideoDecFilter::IsAVI() : '%s' is a valid AVI file"), fname));
			return true;
		}

		DbgLog((LOG_TRACE, 3, _T("CMPCVideoDecFilter::IsAVI() : '%s' is not valid AVI file"), fname));
	}

	return false;
}

HRESULT CMPCVideoDecFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	if (direction == PINDIR_INPUT) {

		HRESULT hr = InitDecoder(pmt);
		if (FAILED(hr)) {
			return hr;
		}
	}

	return __super::SetMediaType(direction, pmt);
}

VIDEO_OUTPUT_FORMATS DXVAFormats[] = { // DXVA2
	{&MEDIASUBTYPE_NV12, 1, 12, 'avxd'},
	{&MEDIASUBTYPE_NV12, 1, 12, 'AVXD'},
	{&MEDIASUBTYPE_NV12, 1, 12, 'AVxD'},
	{&MEDIASUBTYPE_NV12, 1, 12, 'AvXD'}
};

// === New swscaler options
VIDEO_OUTPUT_FORMATS SoftwareFormats[] = { // Software
	{&MEDIASUBTYPE_NV12,   2, 12, '21VN'},
	{&MEDIASUBTYPE_YV12,   3, 12, '21VY'},
	{&MEDIASUBTYPE_YUY2,   1, 16, '2YUY'},
	{&MEDIASUBTYPE_RGB32,  1, 32, BI_RGB}, //1
	{&MEDIASUBTYPE_RGB565, 1, 16, BI_BITFIELDS},
	{&MEDIASUBTYPE_RGB555, 1, 16, BI_RGB}
};

bool CMPCVideoDecFilter::IsDXVASupported()
{
	if (m_nCodecNb != -1) {
		// Does the codec suppport DXVA ?
		if (ffCodecs[m_nCodecNb].DXVAModes != NULL) {
			// Enabled by user ?
			if (m_bUseDXVA) {
				// is the file compatible ?
				if (m_bDXVACompatible) {
					return true;
				}
			}
		}
	}
	return false;
}

HRESULT CMPCVideoDecFilter::FindDecoderConfiguration()
{
	TRACE(_T("CMPCVideoDecFilter::FindDecoderConfiguration()\n"));

	HRESULT hr = E_FAIL;

	m_DXVADecoderGUID = GUID_NULL;

	if (m_pDecoderService) {
		UINT cDecoderGuids = 0;
		GUID* pDecoderGuids = NULL;
		GUID guidDecoder = GUID_NULL;
		BOOL bFoundDXVA2Configuration = FALSE;
		BOOL bHasIntelGuid = FALSE;
		DXVA2_ConfigPictureDecode config;
		ZeroMemory(&config, sizeof(config));

		hr = m_pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);

		if (SUCCEEDED(hr)) {

			//Intel patch for Ivy Bridge and Sandy Bridge
			if (m_nPCIVendor == PCIV_Intel) {
				for (UINT iCnt = 0; iCnt < cDecoderGuids; iCnt++) {
					if (pDecoderGuids[iCnt] == DXVA_Intel_H264_ClearVideo)
						bHasIntelGuid = TRUE;
				}
			}
			// Look for the decoder GUIDs we want.
			for (UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++) {
				TRACE(_T("Enumerate DXVA mode : %s\n"), GetDXVAMode(&pDecoderGuids[iGuid]));

				// Do we support this mode?
				if (!IsSupportedDecoderMode(pDecoderGuids[iGuid])) {
					continue;
				}

				TRACE(_T("	=> trying : %s\n"), GetDXVAMode(&pDecoderGuids[iGuid]));

				// Find a configuration that we support.
				hr = FindDXVA2DecoderConfiguration(m_pDecoderService, pDecoderGuids[iGuid], &config, &bFoundDXVA2Configuration);

				if (FAILED(hr)) {
					break;
				}

				// Patch for the Sandy Bridge (prevent crash on Mode_E, fixme later)
				if (m_nPCIVendor == PCIV_Intel && pDecoderGuids[iGuid] == DXVA2_ModeH264_E && bHasIntelGuid) {
					continue;
				}

				if (bFoundDXVA2Configuration) {
					// Found a good configuration. Save the GUID.
					guidDecoder = pDecoderGuids[iGuid];
					TRACE(_T("	=> Found : %s\n"), GetDXVAMode(&guidDecoder));
					if (!bHasIntelGuid) {
						break;
					}
				}
			}
		}

		if (pDecoderGuids) {
			CoTaskMemFree(pDecoderGuids);
		}
		if (!bFoundDXVA2Configuration) {
			hr = E_FAIL; // Unable to find a configuration.
		}

		if (SUCCEEDED(hr)) {
			m_DXVA2Config		= config;
			m_DXVADecoderGUID	= guidDecoder;
		}
	}

	return hr;
}

#define ATI_IDENTIFY					_T("ATI ")
#define AMD_IDENTIFY					_T("AMD ")
#define RADEON_HD_IDENTIFY				_T("Radeon HD ")
#define RADEON_MOBILITY_HD_IDENTIFY		_T("Mobility Radeon HD ")

HRESULT CMPCVideoDecFilter::InitDecoder(const CMediaType *pmt)
{
	bool bChangeMT = (m_pAVCtx != NULL);

	ffmpegCleanup();

	int nNewCodec = FindCodec(pmt, bChangeMT);

	if (nNewCodec == -1) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (CComPtr<IBaseFilter> pFilter = GetFilterFromPin(m_pInput->GetConnected()) ) {
		// Prevent connection to the video decoder - need to support decoding of uncompressed(v210) video
		if (IsVideoDecoder(pFilter, true)) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	if (nNewCodec != m_nCodecNb) {
			
		m_nCodecNb	= nNewCodec;
		m_nCodecId	= ffCodecs[nNewCodec].nFFCodec;

		m_bReorderBFrame	= true;
		m_pAVCodec			= avcodec_find_decoder(m_nCodecId);
		CheckPointer (m_pAVCodec, VFW_E_UNSUPPORTED_VIDEO);

		m_pAVCtx	= avcodec_alloc_context3(m_pAVCodec);
		CheckPointer (m_pAVCtx, E_POINTER);

		if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO || m_nCodecId == AV_CODEC_ID_MPEG1VIDEO) {
			m_pParser = av_parser_init(m_nCodecId);
		}

		if (bChangeMT && m_nDXVAMode == MODE_SOFTWARE) {
			m_bUseDXVA = false;
		}

		int nThreadNumber = m_nThreadNumber ? m_nThreadNumber : m_pCpuId->GetProcessorNumber() * 3/2;
		if ((nThreadNumber > 1) && FFGetThreadType(m_nCodecId)) {
			FFSetThreadNumber(m_pAVCtx, m_nCodecId, IsDXVASupported() ? 1 : nThreadNumber);
		}

		m_pFrame = avcodec_alloc_frame();
		CheckPointer (m_pFrame, E_POINTER);

		m_h264RandomAccess.SetAVCNALSize(0);
		m_h264RandomAccess.flush(m_pAVCtx->thread_count);

		BITMAPINFOHEADER *pBMI = NULL;
		if (pmt->formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER* vih	= (VIDEOINFOHEADER*)pmt->pbFormat;
			pBMI					= &vih->bmiHeader;
		} else if (pmt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2	= (VIDEOINFOHEADER2*)pmt->pbFormat;
			pBMI					= &vih2->bmiHeader;
		} else if (pmt->formattype == FORMAT_MPEGVideo) {
			MPEG1VIDEOINFO* mpgv	= (MPEG1VIDEOINFO*)pmt->pbFormat;
			pBMI					= &mpgv->hdr.bmiHeader;
		} else if (pmt->formattype == FORMAT_MPEG2Video) {
			MPEG2VIDEOINFO* mpg2v	= (MPEG2VIDEOINFO*)pmt->pbFormat;
			pBMI					= &mpg2v->hdr.bmiHeader;

			m_pAVCtx->codec_tag		= pBMI->biCompression ? pBMI->biCompression : pmt->subtype.Data1;

			if ((m_pAVCtx->codec_tag == MAKEFOURCC('a','v','c','1')) || (m_pAVCtx->codec_tag == MAKEFOURCC('A','V','C','1'))) {
				m_bReorderBFrame			= IsAVI() ? true : false;
			} else if ((m_pAVCtx->codec_tag == MAKEFOURCC('m','p','4','v')) || (m_pAVCtx->codec_tag == MAKEFOURCC('M','P','4','V'))) {
				m_bReorderBFrame			= false;
			}
		} else {
			return VFW_E_INVALIDMEDIATYPE;
		}

		if (m_nCodecId == AV_CODEC_ID_RV10
			|| m_nCodecId == AV_CODEC_ID_RV20
			|| m_nCodecId == AV_CODEC_ID_RV30
			|| m_nCodecId == AV_CODEC_ID_RV40
			|| m_nCodecId == AV_CODEC_ID_VP8
			|| m_nCodecId == AV_CODEC_ID_VP3
			|| m_nCodecId == AV_CODEC_ID_THEORA
			|| m_nCodecId == AV_CODEC_ID_MPEG2VIDEO
			|| m_nCodecId == AV_CODEC_ID_MPEG1VIDEO
			|| m_nCodecId == AV_CODEC_ID_DIRAC
			|| m_nCodecId == AV_CODEC_ID_UTVIDEO) {
			m_bReorderBFrame = false;
		}

		m_pAVCtx->codec_id              = m_nCodecId;
		m_pAVCtx->codec_tag             = pBMI->biCompression ? pBMI->biCompression : pmt->subtype.Data1;
		m_pAVCtx->coded_width           = pBMI->biWidth;
		m_pAVCtx->coded_height          = abs(pBMI->biHeight);
		m_pAVCtx->bits_per_coded_sample = pBMI->biBitCount;
		m_pAVCtx->workaround_bugs       = m_nWorkaroundBug;
		m_pAVCtx->error_concealment     = m_nErrorConcealment;
		m_pAVCtx->err_recognition       = AV_EF_CAREFUL;
		m_pAVCtx->idct_algo             = FF_IDCT_AUTO;
		m_pAVCtx->skip_loop_filter      = (AVDiscard)m_nDiscardMode;

		if (m_nCodecId == AV_CODEC_ID_H264) {
			m_pAVCtx->flags2           |= CODEC_FLAG2_SHOW_ALL;
		}

		if (m_pAVCtx->codec_tag == MAKEFOURCC('m','p','g','2')) {
			m_pAVCtx->codec_tag = MAKEFOURCC('M','P','E','G');
		}

		AllocExtradata(m_pAVCtx, pmt);
		ExtractAvgTimePerFrame(&m_pInput->CurrentMediaType(), m_rtAvrTimePerFrame);
		int wout, hout;
		ExtractDim(&m_pInput->CurrentMediaType(), wout, hout, m_nARX, m_nARY);
		UNREFERENCED_PARAMETER(wout);
		UNREFERENCED_PARAMETER(hout);

		m_pAVCtx->using_dxva = (IsDXVASupported() && (m_nCodecId == AV_CODEC_ID_H264 || m_nCodecId == AV_CODEC_ID_MPEG2VIDEO));

		if (avcodec_open2(m_pAVCtx, m_pAVCodec, NULL) < 0) {
			return VFW_E_INVALIDMEDIATYPE;
		}

		FFGetOutputSize(m_pAVCtx, m_pFrame, &m_nOutputWidth, &m_nOutputHeight);

		if (IsDXVASupported()) {
			do {
				m_bDXVACompatible = false;

				if (!DXVACheckFramesize(PictWidth(), PictHeight(), m_nPCIVendor, m_nPCIDevice)) { // check frame size
					break;
				}

				if (m_nCodecId == AV_CODEC_ID_H264) {
					if (m_nDXVA_SD && PictWidthRounded() < 1280) { // check "Disable DXVA for SD" option
						break;
					}

					bool IsAtiDXVACompatible = false;
					if (m_nPCIVendor == PCIV_ATI) {
						if (!m_strDeviceDescription.Find(ATI_IDENTIFY) || !m_strDeviceDescription.Find(AMD_IDENTIFY)) {
							m_strDeviceDescription.Delete(0, 4);
							TCHAR ati_version = '0';
							if (!m_strDeviceDescription.Find(RADEON_HD_IDENTIFY)) {
								ati_version = m_strDeviceDescription.GetAt(CString(RADEON_HD_IDENTIFY).GetLength());
							} else if (!m_strDeviceDescription.Find(RADEON_MOBILITY_HD_IDENTIFY)) {
								ati_version = m_strDeviceDescription.GetAt(CString(RADEON_MOBILITY_HD_IDENTIFY).GetLength());
							}
							IsAtiDXVACompatible = (atoi(&ati_version) >= 4); // HD4xxx/Mobility and above AMD/ATI cards support level 5.1 and ref = 16
						}
					} else if (m_nPCIVendor == PCIV_Intel && !IsWinVistaOrLater() && m_nPCIDevice == 0x8108) {
						break; // Disable support H.264 DXVA on Intel GMA500 in WinXP
					}
					int nCompat = FFH264CheckCompatibility (PictWidthRounded(), PictHeightRounded(), m_pAVCtx, m_pFrame, m_nPCIVendor, m_nPCIDevice, m_VideoDriverVersion, IsAtiDXVACompatible);

					if ((nCompat & DXVA_PROFILE_HIGHER_THAN_HIGH) || (nCompat & DXVA_HIGH_BIT)) { // DXVA unsupported
						break;
					}
					
					if (nCompat) {
						bool bDXVACompatible = true;
						switch (m_nDXVACheckCompatibility) {
							case 0:
								bDXVACompatible = false;
								break;
							case 1:
								bDXVACompatible = (nCompat == DXVA_UNSUPPORTED_LEVEL);
								break;
							case 2:
								bDXVACompatible = (nCompat == DXVA_TOO_MANY_REF_FRAMES);
								break;
						}
						if (!bDXVACompatible) {
							break;
						}
					}
				} else if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO) {
					if (!MPEG2CheckCompatibility(m_pAVCtx, m_pFrame)) {
						break;
					}
				} else if (m_nCodecId == AV_CODEC_ID_WMV3) {
					if (PictWidth() <= 720) { // fixes color problem for some wmv files (profile <= MP@ML)
						break;
					}
				}

				m_bDXVACompatible = true;
			} while (false);

			if (!m_bDXVACompatible) {
				HRESULT hr;
				if FAILED(hr = ReopenVideo()) {
					return hr;
				}
			}
		}

		BuildDXVAOutputFormat();

		if (bChangeMT) {
			if (IsDXVASupported() && SUCCEEDED(FindDecoderConfiguration())) {
				dynamic_cast<CVideoDecOutputPin*>(m_pOutput)->Recommit();
				m_nDXVAMode = MODE_DXVA2;
			} else {
				SAFE_DELETE (m_pDXVADecoder);
			}
		}
	}

	return S_OK;
}

void CMPCVideoDecFilter::BuildDXVAOutputFormat()
{
	SAFE_DELETE_ARRAY (m_pVideoOutputFormat);

	// === New swscaler options
	int nSwOF = 0;
	int	nSwIndex[6];
	int nSwCount = 0;
	// get the output formats order from the DWORD nibbles extracting literal values [0x00543210] = 0,1,2,3,4,5
	// get the output formats count from the DWORD nibbles extracting checked flag (8) [0x0054ba98] = 4
	for (int i=0; i<6; i++) {
		nSwOF = ( m_nSwOutputFormats & (0x0000000F << (4*i)) ) >> (4*i);
		if ((nSwOF & 8) !=0) {
			nSwIndex[nSwCount]=nSwOF & ~8;
			nSwCount++;
		}
	}
	m_nVideoOutputCount = (IsDXVASupported() ? ffCodecs[m_nCodecNb].DXVAModeCount() + _countof (DXVAFormats) : 0) +
						  (m_bUseFFmpeg   ? (nSwCount>0) ? nSwCount : _countof(SoftwareFormats) : 0);

	m_pVideoOutputFormat = DNew VIDEO_OUTPUT_FORMATS[m_nVideoOutputCount];

	int nPos = 0;
	if (IsDXVASupported()) {
		// Dynamic DXVA media types for DXVA1
		for (nPos=0; nPos<ffCodecs[m_nCodecNb].DXVAModeCount(); nPos++) {
			m_pVideoOutputFormat[nPos].subtype			= ffCodecs[m_nCodecNb].DXVAModes->Decoder[nPos];
			m_pVideoOutputFormat[nPos].biCompression	= 'avxd';
			m_pVideoOutputFormat[nPos].biBitCount		= 12;
			m_pVideoOutputFormat[nPos].biPlanes			= 1;
		}

		// Static list for DXVA2
		memcpy (&m_pVideoOutputFormat[nPos], DXVAFormats, sizeof(DXVAFormats));
		nPos += _countof (DXVAFormats);
	}
	// Software rendering
	if (m_bUseFFmpeg) {
		if (nSwCount>0) {
			for (int i=0; i<nSwCount; i++) {
				m_pVideoOutputFormat[nPos + i] = SoftwareFormats[nSwIndex[i]];
			}
		} else {
			memcpy (&m_pVideoOutputFormat[nPos], SoftwareFormats, sizeof(SoftwareFormats));
		}
	}
}

int CMPCVideoDecFilter::GetPicEntryNumber()
{
	if (IsDXVASupported()) {
		return IsWinVistaOrLater()
				? ffCodecs[m_nCodecNb].DXVAModes->PicEntryNumber_DXVA2
				: ffCodecs[m_nCodecNb].DXVAModes->PicEntryNumber_DXVA1;
	} else {
		return 0;
	}
}

void CMPCVideoDecFilter::GetOutputFormats (int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats)
{
	nNumber		= m_nVideoOutputCount;
	*ppFormats	= m_pVideoOutputFormat;
}

void CMPCVideoDecFilter::AllocExtradata(AVCodecContext* pAVCtx, const CMediaType* pmt)
{
	// code from LAV ...
	// Process Extradata
	BYTE *extra = NULL;
	unsigned int extralen = 0;
	getExtraData((const BYTE *)pmt->Format(), pmt->FormatType(), pmt->FormatLength(), NULL, &extralen);

	BOOL bH264avc = FALSE;
	if (extralen > 0) {
		TRACE(_T("CMPCVideoDecFilter::AllocExtradata() : processing extradata of %d bytes\n"), extralen);
		// Reconstruct AVC1 extradata format
		if (pmt->formattype == FORMAT_MPEG2Video && (m_pAVCtx->codec_tag == MAKEFOURCC('a','v','c','1') || m_pAVCtx->codec_tag == MAKEFOURCC('A','V','C','1') || m_pAVCtx->codec_tag == MAKEFOURCC('C','C','V','1'))) {
			MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)pmt->Format();
			extralen += 7;
			extra = (uint8_t *)av_mallocz(extralen + FF_INPUT_BUFFER_PADDING_SIZE);
			extra[0] = 1;
			extra[1] = (BYTE)mp2vi->dwProfile;
			extra[2] = 0;
			extra[3] = (BYTE)mp2vi->dwLevel;
			extra[4] = (BYTE)(mp2vi->dwFlags ? mp2vi->dwFlags : 2) - 1;

			// Actually copy the metadata into our new buffer
			unsigned int actual_len;
			getExtraData((const BYTE *)pmt->Format(), pmt->FormatType(), pmt->FormatLength(), extra+6, &actual_len);

			// Count the number of SPS/PPS in them and set the length
			// We'll put them all into one block and add a second block with 0 elements afterwards
			// The parsing logic does not care what type they are, it just expects 2 blocks.
			BYTE *p = extra+6, *end = extra+6+actual_len;
			BOOL bSPS = FALSE, bPPS = FALSE;
			int count = 0;
			while (p+1 < end) {
				unsigned len = (((unsigned)p[0] << 8) | p[1]) + 2;
				if (p + len > end) {
					break;
				}
				if ((p[2] & 0x1F) == 7)
					bSPS = TRUE;
				if ((p[2] & 0x1F) == 8)
					bPPS = TRUE;
				count++;
				p += len;
			}
			extra[5] = count;
			extra[extralen-1] = 0;

			bH264avc = TRUE;
			m_h264RandomAccess.SetAVCNALSize(mp2vi->dwFlags);
		} else {
			// Just copy extradata for other formats
			extra = (uint8_t *)av_mallocz(extralen + FF_INPUT_BUFFER_PADDING_SIZE);
			getExtraData((const BYTE *)pmt->Format(), pmt->FormatType(), pmt->FormatLength(), extra, NULL);
		}
		// Hack to discard invalid MP4 metadata with AnnexB style video
		if (m_nCodecId == AV_CODEC_ID_H264 && !bH264avc && extra[0] == 1) {
			av_freep(&extra);
			extralen = 0;
		}
		m_pAVCtx->extradata = extra;
		m_pAVCtx->extradata_size = (int)extralen;
	}
}

HRESULT CMPCVideoDecFilter::CompleteConnect(PIN_DIRECTION direction, IPin* pReceivePin)
{
	if (direction == PINDIR_INPUT && m_pOutput->IsConnected()) {
		ReconnectOutput (PictWidth(), PictHeight());
	} else if (direction == PINDIR_OUTPUT) {
		DetectVideoCard_EVR(pReceivePin);

		if (IsDXVASupported()) {
			if (m_nDXVAMode == MODE_DXVA1) {
				m_pDXVADecoder->ConfigureDXVA1();
			} else if (SUCCEEDED (ConfigureDXVA2 (pReceivePin)) && SUCCEEDED (SetEVRForDXVA2 (pReceivePin)) ) {
				m_nDXVAMode  = MODE_DXVA2;
			}
		}
		if (m_nDXVAMode == MODE_SOFTWARE && !m_bUseFFmpeg) {
			return VFW_E_INVALIDMEDIATYPE;
		}

		if (m_nDXVAMode == MODE_SOFTWARE && IsDXVASupported()) {
			HRESULT hr;
			if FAILED(hr = ReopenVideo()) {
				return hr;
			}
		}

		CLSID ClsidSourceFilter = GetCLSID(m_pInput->GetConnected());
		if ((ClsidSourceFilter == __uuidof(CMpegSourceFilter)) || (ClsidSourceFilter == __uuidof(CMpegSplitterFilter))) {
			m_bReorderBFrame = false;

			if (CComPtr<IBaseFilter> pFilter = GetFilterFromPin(m_pInput->GetConnected()) ) {
				if (CComQIPtr<IMpegSplitterFilter> MpegSplitterFilter = pFilter ) {
					m_bIsEVO = (m_nCodecId == AV_CODEC_ID_VC1 && mpeg_ps == MpegSplitterFilter->GetMPEGType());
				}
			}
		}

		if (m_nDXVAMode != MODE_SOFTWARE) {
			m_nOutCsp = FF_CSP_UNSUPPORTED;
		}
		
		// Cannot use YUY2 if horizontal or vertical resolution is not even
		if (((m_pOutput->CurrentMediaType().subtype == MEDIASUBTYPE_YUY2) && (m_pAVCtx->width&1 || m_pAVCtx->height&1))) {
			return VFW_E_INVALIDMEDIATYPE;
		}
	}

	return __super::CompleteConnect (direction, pReceivePin);
}

HRESULT CMPCVideoDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if (UseDXVA2()) {
		HRESULT					hr;
		ALLOCATOR_PROPERTIES	Actual;

		if (m_pInput->IsConnected() == FALSE) {
			return E_UNEXPECTED;
		}

		pProperties->cBuffers = GetPicEntryNumber();

		if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
			return hr;
		}

		return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
			   ? E_FAIL
			   : NOERROR;
	} else {
		return __super::DecideBufferSize (pAllocator, pProperties);
	}
}

HRESULT CMPCVideoDecFilter::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMPCVideoDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	return __super::EndFlush();
}

HRESULT CMPCVideoDecFilter::NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	
	if (m_pAVCtx) {
		avcodec_flush_buffers(m_pAVCtx);
	}

	if (m_pParser) {
		av_parser_close(m_pParser);
		m_pParser = av_parser_init(m_nCodecId);
	}

	if (m_pDXVADecoder) {
		m_pDXVADecoder->NewSegment();
	}
	
	m_nPosB = 1;
	memset(&m_BFrames, 0, sizeof(m_BFrames));
	m_rtLastStop		= 0;
	m_dRate				= dRate;

	m_h264RandomAccess.flush(m_pAVCtx->thread_count);

	m_bWaitingForKeyFrame = TRUE;

	m_rtStartCache = INVALID_TIME;

	m_rtPrevStop = 0;

	rm.video_after_seek	= true;
	m_rtStart			= rtStart;

	if (m_nCodecId == AV_CODEC_ID_H264 && m_nFrameType != PICT_FRAME && m_nPCIVendor == PCIV_ATI && m_nDXVAMode == MODE_DXVA2) {
		if (SUCCEEDED(FindDecoderConfiguration())) {
			dynamic_cast<CVideoDecOutputPin*>(m_pOutput)->Recommit();
		}
	}

	return __super::NewSegment (rtStart, rtStop, dRate);
}

HRESULT CMPCVideoDecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	if (m_nDXVAMode == MODE_SOFTWARE) {
		REFERENCE_TIME rtStart = 0, rtStop = 0;
		SoftwareDecode(NULL, NULL, 0, rtStart, rtStop);
	} else if (m_nDXVAMode == MODE_DXVA2 && m_pDXVADecoder) { // TODO - need check under WinXP on DXVA1
		m_pDXVADecoder->EndOfStream();
	}

	return __super::EndOfStream();
}

HRESULT CMPCVideoDecFilter::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT) {
		Cleanup();
	}

	return __super::BreakConnect (dir);
}

void CMPCVideoDecFilter::SetTypeSpecificFlags(IMediaSample* pMS)
{
	if (CComQIPtr<IMediaSample2> pMS2 = pMS) {
		AM_SAMPLE2_PROPERTIES props;
		if (SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))) {
			props.dwTypeSpecificFlags &= ~0x7f;

			switch (m_nDeinterlacing) {
				case AUTO :
					m_nFrameType = PICT_BOTTOM_FIELD;
					if (!m_pFrame->interlaced_frame) {
						props.dwTypeSpecificFlags		|= AM_VIDEO_FLAG_WEAVE;
						m_nFrameType					= PICT_FRAME;
					} else {
						if (m_pFrame->top_field_first) {
							props.dwTypeSpecificFlags	|= AM_VIDEO_FLAG_FIELD1FIRST;
							m_nFrameType				= PICT_TOP_FIELD;
						}
					}
					break;
				case PROGRESSIVE :
					props.dwTypeSpecificFlags	|= AM_VIDEO_FLAG_WEAVE;
					m_nFrameType				= PICT_FRAME;
					break;
				case TOPFIELD :
					props.dwTypeSpecificFlags	|= AM_VIDEO_FLAG_FIELD1FIRST;
					m_nFrameType				= PICT_TOP_FIELD;
					break;
				case BOTTOMFIELD :
					m_nFrameType = PICT_BOTTOM_FIELD;
			}

			switch (m_pFrame->pict_type) {
				case AV_PICTURE_TYPE_I :
				case AV_PICTURE_TYPE_SI :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_I_SAMPLE;
					break;
				case AV_PICTURE_TYPE_P :
				case AV_PICTURE_TYPE_SP :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_P_SAMPLE;
					break;
				default :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_B_SAMPLE;
					break;
			}

			pMS2->SetProperties(sizeof(props), (BYTE*)&props);
		}
	}
}

unsigned __int64 CMPCVideoDecFilter::GetCspFromMediaType(GUID& subtype)
{
	// === New swscaler options
	if (subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_YV12) {
		return (FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ);
	} else if (subtype == MEDIASUBTYPE_NV12) {
		return FF_CSP_NV12;
	} else if (subtype == MEDIASUBTYPE_YUY2) {
		return FF_CSP_YUY2;
	} else if (subtype == MEDIASUBTYPE_RGB32) {
		return FF_CSP_RGB32;
	} else if (subtype == MEDIASUBTYPE_RGB565) {
		return FF_CSP_RGB16;
	} else if (subtype == MEDIASUBTYPE_RGB555) {
		return FF_CSP_RGB15;
	}
	//
	ASSERT (FALSE);
	return FF_CSP_NULL;
}

void CMPCVideoDecFilter::InitSwscale()
{
	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	if (m_pSwsContext == NULL
		|| (m_pOutSize.cx != bihOut.biWidth)
		|| (m_pOutSize.cy != abs(bihOut.biHeight))) {

		if (m_pSwsContext) {
			sws_freeContext(m_pSwsContext);
			m_pSwsContext	= NULL;
			m_PixFmt		= AV_PIX_FMT_NB;
		}

		int sws_FlagsR = 0;
		int sws_FlagsO = 0;

		switch (m_nSwChromaToRGB) {
			case 0  :										// GUI 'Fast'
				sws_FlagsO = SWS_ACCURATE_RND;
				break;
			case 1  :										// GUI 'Normal'
			default :
				sws_FlagsO = SWS_FULL_CHR_H_INP | SWS_ACCURATE_RND;
				break;
			case 2  :										// GUI 'Full'
				sws_FlagsO = SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP | SWS_ACCURATE_RND;
				break;
		}

		switch (m_nSwResizeMethodBE) {
			case 0  :										// GUI 'Area'
				sws_FlagsR = SWS_AREA;
				break;
			case 1  :										// GUI 'Bicubic'
				sws_FlagsR = SWS_BICUBIC;
				break;
			case 2  :										// GUI 'Bilinear'
			default :
				sws_FlagsR = SWS_BILINEAR;
				break;
			case 3  :										// GUI 'Fast Bilinear'
				sws_FlagsR = SWS_FAST_BILINEAR;
				break;
			case 4  :										// GUI 'Gauss'
				sws_FlagsR = SWS_GAUSS;
				break;
			case 5  :										// GUI 'Lanczos'
				sws_FlagsR = SWS_LANCZOS;
				break;
			case 6  :										// GUI 'Point'
				sws_FlagsR = SWS_POINT;
				break;
			case 7  :										// GUI 'Sinc'
				sws_FlagsR = SWS_SINC;
				break;
			case 8  :										// GUI 'Spline'
				sws_FlagsR = SWS_SPLINE;
				break;
			case 9  :										// GUI 'X'
				sws_FlagsR = SWS_X;
				break;
		}
		
		int sws_Flags = sws_FlagsR | sws_FlagsO;
		
		m_nOutCsp = GetCspFromMediaType(m_pOutput->CurrentMediaType().subtype);

		if (m_nDialogHWND) {
			EnableWindow(GetDlgItem(m_nDialogHWND, IDC_PP_RESIZEMETHODBE), TRUE);
			EnableWindow(GetDlgItem(m_nDialogHWND, IDC_PP_SWCHROMATORGB), (m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp)));
			EnableWindow(GetDlgItem(m_nDialogHWND, IDC_PP_SWCOLORSPACE), (m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp)));
			EnableWindow(GetDlgItem(m_nDialogHWND, IDC_PP_SWINPUTLEVELS), (m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp)));
			EnableWindow(GetDlgItem(m_nDialogHWND, IDC_PP_SWOUTPUTLEVELS), (m_nOutCsp == 0 || csp_isRGB_RGB(m_nOutCsp)));
		}

		m_PixFmt = csp_ffdshow2lavc(csp_lavc2ffdshow(m_pAVCtx->pix_fmt));
		if (m_PixFmt == AV_PIX_FMT_NB) {
			m_PixFmt = m_pAVCtx->pix_fmt;
		}

		m_pSwsContext = sws_getCachedContext(
							NULL,
							m_pAVCtx->width,
							m_pAVCtx->height,
							m_PixFmt,
							m_pAVCtx->width,
							m_pAVCtx->height,
							csp_ffdshow2lavc(m_nOutCsp),
							sws_Flags | SWS_PRINT_INFO,
							NULL,
							NULL,
							NULL);

		if (m_pSwsContext == NULL) {
			m_PixFmt = AV_PIX_FMT_NB;
			return;
		}

		m_nSwOutBpp		= bihOut.biBitCount;
		m_pOutSize.cx	= bihOut.biWidth;
		m_pOutSize.cy	= abs(bihOut.biHeight);

		int *inv_tbl = NULL, *tbl = NULL;
		int srcRange, dstRange, brightness, contrast, saturation;
		int ret = sws_getColorspaceDetails(m_pSwsContext, &inv_tbl, &srcRange, &tbl, &dstRange, &brightness, &contrast, &saturation);
		if (ret >= 0) {
			int nColorspace;
			if (m_nSwColorspace == 2) {
				nColorspace = PictWidth() > 768 ? SWS_CS_ITU709 : SWS_CS_ITU601;	// GUI 'Auto'
			} else {
				nColorspace = m_nSwColorspace == 1 ? SWS_CS_ITU709 : SWS_CS_ITU601;	// GUI 'HD(BT.709)' : 'SD(BT.601)'
			}
		
			dstRange = m_nSwOutputLevels>1 ? 0 : m_nSwOutputLevels; // GUI 'Auto' = 'TV(16-235)'
			srcRange = m_nSwInputLevels>1 ? 0 : m_nSwInputLevels; // GUI 'Auto' = 'TV(16-235)'
			sws_setColorspaceDetails(m_pSwsContext, sws_getCoefficients(nColorspace), srcRange, tbl, dstRange, brightness, contrast, saturation);
		}
	}
}

#define RM_SKIP_BITS(n)	(buffer<<=n)
#define RM_SHOW_BITS(n)	((buffer)>>(32-(n)))
static int rm_fix_timestamp(uint8_t *buf, int64_t timestamp, enum AVCodecID nCodecId, int64_t *kf_base, int *kf_pts)
{
	uint8_t *s = buf + 1 + (*buf+1)*8;
	uint32_t buffer = (s[0]<<24) + (s[1]<<16) + (s[2]<<8) + s[3];
	uint32_t kf = timestamp;
	int pict_type;
	uint32_t orig_kf;

	if (nCodecId == AV_CODEC_ID_RV30) {
		RM_SKIP_BITS(3);
		pict_type = RM_SHOW_BITS(2);
		RM_SKIP_BITS(2 + 7);
	} else {
		RM_SKIP_BITS(1);
		pict_type = RM_SHOW_BITS(2);
		RM_SKIP_BITS(2 + 7 + 3);
	}
	orig_kf = kf = RM_SHOW_BITS(13); // kf= 2*RM_SHOW_BITS(12);
	if (pict_type <= 1) {
		// I frame, sync timestamps:
		*kf_base = (int64_t)timestamp-kf;
		kf = timestamp;
	} else {
		// P/B frame, merge timestamps:
		int64_t tmp = (int64_t)timestamp - *kf_base;
		kf |= tmp&(~0x1fff); // combine with packet timestamp
		if (kf<tmp-4096) {
			kf += 8192;
		} else if (kf>tmp+4096) { // workaround wrap-around problems
			kf -= 8192;
		}
		kf += *kf_base;
	}
	if (pict_type != 3) { // P || I  frame -> swap timestamps
		uint32_t tmp=kf;
		kf = *kf_pts;
		*kf_pts = tmp;
	}

	return kf;
}

static int64_t process_rv_timestamp(RMDemuxContext *rm, enum AVCodecID nCodecId, uint8_t *buf, int64_t timestamp)
{
	if (rm->video_after_seek) {
		rm->kf_base = 0;
		rm->kf_pts = timestamp;
		rm->video_after_seek = false;
	}
	return rm_fix_timestamp(buf, timestamp, nCodecId, &rm->kf_base, &rm->kf_pts);
}

void copyPlane(BYTE *dstp, stride_t dst_pitch, const BYTE *srcp, stride_t src_pitch, int row_size, int height, bool flip = false)
{
	if (!flip) {
		for (int y=height; y>0; --y) {
			memcpy_sse(dstp, srcp, row_size);
			dstp += dst_pitch;
			srcp += src_pitch;
		}
	} else {
		dstp += dst_pitch * (height - 1);
		for (int y=height; y>0; --y) {
			memcpy_sse(dstp, srcp, row_size);
			dstp -= dst_pitch;
			srcp += src_pitch;
		}
	}
}

#define PULLDOWN_FLAG (m_nCodecId == AV_CODEC_ID_VC1 && m_bIsEVO && m_rtAvrTimePerFrame == 333666)

HRESULT CMPCVideoDecFilter::SoftwareDecode(IMediaSample* pIn, BYTE* pDataIn, int nSize, REFERENCE_TIME& rtStartIn, REFERENCE_TIME& rtStopIn)
{
	HRESULT			hr = S_OK;
	int				got_picture;
	int				used_bytes;
	BOOL			bFlush = (pDataIn == NULL);

	AVPacket		avpkt;
	av_init_packet(&avpkt);

	if (!bFlush && m_nCodecId == AV_CODEC_ID_H264) {
		if (!m_h264RandomAccess.searchRecoveryPoint(pDataIn, nSize)) {
			return S_OK;
		}
	}

	while (nSize > 0 || bFlush) {
		REFERENCE_TIME rtStart = rtStartIn, rtStop = rtStopIn;
		
		if (!bFlush) {
			if (nSize + FF_INPUT_BUFFER_PADDING_SIZE > m_nFFBufferSize) {
				m_nFFBufferSize	= nSize + FF_INPUT_BUFFER_PADDING_SIZE;
				m_pFFBuffer		= (BYTE*)av_realloc_f(m_pFFBuffer, m_nFFBufferSize, 1);
				if (!m_pFFBuffer) {
					m_nFFBufferSize = 0;
					return E_OUTOFMEMORY;
				}
			}

			// Required number of additionally allocated bytes at the end of the input bitstream for decoding.
			// This is mainly needed because some optimized bitstream readers read
			// 32 or 64 bit at once and could read over the end.
			// Note: If the first 23 bits of the additional bytes are not 0, then damaged
			// MPEG bitstreams could cause overread and segfault.
			memcpy_sse(m_pFFBuffer, pDataIn, nSize);
			memset(m_pFFBuffer + nSize, 0, FF_INPUT_BUFFER_PADDING_SIZE);

			avpkt.data	= m_pFFBuffer;
			avpkt.size	= nSize;
			avpkt.pts	= rtStartIn;
			avpkt.flags	= AV_PKT_FLAG_KEY;
		} else {
			avpkt.data	= NULL;
			avpkt.size	= 0;
		}

		// all Parser code from LAV ... thanks to it's author
		if (m_pParser) {
			BYTE *pOut		= NULL;
			int pOut_size	= 0;

			used_bytes = av_parser_parse2(m_pParser, m_pAVCtx, &pOut, &pOut_size, avpkt.data, avpkt.size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

			if (used_bytes == 0 && pOut_size == 0 && !bFlush) {
				TRACE(_T("CMPCVideoDecFilter::SoftwareDecode() - could not process buffer, starving?\n"));
				break;
			}

			// Update start time cache
			// If more data was read then output, update the cache (incomplete frame)
			// If output is bigger, a frame was completed, update the actual rtStart with the cached value, and then overwrite the cache
			if (used_bytes > pOut_size) {
				if (rtStartIn != INVALID_TIME) {
					m_rtStartCache = rtStartIn;
				}
			} else if (used_bytes == pOut_size || ((used_bytes + 9) == pOut_size)) {
				// Why +9 above?
				// Well, apparently there are some broken MKV muxers that like to mux the MPEG-2 PICTURE_START_CODE block (which is 9 bytes) in the package with the previous frame
				// This would cause the frame timestamps to be delayed by one frame exactly, and cause timestamp reordering to go wrong.
				// So instead of failing on those samples, lets just assume that 9 bytes are that case exactly.
				m_rtStartCache = rtStartIn = INVALID_TIME;
			} else if (pOut_size > used_bytes) {
				rtStart			= m_rtStartCache;
				m_rtStartCache	= rtStartIn;
				// The value was used once, don't use it for multiple frames, that ends up in weird timings
				rtStartIn		= INVALID_TIME;
			}

			if (pOut_size > 0 || bFlush) {

				if (pOut && pOut_size > 0) {
					if (pOut_size + FF_INPUT_BUFFER_PADDING_SIZE > m_nFFBufferSize2) {
						m_nFFBufferSize2	= pOut_size + FF_INPUT_BUFFER_PADDING_SIZE;
						m_pFFBuffer2		= (BYTE *)av_realloc_f(m_pFFBuffer2, m_nFFBufferSize2, 1);
						if (!m_pFFBuffer2) {
							m_nFFBufferSize2 = 0;
							return E_OUTOFMEMORY;
						}
					}
					memcpy(m_pFFBuffer2, pOut, pOut_size);
					memset(m_pFFBuffer2 + pOut_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

					avpkt.data	= m_pFFBuffer2;
					avpkt.size	= pOut_size;
					avpkt.pts	= rtStart;
				} else {
					avpkt.data	= NULL;
					avpkt.size	= 0;
				}

				int ret2 = avcodec_decode_video2 (m_pAVCtx, m_pFrame, &got_picture, &avpkt);
				if (ret2 < 0) {
					TRACE(_T("CMPCVideoDecFilter::SoftwareDecode() - decoding failed despite successfull parsing\n"));
					got_picture = 0;
				}
			} else {
				got_picture = 0;
			}
		} else {
			used_bytes = avcodec_decode_video2 (m_pAVCtx, m_pFrame, &got_picture, &avpkt);
		}

		if (used_bytes < 0) {
			return S_OK;
		}

		// Comment from LAV Video code:
		// When Frame Threading, we won't know how much data has been consumed, so it by default eats everything.
		// In addition, if no data got consumed, and no picture was extracted, the frame probably isn't all that useufl.
		// The MJPEB decoder is somewhat buggy and doesn't let us know how much data was consumed really...
		if ((!m_pParser && (m_pAVCtx->active_thread_type & FF_THREAD_FRAME || (!got_picture && used_bytes == 0))) || m_nCodecId == AV_CODEC_ID_MJPEGB || bFlush) {
			nSize = 0;
		} else {
			nSize   -= used_bytes;
			pDataIn += used_bytes;
		}

		bool fWaitKeyFrame =	m_nCodecId == AV_CODEC_ID_VC1
							 || m_nCodecId == AV_CODEC_ID_MPEG2VIDEO
							 || m_nCodecId == AV_CODEC_ID_RV30
							 || m_nCodecId == AV_CODEC_ID_RV40
							 || m_nCodecId == AV_CODEC_ID_VP3
							 || m_nCodecId == AV_CODEC_ID_THEORA
							 || m_nCodecId == AV_CODEC_ID_MPEG4;

		if (m_nCodecId == AV_CODEC_ID_H264) {
			m_h264RandomAccess.judgeFrameUsability(m_pFrame, &got_picture);
		} else if (fWaitKeyFrame) {
			if (m_bWaitingForKeyFrame && got_picture) {
				if (m_pFrame->key_frame) {
					m_bWaitingForKeyFrame = FALSE;
				} else {
					got_picture = 0;
				}
			}
		}

		if (!got_picture || !m_pFrame->data[0]) {
			if (!avpkt.size) {
				bFlush = FALSE; // End flushing, no more frames
			}
			continue;
		}

		if ((m_nCodecId == AV_CODEC_ID_RV10 || m_nCodecId == AV_CODEC_ID_RV20) && m_pFrame->pict_type == AV_PICTURE_TYPE_B) {
			rtStart = m_rtPrevStop;
		} else if ((m_nCodecId == AV_CODEC_ID_RV30 || m_nCodecId == AV_CODEC_ID_RV40) && avpkt.data) {
			rtStart = m_pFrame->pkt_pts;
			rtStart = (rtStart == INVALID_TIME) ? m_rtPrevStop : (10000i64*process_rv_timestamp(&rm, m_nCodecId, avpkt.data, (rtStart + m_rtStart)/10000i64) - m_rtStart);
		} else if (!PULLDOWN_FLAG) {
			rtStart = m_pFrame->pkt_pts;
		}

		ReorderBFrames(rtStart, rtStop);

		UpdateFrameTime(rtStart, rtStop, PULLDOWN_FLAG);

		m_rtPrevStop = rtStop;

		if ((pIn && pIn->IsPreroll() == S_OK) || rtStart < 0) {
			continue;
		}

		CComPtr<IMediaSample>	pOut;
		BYTE*					pDataOut = NULL;

		UpdateAspectRatio();
		if (FAILED(hr = GetDeliveryBuffer(m_pAVCtx->width, m_pAVCtx->height, &pOut, GetDuration())) || FAILED(hr = pOut->GetPointer(&pDataOut))) {
			continue;//return hr;
		}

		pOut->SetTime(&rtStart, &rtStop);
		pOut->SetMediaTime(NULL, NULL);

		// change colorspace details/output format
		{
			//soft refresh - signal new swscaler colorspace details
			if (m_nSwRefresh == 1){
				m_nSwRefresh--;
				if (m_pSwsContext) {
					sws_freeContext(m_pSwsContext);
					m_pSwsContext	= NULL;
					m_PixFmt		= AV_PIX_FMT_NB;
				}
			}

			// hard refresh - signal new output format
			if (m_nSwRefresh == 2){
				m_nSwRefresh--;
				CComPtr<IPin> pRendererPin;
				CComPtr<IPinConnection> pRendererConn;
				CMediaType cmtRenderer;

				BuildDXVAOutputFormat(); // refresh supported media types

				CAutoLock cObjectLock(m_pLock);

				cmtRenderer.InitMediaType();
				GetMediaType(0, &cmtRenderer);

 				m_pOutput->ConnectedTo(&pRendererPin);
				hr = pRendererPin->QueryInterface(IID_IPinConnection, (void**)&pRendererConn);
				if (FAILED(hr))	{
					// madVR accepts dynamic media type changes but does not support IPinConnection
					if (S_OK == (hr = pRendererPin->QueryAccept(&cmtRenderer))) {
						if (S_OK == (hr = m_pOutput->SetMediaType(&cmtRenderer))) {
							ReconnectOutput(PictWidth(), PictHeight(), true, true);
						}
					}
					return hr;
				}

				if (S_OK == (hr = NotifyEvent(EC_DISPLAY_CHANGED, (LONG_PTR)m_pOutput->GetConnected(), 0))) {
					SleepEx(200, TRUE);
					hr = m_pOutput->SetMediaType(&cmtRenderer);
				}

				/*
				// VMR accepts dynamic media type changes - but failed QueryAccept()
				hr = pRendererConn->DynamicQueryAccept(&cmtRenderer);
				if (SUCCEEDED(hr)) {
					//VMR accepts dynamic media type changes.
					if (S_OK == (hr = m_pOutput->SetMediaType(&cmtRenderer))) {
						ReconnectOutput(PictWidth(), PictHeight(), true, true);
					}
				}
				*/

				return hr;
			}
		}

		AVPixelFormat PixFmt = csp_ffdshow2lavc(csp_lavc2ffdshow(m_pAVCtx->pix_fmt));
		if (PixFmt == AV_PIX_FMT_NB) {
			PixFmt = m_pAVCtx->pix_fmt;
		}

		if ((m_PixFmt != AV_PIX_FMT_NB) && (PixFmt != m_PixFmt)) {
			sws_freeContext(m_pSwsContext);
			m_pSwsContext	= NULL;
			m_PixFmt		= AV_PIX_FMT_NB;
		}

		InitSwscale();

		if (m_pSwsContext != NULL) {

			int outStride = m_pOutSize.cx;
			BYTE *outData = pDataOut;

			// From LAVVideo ...
			// Check if we have proper pixel alignment and the dst memory is actually aligned
			if (FFALIGN(outStride, 16) != outStride || ((uintptr_t)pDataOut % 16u)) {
				outStride = FFALIGN(outStride, 16);
				int requiredSize = (outStride * m_pAVCtx->height * m_nSwOutBpp) << 3;
				if (requiredSize > m_nAlignedFFBufferSize) {
					av_freep(&m_pAlignedFFBuffer);
					m_nAlignedFFBufferSize	= requiredSize;
					m_pAlignedFFBuffer		= (BYTE*)av_malloc(m_nAlignedFFBufferSize+FF_INPUT_BUFFER_PADDING_SIZE);
				}
				outData = m_pAlignedFFBuffer;
			}

			uint8_t*	dst[4] = {NULL, NULL, NULL, NULL};
			stride_t	dstStride[4] = {0, 0, 0, 0};
			const TcspInfo *outcspInfo=csp_getInfo(m_nOutCsp);

			// === New swscaler options
			if (m_nOutCsp == FF_CSP_YUY2 || m_nOutCsp == FF_CSP_RGB32 || m_nOutCsp == FF_CSP_RGBA || m_nOutCsp == FF_CSP_RGB16 || m_nOutCsp == FF_CSP_RGB15) {
				dst[0] = outData;
				dstStride[0] = (m_nSwOutBpp>>3) * (outStride);
			} else {
				for (unsigned int i=0; i<outcspInfo->numPlanes; i++) {
					dstStride[i] = outStride >> outcspInfo->shiftX[i];
					dst[i] = !i ? outData : dst[i-1] + dstStride[i-1] * (m_pOutSize.cy >> outcspInfo->shiftY[i-1]) ;
				}

				if (m_nOutCsp & FF_CSP_420P) {
					std::swap(dst[1], dst[2]);
				}
			}

			sws_scale(m_pSwsContext, m_pFrame->data, m_pFrame->linesize, 0, m_pAVCtx->height, dst, dstStride);

			if (outData != pDataOut) {
				if (m_nOutCsp & FF_CSP_420P) {
					std::swap(dst[1], dst[2]);
				}
				int rowsize = 0, height = 0;
				for (unsigned int i=0; i<outcspInfo->numPlanes; i++) {
					rowsize	= (m_pOutSize.cx*outcspInfo->Bpp) >> outcspInfo->shiftX[i];
					height	= m_pAVCtx->height >> outcspInfo->shiftY[i];
					copyPlane(pDataOut, rowsize, dst[i], (outStride*outcspInfo->Bpp) >> outcspInfo->shiftX[i], rowsize, height, (m_nOutCsp == FF_CSP_RGB32));
					pDataOut += rowsize * height;
				}
			}
		}

#if defined(_DEBUG) && 0
		static REFERENCE_TIME	rtLast = 0;
		TRACE ("Deliver : %10I64d - %10I64d   (%10I64d)  {%10I64d}\n", rtStart, rtStop,
			   rtStop - rtStart, rtStart - rtLast);
		rtLast = rtStart;
#endif

		SetTypeSpecificFlags (pOut);
		hr = m_pOutput->Deliver(pOut);
	}

	return hr;
}

// reopen video codec - reset the threads count and dxva flag
HRESULT CMPCVideoDecFilter::ReopenVideo()
{
	if (m_pAVCtx) {
		m_bUseDXVA = false;
		avcodec_close(m_pAVCtx);
		m_pAVCtx->using_dxva = false;
		int nThreadNumber = m_nThreadNumber ? m_nThreadNumber : m_pCpuId->GetProcessorNumber() * 3/2;
		if ((nThreadNumber > 1) && FFGetThreadType(m_nCodecId)) {
			FFSetThreadNumber(m_pAVCtx, m_nCodecId, nThreadNumber);
		}
		if (avcodec_open2(m_pAVCtx, m_pAVCodec, NULL) < 0) {
			return VFW_E_INVALIDMEDIATYPE;
		}
	}

	return S_OK;
}

HRESULT CMPCVideoDecFilter::Transform(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);
	HRESULT			hr;
	BYTE*			pDataIn;
	int				nSize;
	REFERENCE_TIME	rtStart	= INVALID_TIME;
	REFERENCE_TIME	rtStop	= INVALID_TIME;

	if (FAILED(hr = pIn->GetPointer(&pDataIn))) {
		return hr;
	}

	nSize = pIn->GetActualDataLength();
	// Skip empty packet
	if (nSize == 0) {
		return S_OK;
	}

	hr = pIn->GetTime(&rtStart, &rtStop);

	if (FAILED(hr)) {
		rtStart = rtStop = INVALID_TIME;
	}

	if (m_pAVCtx->has_b_frames) {
		m_BFrames[m_nPosB].rtStart	= rtStart;
		m_BFrames[m_nPosB].rtStop	= rtStop;
		m_nPosB						= 1-m_nPosB;
	}

	switch (m_nDXVAMode) {
		case MODE_SOFTWARE :
			hr = SoftwareDecode (pIn, pDataIn, nSize, rtStart, rtStop);
			break;
		case MODE_DXVA1 :
		case MODE_DXVA2 :
			{
				CheckPointer (m_pDXVADecoder, E_UNEXPECTED);
				UpdateAspectRatio();

				// Change aspect ratio for DXVA1
				// stupid DXVA1 - size for the output MediaType should be the same that size of DXVA surface
				if (m_nDXVAMode == MODE_DXVA1 && ReconnectOutput(PictWidthRounded(), PictHeightRounded(), true, false, GetDuration(), PictWidth(), PictHeight(), true) == S_OK) {
					m_pDXVADecoder->ConfigureDXVA1();
				}

				int width	= PictWidthRounded();
				int Height	= PictHeightRounded();

				hr = m_pDXVADecoder->DecodeFrame (pDataIn, nSize, rtStart, rtStop);

				if (width != PictWidthRounded() || Height != PictHeightRounded()) {
					FindDecoderConfiguration();
					dynamic_cast<CVideoDecOutputPin*>(m_pOutput)->Recommit();
					ReconnectOutput(PictWidth(), PictHeight());
				}
			}
			break;
		default :
			ASSERT (FALSE);
			hr = E_UNEXPECTED;
	}

	return hr;
}

void CMPCVideoDecFilter::UpdateAspectRatio()
{
	if (m_nARMode) {
		bool bSetAR = true;
		if (m_nARMode == 2) {
			CMediaType& mt = m_pInput->CurrentMediaType();
			if (mt.formattype == FORMAT_VideoInfo2 || mt.formattype == FORMAT_MPEG2_VIDEO || mt.formattype == FORMAT_DiracVideoInfo) {
				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.pbFormat;
				bSetAR = (!vih2->dwPictAspectRatioX && !vih2->dwPictAspectRatioY);
			}
			if (!bSetAR && (m_nARX && m_nARY)) {
				CSize aspect(m_nARX, m_nARY);
				int lnko = LNKO(aspect.cx, aspect.cy);
				if (lnko > 1) {
					aspect.cx /= lnko, aspect.cy /= lnko;
				}
				SetAspect(aspect);			
			}
		}

		if (bSetAR) {
			if (m_pAVCtx && (m_pAVCtx->sample_aspect_ratio.num > 0) && (m_pAVCtx->sample_aspect_ratio.den > 0)) {
				CSize aspect(m_pAVCtx->sample_aspect_ratio.num * m_pAVCtx->width, m_pAVCtx->sample_aspect_ratio.den * m_pAVCtx->height);
				int lnko = LNKO(aspect.cx, aspect.cy);
				if (lnko > 1) {
					aspect.cx /= lnko, aspect.cy /= lnko;
				}
				SetAspect(aspect);
			}
		}
	} else if (m_nARX && m_nARY) {
		CSize aspect(m_nARX, m_nARY);
		int lnko = LNKO(aspect.cx, aspect.cy);
		if (lnko > 1) {
			aspect.cx /= lnko, aspect.cy /= lnko;
		}
		SetAspect(aspect);
	}
}

void CMPCVideoDecFilter::ReorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	// Re-order B-frames if needed
	if (m_pAVCtx->has_b_frames && m_bReorderBFrame) {
		rtStart	= m_BFrames [m_nPosB].rtStart;
		rtStop	= m_BFrames [m_nPosB].rtStop;
	}
}

void CMPCVideoDecFilter::FillInVideoDescription(DXVA2_VideoDesc *pDesc)
{
	memset (pDesc, 0, sizeof(DXVA2_VideoDesc));
	pDesc->SampleWidth			= PictWidthRounded();
	pDesc->SampleHeight			= PictHeightRounded();
	pDesc->Format				= D3DFMT_A8R8G8B8;
	pDesc->UABProtectionLevel	= 1;
}

BOOL CMPCVideoDecFilter::IsSupportedDecoderMode(const GUID& mode)
{
	if (IsDXVASupported()) {
		for (int i=0; i<MAX_SUPPORTED_MODE; i++) {
			if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == GUID_NULL) {
				break;
			} else if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == mode) {
				return true;
			}
		}
	}

	return false;
}

BOOL CMPCVideoDecFilter::IsSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered)
{
	bool bRet = false;

	bRet = (nD3DFormat == MAKEFOURCC('N', 'V', '1', '2') || nD3DFormat == MAKEFOURCC('I', 'M', 'C', '3'));

	bIsPrefered = (config.ConfigBitstreamRaw == ffCodecs[m_nCodecNb].DXVAModes->PreferedConfigBitstream);
	return bRet;
}

HRESULT CMPCVideoDecFilter::FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
		const GUID& guidDecoder,
		DXVA2_ConfigPictureDecode *pSelectedConfig,
		BOOL *pbFoundDXVA2Configuration)
{
	HRESULT hr = S_OK;
	UINT cFormats = 0;
	UINT cConfigurations = 0;
	bool bIsPrefered = false;

	D3DFORMAT                   *pFormats = NULL;			// size = cFormats
	DXVA2_ConfigPictureDecode   *pConfig = NULL;			// size = cConfigurations

	// Find the valid render target formats for this decoder GUID.
	hr = pDecoderService->GetDecoderRenderTargets(guidDecoder, &cFormats, &pFormats);

	if (SUCCEEDED(hr)) {
		// Look for a format that matches our output format.
		for (UINT iFormat = 0; iFormat < cFormats;  iFormat++) {

			// Fill in the video description. Set the width, height, format, and frame rate.
			FillInVideoDescription(&m_VideoDesc); // Private helper function.
			m_VideoDesc.Format = pFormats[iFormat];

			// Get the available configurations.
			hr = pDecoderService->GetDecoderConfigurations(guidDecoder, &m_VideoDesc, NULL, &cConfigurations, &pConfig);

			if (FAILED(hr)) {
				continue;
			}

			// Find a supported configuration.
			for (UINT iConfig = 0; iConfig < cConfigurations; iConfig++) {
				if (IsSupportedDecoderConfig(pFormats[iFormat], pConfig[iConfig], bIsPrefered)) {
					// This configuration is good.
					if (bIsPrefered || !*pbFoundDXVA2Configuration) {
						*pbFoundDXVA2Configuration = TRUE;
						*pSelectedConfig = pConfig[iConfig];
					}

					if (bIsPrefered) {
						break;
					}
				}
			}

			CoTaskMemFree(pConfig);
		} // End of formats loop.
	}

	CoTaskMemFree(pFormats);

	// Note: It is possible to return S_OK without finding a configuration.
	return hr;
}

HRESULT CMPCVideoDecFilter::ConfigureDXVA2(IPin *pPin)
{
	HRESULT hr						 = S_OK;

	CComPtr<IMFGetService>					pGetService;
	CComPtr<IDirect3DDeviceManager9>		pDeviceManager;
	CComPtr<IDirectXVideoDecoderService>	pDecoderService;
	HANDLE									hDevice = INVALID_HANDLE_VALUE;

	// Query the pin for IMFGetService.
	hr = pPin->QueryInterface(__uuidof(IMFGetService), (void**)&pGetService);

	// Get the Direct3D device manager.
	if (SUCCEEDED(hr)) {
		hr = pGetService->GetService(
				 MR_VIDEO_ACCELERATION_SERVICE,
				 __uuidof(IDirect3DDeviceManager9),
				 (void**)&pDeviceManager);
	}

	// Open a new device handle.
	if (SUCCEEDED(hr)) {
		hr = pDeviceManager->OpenDeviceHandle(&hDevice);
	}

	// Get the video decoder service.
	if (SUCCEEDED(hr)) {
		hr = pDeviceManager->GetVideoService(
				 hDevice,
				 __uuidof(IDirectXVideoDecoderService),
				 (void**)&pDecoderService);
	}

	if (SUCCEEDED(hr)) {
		m_pDeviceManager	= pDeviceManager;
		m_pDecoderService	= pDecoderService;
		m_hDevice			= hDevice;
		hr					= FindDecoderConfiguration();
	}

	if (FAILED(hr)) {
		if (hDevice != INVALID_HANDLE_VALUE) {
			pDeviceManager->CloseDeviceHandle(hDevice);
		}
		m_pDeviceManager		= NULL;
		m_pDecoderService		= NULL;
		m_hDevice				= INVALID_HANDLE_VALUE;
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::SetEVRForDXVA2(IPin *pPin)
{
    IMFGetService* pGetService;
    HRESULT hr = pPin->QueryInterface(__uuidof(IMFGetService), reinterpret_cast<void**>(&pGetService));
	if (SUCCEEDED(hr)) {
		IDirectXVideoMemoryConfiguration* pVideoConfig;
		hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_IDirectXVideoMemoryConfiguration, reinterpret_cast<void**>(&pVideoConfig));
		if (SUCCEEDED(hr)) {
			// Notify the EVR.
			DXVA2_SurfaceType surfaceType;
			DWORD dwTypeIndex = 0;
			for (;;) {
				hr = pVideoConfig->GetAvailableSurfaceTypeByIndex(dwTypeIndex, &surfaceType);
				if (FAILED(hr)) {
					break;
				}
				if (surfaceType == DXVA2_SurfaceType_DecoderRenderTarget) {
					hr = pVideoConfig->SetSurfaceType(DXVA2_SurfaceType_DecoderRenderTarget);
					break;
				}
				++dwTypeIndex;
			}
			pVideoConfig->Release();
		}
		pGetService->Release();
    }
	return hr;
}

HRESULT CMPCVideoDecFilter::CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets)
{
	TRACE(_T("CMPCVideoDecFilter::CreateDXVA2Decoder()\n"));

	HRESULT							hr;
	CComPtr<IDirectXVideoDecoder>	pDirectXVideoDec;

	m_pDecoderRenderTarget	= NULL;

	if (m_pDXVADecoder) {
		m_pDXVADecoder->SetDirectXVideoDec(NULL);
	}

	hr = m_pDecoderService->CreateVideoDecoder (m_DXVADecoderGUID, &m_VideoDesc, &m_DXVA2Config,
			pDecoderRenderTargets, nNumRenderTargets, &pDirectXVideoDec);

	if (SUCCEEDED(hr)) {
		// need recreate dxva decoder after "stop" on Intel HD Graphics
		SAFE_DELETE (m_pDXVADecoder);
		m_pDXVADecoder = CDXVADecoder::CreateDecoder(this, pDirectXVideoDec, &m_DXVADecoderGUID, GetPicEntryNumber(), &m_DXVA2Config);
		if (m_pDXVADecoder) {
			m_pDXVADecoder->SetDirectXVideoDec(pDirectXVideoDec);
		} else {
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::FindDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat)
{
	HRESULT			hr				= E_FAIL;
	DWORD			dwFormats		= 0;
	DDPIXELFORMAT*	pPixelFormats	= NULL;


	pAMVideoAccelerator->GetUncompFormatsSupported (guidDecoder, &dwFormats, NULL);
	if (dwFormats > 0) {
		// Find the valid render target formats for this decoder GUID.
		pPixelFormats = DNew DDPIXELFORMAT[dwFormats];
		hr = pAMVideoAccelerator->GetUncompFormatsSupported (guidDecoder, &dwFormats, pPixelFormats);
		if (SUCCEEDED(hr)) {
			// Look for a format that matches our output format.
			for (DWORD iFormat = 0; iFormat < dwFormats; iFormat++) {
				if (pPixelFormats[iFormat].dwFourCC == MAKEFOURCC ('N', 'V', '1', '2')) {
					memcpy (pPixelFormat, &pPixelFormats[iFormat], sizeof(DDPIXELFORMAT));
					SAFE_DELETE_ARRAY(pPixelFormats)
					return S_OK;
				}
			}

			SAFE_DELETE_ARRAY(pPixelFormats);
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::CheckDXVA1Decoder(const GUID *pGuid)
{
	if (m_nCodecNb != -1) {
		for (int i=0; i<MAX_SUPPORTED_MODE; i++)
			if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == *pGuid) {
				return S_OK;
			}
	}

	return E_INVALIDARG;
}

void CMPCVideoDecFilter::SetDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat)
{
	m_DXVADecoderGUID		= *pGuid;
	memcpy (&m_PixelFormat, pPixelFormat, sizeof (DDPIXELFORMAT));
}

WORD CMPCVideoDecFilter::GetDXVA1RestrictedMode()
{
	if (m_nCodecNb != -1) {
		for (int i=0; i<MAX_SUPPORTED_MODE; i++)
			if (*ffCodecs[m_nCodecNb].DXVAModes->Decoder[i] == m_DXVADecoderGUID) {
				return ffCodecs[m_nCodecNb].DXVAModes->RestrictedMode [i];
			}
	}

	return DXVA_RESTRICTED_MODE_UNRESTRICTED;
}

HRESULT CMPCVideoDecFilter::CreateDXVA1Decoder(IAMVideoAccelerator*  pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount)
{
	if (m_pDXVADecoder && m_DXVADecoderGUID	== *pDecoderGuid) {
		return S_OK;
	}
	SAFE_DELETE (m_pDXVADecoder);

	if (!m_bUseDXVA) {
		return E_FAIL;
	}

	m_nDXVAMode			= MODE_DXVA1;
	m_DXVADecoderGUID	= *pDecoderGuid;
	m_pDXVADecoder		= CDXVADecoder::CreateDecoder (this, pAMVideoAccelerator, &m_DXVADecoderGUID, dwSurfaceCount);

	return S_OK;
}

// ISpecifyPropertyPages2

STDMETHODIMP CMPCVideoDecFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

#ifdef REGISTER_FILTER
	pPages->cElems		= 2;
#else
	pPages->cElems		= 1;
#endif

	pPages->pElems		= (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0]	= __uuidof(CMPCVideoDecSettingsWnd);
	if (pPages->cElems>1) {
		pPages->pElems[1]	= __uuidof(CMPCVideoDecCodecWnd);
	}

	return S_OK;
}

STDMETHODIMP CMPCVideoDecFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMPCVideoDecSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMPCVideoDecSettingsWnd>(NULL, &hr))->AddRef();
	} else if (guid == __uuidof(CMPCVideoDecCodecWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMPCVideoDecCodecWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

void CMPCVideoDecFilter::SetFrameType(FF_FIELD_TYPE nFrameType)
{
	m_nFrameType = nFrameType;
}

// EVR functions
HRESULT CMPCVideoDecFilter::DetectVideoCard_EVR(IPin *pPin)
{
	IMFGetService* pGetService;
	HRESULT hr = pPin->QueryInterface(__uuidof(IMFGetService), reinterpret_cast<void**>(&pGetService));
	if (SUCCEEDED(hr)) {
		// Try to get the adapter description of the active DirectX 9 device.
		IDirect3DDeviceManager9* pDevMan9;
		hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_IDirect3DDeviceManager9, reinterpret_cast<void**>(&pDevMan9));
		if (SUCCEEDED(hr)) {
			HANDLE hDevice;
			hr = pDevMan9->OpenDeviceHandle(&hDevice);
			if (SUCCEEDED(hr)) {
				IDirect3DDevice9* pD3DDev9;
				hr = pDevMan9->LockDevice(hDevice, &pD3DDev9, TRUE);
				if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
					// Invalid device handle. Try to open a new device handle.
					hr = pDevMan9->CloseDeviceHandle(hDevice);
					if (SUCCEEDED(hr)) {
						hr = pDevMan9->OpenDeviceHandle(&hDevice);
						// Try to lock the device again.
						if (SUCCEEDED(hr)) {
							hr = pDevMan9->LockDevice(hDevice, &pD3DDev9, TRUE);
						}
					}
				}
				if (SUCCEEDED(hr)) {
					D3DDEVICE_CREATION_PARAMETERS DevPar9;
					hr = pD3DDev9->GetCreationParameters(&DevPar9);
					if (SUCCEEDED(hr)) {
						IDirect3D9* pD3D9;
						hr = pD3DDev9->GetDirect3D(&pD3D9);
						if (SUCCEEDED(hr)) {
							D3DADAPTER_IDENTIFIER9 AdapID9;
							hr = pD3D9->GetAdapterIdentifier(DevPar9.AdapterOrdinal, 0, &AdapID9);
							if (SUCCEEDED(hr)) {
								// copy adapter description
								m_nPCIVendor					= AdapID9.VendorId;
								m_nPCIDevice					= AdapID9.DeviceId;
								m_VideoDriverVersion.QuadPart	= AdapID9.DriverVersion.QuadPart;
								m_strDeviceDescription			= AdapID9.Description;
								m_strDeviceDescription.AppendFormat(_T(" (%04hX:%04hX)"), m_nPCIVendor, m_nPCIDevice);
							}
						}
						pD3D9->Release();
					}
					pD3DDev9->Release();
					pDevMan9->UnlockDevice(hDevice, FALSE);
				}
				pDevMan9->CloseDeviceHandle(hDevice);
			}
			pDevMan9->Release();
		}
		pGetService->Release();
	}
	return hr;
}

// IFFmpegDecFilter
STDMETHODIMP CMPCVideoDecFilter::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, _T("Software\\MPC-BE Filters\\MPC Video Decoder"))) {
		key.SetDWORDValue(_T("ThreadNumber"), m_nThreadNumber);
		key.SetDWORDValue(_T("DiscardMode"), m_nDiscardMode);
		key.SetDWORDValue(_T("Deinterlacing"), (int)m_nDeinterlacing);
		key.SetDWORDValue(_T("ActiveCodecs"), m_nActiveCodecs);
		key.SetDWORDValue(_T("ARMode"), m_nARMode);
		key.SetDWORDValue(_T("DXVACheckCompatibility"), m_nDXVACheckCompatibility);
		key.SetDWORDValue(_T("DisableDXVA_SD"), m_nDXVA_SD);

		// === New swscaler options
		if (m_nSwRefresh>0) {
			key.SetDWORDValue(_T("SwChromaToRGB"), m_nSwChromaToRGB);
			key.SetDWORDValue(_T("SwResizeMethodBE"), m_nSwResizeMethodBE);
			key.SetDWORDValue(_T("SwColorspace"), m_nSwColorspace);
			key.SetDWORDValue(_T("SwInputLevels"), m_nSwInputLevels);
			key.SetDWORDValue(_T("SwOutputLevels"), m_nSwOutputLevels);
		}
		if (m_nSwRefresh>1) {
			key.SetDWORDValue(_T("SwOutputFormats"), m_nSwOutputFormats);
		}
		//
	}
#else
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("ThreadNumber"), m_nThreadNumber);
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("DiscardMode"), m_nDiscardMode);
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("Deinterlacing"), (int)m_nDeinterlacing);
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("ARMode"), m_nARMode);
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("DXVACheckCompatibility"), m_nDXVACheckCompatibility);
	AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("DisableDXVA_SD"), m_nDXVA_SD);

	// === New swscaler options
	if (m_nSwRefresh>0) {
		AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwChromaToRGB"), m_nSwChromaToRGB);
		AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwResizeMethodBE"), m_nSwResizeMethodBE);
		AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwColorspace"), m_nSwColorspace);
		AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwInputLevels"), m_nSwInputLevels);
		AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwOutputLevels"), m_nSwOutputLevels);
	}
	if (m_nSwRefresh>1) {
		AfxGetApp()->WriteProfileInt(_T("Filters\\MPC Video Decoder"), _T("SwOutputFormats"), m_nSwOutputFormats);
	}
	//
#endif

	return S_OK;
}

// === IMPCVideoDecFilter

STDMETHODIMP CMPCVideoDecFilter::SetThreadNumber(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nThreadNumber = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetThreadNumber()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nThreadNumber;
}

STDMETHODIMP CMPCVideoDecFilter::SetDiscardMode(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDiscardMode = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetDiscardMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDiscardMode;
}

STDMETHODIMP CMPCVideoDecFilter::SetDeinterlacing(MPC_DEINTERLACING_FLAGS nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDeinterlacing = nValue;
	return S_OK;
}

STDMETHODIMP_(MPC_DEINTERLACING_FLAGS) CMPCVideoDecFilter::GetDeinterlacing()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDeinterlacing;
}

STDMETHODIMP_(GUID*) CMPCVideoDecFilter::GetDXVADecoderGuid()
{
	if (m_pGraph == NULL) {
		return NULL;
	} else {
		return &m_DXVADecoderGUID;
	}
}

STDMETHODIMP CMPCVideoDecFilter::SetActiveCodecs(DWORD nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nActiveCodecs = nValue;
	return S_OK;
}

STDMETHODIMP_(DWORD) CMPCVideoDecFilter::GetActiveCodecs()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nActiveCodecs;
}

STDMETHODIMP_(LPCTSTR) CMPCVideoDecFilter::GetVideoCardDescription()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_strDeviceDescription;
}

STDMETHODIMP CMPCVideoDecFilter::SetARMode(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nARMode = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetARMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nARMode;
}

STDMETHODIMP CMPCVideoDecFilter::SetDXVACheckCompatibility(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDXVACheckCompatibility = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetDXVACheckCompatibility()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDXVACheckCompatibility;
}

STDMETHODIMP CMPCVideoDecFilter::SetDXVA_SD(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDXVA_SD = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetDXVA_SD()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDXVA_SD;
}

// === New swscaler options
STDMETHODIMP CMPCVideoDecFilter::SetSwRefresh(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwRefresh = nValue;
	return S_OK;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwOutputFormats(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwOutputFormats = (int)nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwOutputFormats()
{
	CAutoLock cAutoLock(&m_csProps);
	return (int)m_nSwOutputFormats;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwChromaToRGB(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwChromaToRGB = nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwChromaToRGB()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nSwChromaToRGB;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwResizeMethodBE(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwResizeMethodBE = nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwResizeMethodBE()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nSwResizeMethodBE;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwColorspace(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwColorspace = nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwColorspace()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nSwColorspace;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwInputLevels(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwInputLevels = nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwInputLevels()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nSwInputLevels;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwOutputLevels(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwOutputLevels = nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwOutputLevels()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nSwOutputLevels;
}

STDMETHODIMP CMPCVideoDecFilter::SetDialogHWND(HWND nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDialogHWND = nValue;
	return S_OK;
}

STDMETHODIMP_(unsigned __int64) CMPCVideoDecFilter::GetOutputFormat()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nOutCsp;
}

// === IMPCVideoDecFilter2
STDMETHODIMP_(int) CMPCVideoDecFilter::GetFrameType()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nFrameType;
}

// === IMPCVideoDecFilterCodec
STDMETHODIMP CMPCVideoDecFilter::SetFFMpegCodec(int nCodec, bool bEnabled)
{
	CAutoLock cAutoLock(&m_csProps);

	if (nCodec < 0 || nCodec >= FFM_LAST) {
		return E_FAIL;
	}

	m_FFmpegFilters[nCodec] = bEnabled;
	return S_OK;
}

STDMETHODIMP CMPCVideoDecFilter::SetDXVACodec(int nCodec, bool bEnabled)
{
	CAutoLock cAutoLock(&m_csProps);

	if (nCodec < 0 || nCodec >= TRA_DXVA_LAST) {
		return E_FAIL;
	}

	m_DXVAFilters[nCodec] = bEnabled;
	return S_OK;
}
