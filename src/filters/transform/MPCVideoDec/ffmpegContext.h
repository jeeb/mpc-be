/*
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

#pragma once

#include <dxva.h>

struct AVCodecContext;
struct AVFrame;

enum PCI_Vendors {
	PCIV_ATI			= 0x1002,
	PCIV_nVidia			= 0x10DE,
	PCIV_Intel			= 0x8086,
	PCIV_S3_Graphics	= 0x5333
};

#define PCID_Intel_HD2500    0x0152
#define PCID_Intel_HD4000    0x0162

// Bitmasks for DXVA compatibility check
#define DXVA_UNSUPPORTED_LEVEL			1
#define DXVA_TOO_MANY_REF_FRAMES		DXVA_UNSUPPORTED_LEVEL << 1
#define DXVA_INCOMPATIBLE_SAR			DXVA_UNSUPPORTED_LEVEL << 2
#define DXVA_PROFILE_HIGHER_THAN_HIGH	DXVA_UNSUPPORTED_LEVEL << 3
#define DXVA_HIGH_BIT					DXVA_UNSUPPORTED_LEVEL << 4

bool			IsATIUVD(DWORD nPCIVendor, DWORD nPCIDevice);

// === H264 functions
HRESULT			FFH264DecodeFrame(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, BYTE* pBuffer, UINT nSize, REFERENCE_TIME rtStart,
								  int* pFramePOC, int* pOutPOC, REFERENCE_TIME* pOutrtStart,
								  UINT* SecondFieldOffset, int* Sync, int* NALLength);
HRESULT			FFH264BuildPicParams(struct AVCodecContext* pAVCtx, DWORD nPCIVendor, DWORD nPCIDevice,
									 DXVA_PicParams_H264* pDXVAPicParams, DXVA_Qmatrix_H264* pDXVAScalingMatrix,
									 int* nPictStruct);
int				FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx,
										 DWORD nPCIVendor, DWORD nPCIDevice, LARGE_INTEGER VideoDriverVersion, bool nIsAtiDXVACompatible);
void			FFH264SetCurrentPicture(int nIndex, DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
void			FFH264UpdateRefFramesList(DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
BOOL			FFH264IsRefFrameInUse(int nFrameNum, struct AVCodecContext* pAVCtx);
void			FF264UpdateRefFrameSliceLong(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Slice_H264_Long* pSlice, struct AVCodecContext* pAVCtx);
void			FFH264SetDxvaSliceLong(struct AVCodecContext* pAVCtx, void* pSliceLong);

// === VC1 functions
HRESULT			FFVC1DecodeFrame(DXVA_PictureParameters* pPicParams, struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, REFERENCE_TIME rtStart,
								 BYTE* pBuffer, UINT nSize,
								 BOOL* b_repeat_pict, UINT* nFrameSize, BOOL b_SecondField);

// === Mpeg2 functions
int				MPEG2CheckCompatibility(struct AVCodecContext* pAVCtx);
HRESULT			FFMpeg2DecodeFrame(DXVA_PictureParameters* pPicParams, DXVA_QmatrixData* m_QMatrixData, DXVA_SliceInfo* pSliceInfo,
									struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, BYTE* pBuffer, UINT nSize,
									int* nSliceCount, int* nNextCodecIndex, bool* bIsField);

// === Common functions
int				FFGetCodedPicture(struct AVCodecContext* pAVCtx);
BOOL			FFGetAlternateScan(struct AVCodecContext* pAVCtx);
void			FFGetFrameProps(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, int& width, int& height);
BOOL			DXVACheckFramesize(int width, int height, DWORD nPCIVendor, DWORD nPCIDevice);
