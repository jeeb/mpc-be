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
#include "DXVADecoder.h"

// Also define in ffmpeg!
#define MAX_SLICES 16
#define FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG 1 ///< Work around for DXVA2 and old UVD/UVD+ ATI video cards
#define FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO    2 ///< Work around for DXVA2 and old Intel GPUs with ClearVideo interface

class CDXVADecoderH264 : public CDXVADecoder
{
public:
	CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderH264();

	virtual void			Flush();
	virtual void			CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual HRESULT			DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);

private:
	struct DXVA_H264_Context {
		DXVA_PicParams_H264		DXVAPicParams;
		DXVA_Qmatrix_H264		DXVAScalingMatrix;
		unsigned				slice_count;
		DXVA_Slice_H264_Short	SliceShort[MAX_SLICES];
		DXVA_Slice_H264_Long	SliceLong[MAX_SLICES];
		const uint8_t			*bitstream;
		unsigned				bitstream_size;
	};
	struct DXVA_Context {
		uint64_t				workaround;
		int						longSlice;
		DXVA_H264_Context		DXVA_H264Context[2];	
	} m_DXVA_Context;

	bool					m_bUseLongSlice;

	UINT					m_nFieldNum;
	UINT					StatusReportFeedbackNumber;
	USHORT					Reserved16Bits;

	void					Init();
};
