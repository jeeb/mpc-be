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

#include "stdafx.h"
#include "DXVADecoderVC1.h"
#include "MPCVideoDec.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

#if 0
	#define TRACE_VC1 TRACE
#else
	#define TRACE_VC1(...)
#endif

CDXVADecoderVC1::CDXVADecoderVC1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderVC1::CDXVADecoderVC1(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	Init();
}

CDXVADecoderVC1::~CDXVADecoderVC1(void)
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderVC1::Destroy()"));
	Flush();
}

void CDXVADecoderVC1::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderVC1::Init()"));

	memset (&m_PictureParams, 0, sizeof(m_PictureParams));
	memset (&m_SliceInfo,     0, sizeof(m_SliceInfo));

	m_PictureParams.bMacroblockWidthMinus1			= 15;
	m_PictureParams.bMacroblockHeightMinus1			= 15;
	m_PictureParams.bBlockWidthMinus1				= 7;
	m_PictureParams.bBlockHeightMinus1				= 7;
	m_PictureParams.bBPPminus1						= 7;

	m_PictureParams.bChromaFormat					= VC1_CHROMA_420;

	// iWMV9 - i9IRU - iOHIT - iINSO - iWMVA - 0 - 0 - 0			| Section 3.2.5
	m_PictureParams.bBidirectionalAveragingMode		= (1 << 7) |
						(GetConfigIntraResidUnsigned()   << 6) |	// i9IRU
						(GetConfigResidDiffAccelerator() << 5);		// iOHIT

	m_nMaxWaiting		  = 5;
	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;

	switch (GetMode()) {
		case VC1_VLD :
			AllocExecuteParams (3);
			break;
		default :
			ASSERT(FALSE);
	}

	m_bFrame_repeat_pict = FALSE;

	Flush();
}

// === Public functions
HRESULT CDXVADecoderVC1::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT						hr;
	int							nSurfaceIndex;
	CComPtr<IMediaSample>		pSampleToDeliver;
	UINT						nFrameSize, nSize_Result;

	CHECK_HR_FALSE (FFVC1DecodeFrame(&m_PictureParams, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), rtStart, 
									 pDataIn, nSize, 
									 &m_bFrame_repeat_pict, &nFrameSize, FALSE));

	// Wait I frame after a flush
	if (m_bFlushed && ! m_PictureParams.bPicIntra) {
		return S_FALSE;
	}

	BYTE bPicBackwardPrediction = m_PictureParams.bPicBackwardPrediction;

	CHECK_HR (GetFreeSurfaceIndex(nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));

	CHECK_HR (BeginFrame(nSurfaceIndex, pSampleToDeliver));

	TRACE_VC1 ("CDXVADecoderVC1::DecodeFrame() : PictureType = %d, rtStart = %I64d, Surf = %d\n", nSliceType, rtStart, nSurfaceIndex);

	m_PictureParams.wDecodedPictureIndex	= nSurfaceIndex;
	m_PictureParams.wDeblockedPictureIndex	= m_PictureParams.wDecodedPictureIndex;

	// Manage reference picture list
	if (!m_PictureParams.bPicBackwardPrediction) {
		if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
			RemoveRefFrame(m_wRefPictureIndex[0]);
		}
		m_wRefPictureIndex[0] = m_wRefPictureIndex[1];
		m_wRefPictureIndex[1] = nSurfaceIndex;
	}
	m_PictureParams.wForwardRefPictureIndex		= (m_PictureParams.bPicIntra == 0)				? m_wRefPictureIndex[0] : NO_REF_FRAME;
	m_PictureParams.wBackwardRefPictureIndex	= (m_PictureParams.bPicBackwardPrediction == 1) ? m_wRefPictureIndex[1] : NO_REF_FRAME;

	m_PictureParams.bPic4MVallowed				= (m_PictureParams.wBackwardRefPictureIndex == NO_REF_FRAME && m_PictureParams.bPicStructure == 3) ? 1 : 0;
	m_PictureParams.bPicDeblockConfined		   |= (m_PictureParams.wBackwardRefPictureIndex == NO_REF_FRAME) ? 0x04 : 0;

	m_PictureParams.bPicScanMethod++;			// Use for status reporting sections 3.8.1 and 3.8.2

	TRACE_VC1 ("CDXVADecoderVC1::DecodeFrame() : Decode frame %i\n", m_PictureParams.bPicScanMethod);

	// Send picture params to accelerator
	CHECK_HR (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));

	// Send bitstream to accelerator
	CHECK_HR (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nFrameSize ? nFrameSize : nSize, pDataIn, &nSize_Result));

	m_SliceInfo.wQuantizerScaleCode	= 1;		// TODO : 1->31 ???
	m_SliceInfo.dwSliceBitsInBuffer	= nSize_Result * 8;
	CHECK_HR (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(m_SliceInfo), &m_SliceInfo));

	// Decode frame
	CHECK_HR (Execute());
	CHECK_HR (EndFrame(nSurfaceIndex));

	// ***************
	if (nFrameSize) { // Decoding Second Field
		FFVC1DecodeFrame(&m_PictureParams, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), rtStart,
						 pDataIn, nSize,
						 &m_bFrame_repeat_pict, NULL, TRUE);

		CHECK_HR (BeginFrame(nSurfaceIndex, pSampleToDeliver));

		TRACE_VC1 ("CDXVADecoderVC1::DecodeFrame() : PictureType = %d\n", nSliceType);

		CHECK_HR (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));

		// Send bitstream to accelerator
		CHECK_HR (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize - nFrameSize, pDataIn + nFrameSize, &nSize_Result));

		m_SliceInfo.wQuantizerScaleCode	= 1;		// TODO : 1->31 ???
		m_SliceInfo.dwSliceBitsInBuffer	= nSize_Result * 8;
		CHECK_HR (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(m_SliceInfo), &m_SliceInfo));

		// Decode frame
		CHECK_HR (Execute());
		CHECK_HR (EndFrame(nSurfaceIndex));
	}
	// ***************

