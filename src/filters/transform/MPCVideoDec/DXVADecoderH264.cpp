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

#include "stdafx.h"
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"
#include "DXVADecoderH264.h"
#include "MPCVideoDec.h"
#include "DXVAAllocator.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVADecoderH264::CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	m_bUseLongSlice = (GetDXVA1Config()->bConfigBitstreamRaw != 2);
	Init();
}

CDXVADecoderH264::CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	m_bUseLongSlice = (GetDXVA2Config()->ConfigBitstreamRaw != 2);
	Init();
}

CDXVADecoderH264::~CDXVADecoderH264()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264::Destroy()"));
}

void CDXVADecoderH264::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264::Init()"));

	memset(&m_DXVAPicParams, 0, sizeof(m_DXVAPicParams));
	memset(&m_pSliceLong, 0, sizeof(m_pSliceLong));
	memset(&m_pSliceShort, 0, sizeof(m_pSliceShort));

	Reserved16Bits		= 3;
	if (m_pFilter->GetPCIVendor() == PCIV_Intel && m_guidDecoder == DXVA_Intel_H264_ClearVideo) {
		Reserved16Bits	= 0x534c;
	} else if (IsATIUVD(m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice())) {
		Reserved16Bits	= 0;
	}


	m_nNALLength	= 4;
	m_nMaxSlices	= 0;
	m_nSlices		= 0;

	switch (GetMode()) {
		case H264_VLD :
			AllocExecuteParams (4);
			break;
		default :
			ASSERT(FALSE);
	}

	FFH264SetDxvaParams(m_pFilter->GetAVCtx(), m_pSliceLong, m_DXVAPicParams);

	Flush();
}

void CDXVADecoderH264::Flush()
{
	StatusReportFeedbackNumber = 1;

	__super::Flush();
}

void CDXVADecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	CH264Nalu	Nalu;
	UINT		m_nSize = nSize;
	int			nDxvaNalLength;

	m_nSlices = 0;

	Nalu.SetBuffer(pBuffer, m_nSize, m_nNALLength);
	nSize = 0;
	while (Nalu.ReadNext()) {
		switch (Nalu.GetType()) {
			case NALU_TYPE_SLICE:
			case NALU_TYPE_IDR:
				// Skip the NALU if the data length is below 0
				if ((int)Nalu.GetDataLength() < 0) {
					break;
				}

				// For AVC1, put startcode 0x000001
				pDXVABuffer[0] = pDXVABuffer[1] = 0; pDXVABuffer[2] = 1;

				// Copy NALU
				memcpy_sse(pDXVABuffer + 3, Nalu.GetDataBuffer(), Nalu.GetDataLength());

				// Update slice control buffer
				nDxvaNalLength									= Nalu.GetDataLength() + 3;
				m_pSliceShort[m_nSlices].BSNALunitDataLocation	= nSize;
				m_pSliceShort[m_nSlices].SliceBytesInBuffer		= nDxvaNalLength;

				nSize											+= nDxvaNalLength;
				pDXVABuffer										+= nDxvaNalLength;
				m_nSlices++;
				break;
		}
	}

	// Complete bitstream buffer with zero padding (buffer size should be a multiple of 128)
	if (nSize % 128) {
		int nDummy = 128 - (nSize % 128);

		memset(pDXVABuffer, 0, nDummy);
		m_pSliceShort[m_nSlices - 1].SliceBytesInBuffer	+= nDummy;
		nSize											+= nDummy;		
	}
}

HRESULT CDXVADecoderH264::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT						hr					= S_FALSE;
	UINT						nSlices				= 0;
	UINT						nNalOffset			= 0;
	UINT						SecondFieldOffset	= 0;
	UINT						nSize_Result		= 0;
	int							Sync				= 0;
	int							got_picture			= 0;
	CH264Nalu					Nalu;

	CHECK_HR_FALSE (FFH264DecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), pDataIn, nSize, rtStart, 
					&SecondFieldOffset, &Sync, &m_nNALLength, &got_picture));

	if (m_nSurfaceIndex == -1) {
		return S_FALSE;
	}

	Nalu.SetBuffer(pDataIn, nSize, m_nNALLength);
	while (Nalu.ReadNext()) {
		switch (Nalu.GetType()) {
			case NALU_TYPE_SLICE:
			case NALU_TYPE_IDR:
				if (m_bUseLongSlice) {
					m_pSliceLong[nSlices].BSNALunitDataLocation	= nNalOffset;
					m_pSliceLong[nSlices].SliceBytesInBuffer	= Nalu.GetDataLength() + 3;
					m_pSliceLong[nSlices].slice_id				= nSlices;
					FF264UpdateRefFrameSliceLong(&m_DXVAPicParams[0], &m_pSliceLong[nSlices], m_pFilter->GetAVCtx());

					if (nSlices) {
						m_pSliceLong[nSlices - 1].NumMbsForSlice = m_pSliceLong[nSlices].NumMbsForSlice = m_pSliceLong[nSlices].first_mb_in_slice - m_pSliceLong[nSlices - 1].first_mb_in_slice;
					}
				}
				nSlices++;
				nNalOffset += (UINT)(Nalu.GetDataLength() + 3);
				if (nSlices > MAX_SLICES) {
					break;
				}
				break;
		}
	}

	if (!nSlices) {
		return S_FALSE;
	}

	{
		CHECK_HR_FALSE (FFH264UpdatePicParams(m_pFilter->GetAVCtx(), m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice(), &m_DXVAPicParams[0], &m_DXVAScalingMatrix));
		m_DXVAPicParams[0].Reserved16Bits				= Reserved16Bits;
		m_DXVAPicParams[0].StatusReportFeedbackNumber	= StatusReportFeedbackNumber++;
		
		// Begin frame
		CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
		// Add picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_H264), &m_DXVAPicParams[0]));
		// Add quantization matrix
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), &m_DXVAScalingMatrix));
		// Add bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, SecondFieldOffset ? SecondFieldOffset : nSize, pDataIn, &nSize_Result));
		// Add slice control
		if (m_bUseLongSlice) {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Long) * m_nSlices, m_pSliceLong));
		} else {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * m_nSlices, m_pSliceShort));
		}
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));

		if (SecondFieldOffset) {
			// Decoding Second Field

			CHECK_HR_FALSE (FFH264UpdatePicParams(m_pFilter->GetAVCtx(), m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice(), &m_DXVAPicParams[1], &m_DXVAScalingMatrix));
			m_DXVAPicParams[1].Reserved16Bits				= Reserved16Bits;
			m_DXVAPicParams[1].StatusReportFeedbackNumber	= StatusReportFeedbackNumber++;
			
			// Begin frame
			CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
			// Add picture parameters
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_H264), &m_DXVAPicParams[1]));
			// Add quantization matrix
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), &m_DXVAScalingMatrix));
			// Add bitstream
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize - SecondFieldOffset, pDataIn + SecondFieldOffset, &nSize_Result));
			// Add slice control
			if (m_bUseLongSlice) {
				CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Long) * m_nSlices, m_pSliceLong));
			} else {
				CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * m_nSlices, m_pSliceShort));
			}
			// Decode frame
			CHECK_HR_FRAME (Execute());
			CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));
		}

	}

	if (got_picture) {
		AddToStore(m_nSurfaceIndex, m_pSampleToDeliver, rtStart, rtStop);
		hr = DisplayNextFrame();
	}

	return hr;
}
