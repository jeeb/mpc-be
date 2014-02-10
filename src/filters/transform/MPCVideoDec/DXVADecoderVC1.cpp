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
#include "DXVADecoderVC1.h"
#include "MPCVideoDec.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

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
}

void CDXVADecoderVC1::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderVC1::Init()"));

	memset(&m_PictureParams, 0, sizeof(m_PictureParams));
	memset(&m_SliceInfo, 0, sizeof(m_SliceInfo));

	m_PictureParams.bMacroblockWidthMinus1			= 15;
	m_PictureParams.bMacroblockHeightMinus1			= 15;
	m_PictureParams.bBlockWidthMinus1				= 7;
	m_PictureParams.bBlockHeightMinus1				= 7;
	m_PictureParams.bBPPminus1						= 7;
	m_PictureParams.bChromaFormat					= 1;

	// iWMV9 - i9IRU - iOHIT - iINSO - iWMVA - 0 - 0 - 0			| Section 3.2.5
	m_PictureParams.bBidirectionalAveragingMode		= (1 << 7) |
						(GetConfigIntraResidUnsigned()   << 6) |	// i9IRU
						(GetConfigResidDiffAccelerator() << 5);		// iOHIT

	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;

	switch (GetMode()) {
		case VC1_VLD :
			AllocExecuteParams (3);
			break;
		default :
			ASSERT(FALSE);
	}

	Flush();
}

// === Public functions
HRESULT CDXVADecoderVC1::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT						hr				= S_FALSE;
	UINT						nFrameSize		= 0;
	UINT						nSize_Result	= 0;
	int							got_picture		= 0;

	CHECK_HR_FALSE (FFVC1DecodeFrame(&m_PictureParams, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), rtStart, 
									 pDataIn, nSize, 
									 &nFrameSize, FALSE, &got_picture));

	if (m_nSurfaceIndex == -1) {
		return S_FALSE;
	}

	if (m_bWaitingForKeyFrame && got_picture) {
		if (m_pFilter->GetFrame()->key_frame) {
			m_bWaitingForKeyFrame = FALSE;
		} else {
			got_picture = 0;
		}
	}

	BYTE bPicBackwardPrediction = m_PictureParams.bPicBackwardPrediction;
	
	CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));

	m_PictureParams.wDecodedPictureIndex	= m_nSurfaceIndex;
	m_PictureParams.wDeblockedPictureIndex	= m_PictureParams.wDecodedPictureIndex;

	// Manage reference picture list
	if (!m_PictureParams.bPicBackwardPrediction) {
		if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
			FreePictureSlot(m_wRefPictureIndex[0]);
		}
		m_wRefPictureIndex[0] = m_wRefPictureIndex[1];
		m_wRefPictureIndex[1] = m_nSurfaceIndex;
	}
	m_PictureParams.wForwardRefPictureIndex		= (m_PictureParams.bPicIntra == 0)				? m_wRefPictureIndex[0] : NO_REF_FRAME;
	m_PictureParams.wBackwardRefPictureIndex	= (m_PictureParams.bPicBackwardPrediction == 1) ? m_wRefPictureIndex[1] : NO_REF_FRAME;

	m_PictureParams.bPic4MVallowed				= (m_PictureParams.wBackwardRefPictureIndex == NO_REF_FRAME && m_PictureParams.bPicStructure == 3) ? 1 : 0;
	m_PictureParams.bPicDeblockConfined		   |= (m_PictureParams.wBackwardRefPictureIndex == NO_REF_FRAME) ? 0x04 : 0;

	m_PictureParams.bPicScanMethod++;			// Use for status reporting sections 3.8.1 and 3.8.2

	// Send picture params to accelerator
	CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));

	// Send bitstream to accelerator
	CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nFrameSize ? nFrameSize : nSize, pDataIn, &nSize_Result));

	m_SliceInfo.wQuantizerScaleCode	= 1;		// TODO : 1->31 ???
	m_SliceInfo.dwSliceBitsInBuffer	= nSize_Result * 8;
	CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(m_SliceInfo), &m_SliceInfo));

	// Decode frame
	CHECK_HR_FRAME (Execute());
	CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));

	// ***************
	if (nFrameSize) { // Decoding Second Field
		FFVC1DecodeFrame(&m_PictureParams, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), rtStart,
						 pDataIn, nSize,
						 NULL, TRUE, NULL);

		CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));

		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));

		// Send bitstream to accelerator
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize - nFrameSize, pDataIn + nFrameSize, &nSize_Result));

		m_SliceInfo.wQuantizerScaleCode	= 1;		// TODO : 1->31 ???
		m_SliceInfo.dwSliceBitsInBuffer	= nSize_Result * 8;
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(m_SliceInfo), &m_SliceInfo));

		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));
	}
	// ***************

	if (got_picture) {
		AddToStore(m_nSurfaceIndex, m_pSampleToDeliver, rtStart, rtStop);
		hr = DisplayNextFrame();
	}

	return hr;
}

static BYTE* FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize)
{
	BYTE*	pStart	= pBuffer;
	BYTE	bCode	= 0;
	for (UINT i = 0; i < nSize - 4; i++) {
		if (((*((DWORD*)(pBuffer + i)) & 0x00FFFFFF) == 0x00010000) || (i >= nSize - 5)) {
			if (bCode == 0) {
				bCode = pBuffer[i + 3];
				if ((nSize == 5) && (bCode == 0x0D)) {
					nPacketSize = nSize;
					return pBuffer;
				}
			} else {
				if (bCode == 0x0D) {
					// Start code found!
					nPacketSize = i - (pStart - pBuffer) + (i >= nSize - 5 ? 5 : 1);
					return pStart;
				} else {
					// Other stuff, ignore it
					pStart	= pBuffer + i;
					bCode	= pBuffer[i + 3];
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
		memcpy_sse(pDXVABuffer, (BYTE*)pBuffer, nSize);
	} else {
		if ((*((DWORD*)pBuffer) & 0x00FFFFFF) != 0x00010000) {
			if (m_pFilter->GetCodec() == AV_CODEC_ID_WMV3) {
				memcpy_sse(pDXVABuffer, (BYTE*)pBuffer, nSize);
			} else {
				pDXVABuffer[0] = pDXVABuffer[1] = 0;
				pDXVABuffer[2] = 1;
				pDXVABuffer[3] = 0x0D;
				memcpy_sse(pDXVABuffer + 4, (BYTE*)pBuffer, nSize);
				nSize += 4;
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
	m_wRefPictureIndex[0]	= NO_REF_FRAME;
	m_wRefPictureIndex[1]	= NO_REF_FRAME;

	__super::Flush();
}