#ifdef _DEBUG
	DisplayStatus();
#endif

	// Update timestamp & Re-order B frames
	m_pFilter->UpdateFrameTime(rtStart, rtStop, !!m_bFrame_repeat_pict);

	if (m_pFilter->IsReorderBFrame() || m_pFilter->IsEvo()) {
		if (bPicBackwardPrediction == 1) {
			std::swap(rtStart, m_rtStartDelayed);
			std::swap(rtStop, m_rtStopDelayed);
		} else {
			// Save I or P reference time (swap later)
			if (!m_bFlushed) {
				if (m_nDelayedSurfaceIndex != -1) {
					UpdateStore(m_nDelayedSurfaceIndex, m_rtStartDelayed, m_rtStopDelayed);
				}
				m_rtStartDelayed = m_rtStopDelayed = _I64_MAX;
				std::swap(rtStart, m_rtStartDelayed);
				std::swap(rtStop, m_rtStopDelayed);
				m_nDelayedSurfaceIndex = nSurfaceIndex;
			}
		}
	}

	AddToStore(nSurfaceIndex, pSampleToDeliver, (bPicBackwardPrediction != 1), rtStart, rtStop, false, 0);

	m_bFlushed = false;

	return DisplayNextFrame();
}

BYTE* CDXVADecoderVC1::FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize)
{
	BYTE*	pStart	= pBuffer;
	BYTE	bCode	= 0;
	for (UINT i = 0; i < nSize-4; i++) {
		if (((*((DWORD*)(pBuffer+i)) & 0x00FFFFFF) == 0x00010000) || (i >= nSize-5)) {
			if (bCode == 0) {
				bCode = pBuffer[i+3];
				if ((nSize == 5) && (bCode == 0x0D)) {
					nPacketSize = nSize;
					return pBuffer;
				}
			} else {
				if (bCode == 0x0D) {
					// Start code found!
					nPacketSize = i - (pStart - pBuffer) + (i >= nSize-5 ? 5 : 1);
					return pStart;
				} else {
					// Other stuff, ignore it
					pStart	= pBuffer + i;
					bCode	= pBuffer[i+3];
				}
			}
		}
	}

	ASSERT(FALSE);		// Should never happen!

	return NULL;
}

void CDXVADecoderVC1::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	if (m_PictureParams.bSecondField) {
		memcpy_sse (pDXVABuffer, (BYTE*)pBuffer, nSize);
	} else {
		if ((*((DWORD*)pBuffer) & 0x00FFFFFF) != 0x00010000) {
			if (m_pFilter->GetCodec() == AV_CODEC_ID_WMV3) {
				memcpy_sse(pDXVABuffer, (BYTE*)pBuffer, nSize);
			} else {
				pDXVABuffer[0] = pDXVABuffer[1] = 0;
				pDXVABuffer[2] = 1;
				pDXVABuffer[3] = 0x0D;
				memcpy_sse(pDXVABuffer + 4, (BYTE*)pBuffer, nSize);
				nSize +=4;
			}
		} else {
			BYTE*	pStart;
			UINT	nPacketSize;

			pStart = FindNextStartCode(pBuffer, nSize, nPacketSize);
			if (pStart) {
				// Startcode already present
				memcpy_sse(pDXVABuffer, (BYTE*)pStart, nPacketSize);
				nSize = nPacketSize;
			}
		}
	}

	// Complete bitstream buffer with zero padding (buffer size should be a multiple of 128)
	if (nSize % 128) {
		int nDummy = 128 - (nSize % 128);
		
		pDXVABuffer += nSize;
		memset(pDXVABuffer, 0, nDummy);
		nSize += nDummy;
	}
}

void CDXVADecoderVC1::Flush()
{
	m_nDelayedSurfaceIndex	= -1;
	m_rtStartDelayed		= _I64_MAX;
	m_rtStopDelayed			= _I64_MAX;

	if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
		RemoveRefFrame (m_wRefPictureIndex[0]);
	}
	if (m_wRefPictureIndex[1] != NO_REF_FRAME) {
		RemoveRefFrame (m_wRefPictureIndex[1]);
	}

	m_wRefPictureIndex[0]	= NO_REF_FRAME;
	m_wRefPictureIndex[1]	= NO_REF_FRAME;

	__super::Flush();
}

HRESULT CDXVADecoderVC1::DisplayStatus()
{
	HRESULT			hr = E_INVALIDARG;
	DXVA_Status_VC1 Status;

	memset (&Status, 0, sizeof(Status));

	if (SUCCEEDED (hr = CDXVADecoder::QueryStatus(&Status, sizeof(Status)))) {
		Status.StatusReportFeedbackNumber = 0x00FF & Status.StatusReportFeedbackNumber;

		TRACE_VC1 ("CDXVADecoderVC1::DisplayStatus() : Status for the frame %u : bBufType = %u, bStatus = %u, wNumMbsAffected = %u\n",
				   Status.StatusReportFeedbackNumber,
				   Status.bBufType,
				   Status.bStatus,
				   Status.wNumMbsAffected);
	}

	return hr;
}
