/*
 * (C) 2010-2014 see Authors.txt
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

enum {
	SOURCE_FILTER,
	DECODER,
	DXVA_DECODER,
	FFMPEG_DECODER,
	FILTER_TYPE_NB
};

enum SOURCE_FILTER {
	SRC_CDDA,
	SRC_CDXA,
	SRC_VTS,
	SRC_FLIC,
	SRC_DTS,
	SRC_DTSAC3,
	SRC_MATROSKA,
	SRC_SHOUTCAST,
	SRC_REALMEDIA,
	SRC_ROQ,
	SRC_AVI,
	SRC_OGG,
	SRC_MPEG,
	SRC_MPA,
	SRC_DSM,
	SRC_SUBS,
	SRC_MP4,
	SRC_FLV,
	SRC_FLAC,
	SRC_WPAC,
	SRC_MPAC,
	SRC_AMR,
	SRC_TTA,
	SRC_UDP,
	SRC_APE_TAK,
	SRC_RAWVIDEO,

	SRC_LAST
};

enum DECODER {
	TRA_MPEG1,
	TRA_MPEG2,
	TRA_RA,
	TRA_DTS,
	TRA_LPCM,
	TRA_AC3,
	TRA_PS2AUD,
	TRA_PCM,

	TRA_LAST
};

enum DXVA_DECODER {
	TRA_DXVA_H264,
	TRA_DXVA_VC1,
	TRA_DXVA_MPEG2,
	TRA_DXVA_WMV3,

	TRA_DXVA_LAST
};

enum FFMPEG_DECODER {
	FFM_AAC,
	FFM_MPA,
	FFM_VORBIS,
	FFM_NELLY,
	FFM_ALAC,
	FFM_ALS,
	FFM_AMR,
	FFM_H264,
	FFM_VC1,
	FFM_FLAC,
	FFM_FLV4,
	FFM_VP356,
	FFM_VP8,
	FFM_XVID,
	FFM_DIVX,
	FFM_MSMPEG4,
	FFM_WMV,
	FFM_SVQ3,
	FFM_H263,
	FFM_DIRAC,
	FFM_DV,
	FFM_THEORA,
	FFM_AMVV,
	FFM_MJPEG,
	FFM_INDEO,
	FFM_UTVD,
	FFM_SCREC,
	FFM_WPAC,
	FFM_LOSSLESS,
	FFM_MPAC,
	FFM_QDM2,
	FFM_APE,
	FFM_TAK,
	FFM_TTA,
	FFM_PRORES,
	FFM_TRUESPEECH,
	FFM_WMAPRO,
	FFM_WMALOSS,
	FFM_WMA2,
	FFM_BINKV,
	FFM_BINKA,
	FFM_IAC,
	FFM_PNG,
	FFM_WMAVOICE,
	FFM_CLLC,
	FFM_OPUS,
	FFM_SPEEX,
	FFM_RV,
	FFM_UNCOMPRESSED,
	FFM_MPEG2,
	FFM_MPEG1,
	FFM_HEVC,
	FFM_DNXHD,
	FFM_VOXWARE,
	FFM_LAST
};
