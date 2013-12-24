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
#include "DXVADecoderMpeg2.h"
#include "MPCVideoDec.h"
#include "FfmpegContext.h"

#pragma warning(disable: 4005)
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
}
#pragma warning(default: 4005)

#if 0
	#define TRACE_MPEG2 TRACE
#else
	#define TRACE_MPEG2(...)
#endif

CDXVADecoderMpeg2::CDXVADecoderMpeg2(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderMpeg2::CDXVADecoderMpeg2(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	Init();
}

CDXVADecoderMpeg2::~CDXVADecoderMpeg2(void)
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderMpeg2::Destroy()"));
	NewSegment();
}

void CDXVADecoderMpeg2::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderMpeg2::Init()"));

	memset (&m_PictureParams,	0, sizeof(m_PictureParams));
	memset (&m_SliceInfo,		0, sizeof(m_SliceInfo));
	memset (&m_QMatrixData,		0, sizeof(m_QMatrixData));

	m_PictureParams.bMacroblockWidthMinus1			= 15;	// This is equal to "15" for MPEG-1, MPEG-2, H.263, and MPEG-4
	m_PictureParams.bMacroblockHeightMinus1			= 15;	// This is equal to "15" for MPEG-1, MPEG-2, H.261, H.263, and MPEG-4
	m_PictureParams.bBlockWidthMinus1				= 7;	// This is equal to "7" for MPEG-1, MPEG-2, H.261, H.263, and MPEG-4
	m_PictureParams.bBlockHeightMinus1				= 7;	// This is equal to "7" for MPEG-1, MPEG-2, H.261, H.263, and MPEG-4
	m_PictureParams.bBPPminus1						= 7;	// It is equal to "7" for MPEG-1, MPEG-2, H.261, and H.263

	m_PictureParams.bChromaFormat					= 0x01;	// For MPEG-1, MPEG-2 "Main Profile," H.261 and H.263 bitstreams, this value shall always be set to "01", indicating "4:2:0" format


	m_nMaxWaiting			= 5;
	m_wRefPictureIndex[0]	= NO_REF_FRAME;
	m_wRefPictureIndex[1]	= NO_REF_FRAME;
	m_nSliceCount			= 0;

	switch (GetMode()) {
		case MPEG2_VLD :
			AllocExecuteParams (4);
			break;
		default :
			ASSERT(FALSE);
	}

	m_pMPEG2Buffer			= NULL;
	m_nMPEG2BufferSize		= 0;

	NewSegment();
}

HRESULT CDXVADecoderMpeg2::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	TRACE_MPEG2 ("CDXVADecoderMpeg2::DecodeFrame() : %d\n", nSize);

	AppendBuffer (pDataIn, nSize, rtStart, rtStop);
	HRESULT hr = S_OK;

	while (FindPicture (max (m_nMPEG2BufferPos-int(nSize)-4, 0), 0x00)) {
		if (m_MPEG2BufferTime[0].nBuffPos != INT_MIN && m_MPEG2BufferTime[0].nBuffPos < m_nMPEG2PicEnd) {
			rtStart = m_MPEG2BufferTime[0].rtStart;
			rtStop  = m_MPEG2BufferTime[0].rtStop;
		} else {
			rtStart = rtStop = INVALID_TIME;
		}

		hr = DecodeFrameInternal(m_pMPEG2Buffer, m_nMPEG2PicEnd, rtStart, rtStop);
		av_frame_unref(m_pFilter->GetFrame());

		ShrinkBuffer();
	}

	return hr;
}

static CString FrameType(bool bIsField, BYTE bSecondField)
{
	CString str;
	if (bIsField) {
		str.Format(_T("Field [%d]"), bSecondField);
	} else {
		str = _T("Frame");
	}

	return str;
}

HRESULT CDXVADecoderMpeg2::DecodeFrameInternal(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT					hr					= S_FALSE;
	int						nSurfaceIndex		= -1;
	CComPtr<IMediaSample>	pSampleToDeliver;
	bool					bIsField			= false;
	int						bFrame_repeat_pict	= 0;

	CHECK_HR_FALSE (FFMpeg2DecodeFrame(&m_PictureParams, &m_QMatrixData, m_SliceInfo,
									   m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), pDataIn, nSize,
									   &m_nSliceCount, &m_nNextCodecIndex,
									   &bIsField, &bFrame_repeat_pict));

	// Wait I frame after a flush
	if (m_bFlushed && (!m_PictureParams.bPicIntra || (bIsField && m_PictureParams.bSecondField))) {
		TRACE_MPEG2 ("CDXVADecoderMpeg2::DecodeFrame() : Flush - wait I frame, %ws\n", FrameType(bIsField, m_PictureParams.bSecondField));
		return S_FALSE;
	}

	CHECK_HR (GetFreeSurfaceIndex(nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));

	if (!bIsField || (bIsField && !m_PictureParams.bSecondField)) {
		UpdatePictureParams(nSurfaceIndex);	
	}

	TRACE_MPEG2 ("CDXVADecoderMpeg2::DecodeFrame() : Surf = %d, PictureType = %d, %ws, m_nNextCodecIndex = %d, rtStart = [%I64d]\n",
				 nSurfaceIndex, nSliceType, FrameType(bIsField, m_PictureParams.bSecondField), m_nNextCodecIndex, rtStart);

	{
		CHECK_HR (BeginFrame(nSurfaceIndex, pSampleToDeliver));
		// Send picture parameters
		CHECK_HR (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));
		// Add quantization matrix
		CHECK_HR (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(m_QMatrixData), &m_QMatrixData));
		// Add slice control
		CHECK_HR (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo) * m_nSliceCount, &m_SliceInfo));
		// Add bitstream
		CHECK_HR (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize));
		// Decode frame
		CHECK_HR (Execute());
		CHECK_HR (EndFrame(nSurfaceIndex));
	}

	bool bAdded = AddToStore(nSurfaceIndex, pSampleToDeliver, (m_PictureParams.bPicBackwardPrediction != 1), rtStart, rtStop,
							 bIsField, FFGetCodedPicture(m_pFilter->GetAVCtx()));

	if (bAdded) {
		hr = DisplayNextFrame();
	}
		
	m_bFlushed = false;
	return hr;
}

void CDXVADecoderMpeg2::UpdatePictureParams(int nSurfaceIndex)
{
	DXVA2_ConfigPictureDecode* cpd = GetDXVA2Config();		// Ok for DXVA1 too (parameters have been copied)

	m_PictureParams.wDecodedPictureIndex = nSurfaceIndex;

	// Manage reference picture list
	if (!m_PictureParams.bPicBackwardPrediction) {
		if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
			RemoveRefFrame (m_wRefPictureIndex[0]);
		}
		m_wRefPictureIndex[0] = m_wRefPictureIndex[1];
		m_wRefPictureIndex[1] = nSurfaceIndex;
	}
	m_PictureParams.wForwardRefPictureIndex		= (m_PictureParams.bPicIntra == 0)				? m_wRefPictureIndex[0] : NO_REF_FRAME;
	m_PictureParams.wBackwardRefPictureIndex	= (m_PictureParams.bPicBackwardPrediction == 1) ? m_wRefPictureIndex[1] : NO_REF_FRAME;

	// Shall be 0 if bConfigResidDiffHost is 0 or if BPP > 8
	if (cpd->ConfigResidDiffHost == 0 || m_PictureParams.bBPPminus1 > 7) {
		m_PictureParams.bPicSpatialResid8 = 0;
	} else {
		if (m_PictureParams.bBPPminus1 == 7 && m_PictureParams.bPicIntra && cpd->ConfigResidDiffHost) {
			// Shall be 1 if BPP is 8 and bPicIntra is 1 and bConfigResidDiffHost is 1
			m_PictureParams.bPicSpatialResid8 = 1;
		} else {
			// Shall be 1 if bConfigSpatialResid8 is 1
			m_PictureParams.bPicSpatialResid8 = cpd->ConfigSpatialResid8;
		}
	}

	// Shall be 0 if bConfigResidDiffHost is 0 or if bConfigSpatialResid8 is 0 or if BPP > 8
	if (cpd->ConfigResidDiffHost == 0 || cpd->ConfigSpatialResid8 == 0 || m_PictureParams.bBPPminus1 > 7) {
		m_PictureParams.bPicOverflowBlocks = 0;
	}

	// Shall be 1 if bConfigHostInverseScan is 1 or if bConfigResidDiffAccelerator is 0.

	if (cpd->ConfigHostInverseScan == 1 || cpd->ConfigResidDiffAccelerator == 0) {
		m_PictureParams.bPicScanFixed	= 1;

		if (cpd->ConfigHostInverseScan != 0) {
			m_PictureParams.bPicScanMethod	= 3;    // 11 = Arbitrary scan with absolute coefficient address.
		} else if (FFGetAlternateScan(m_pFilter->GetAVCtx())) {
			m_PictureParams.bPicScanMethod	= 1;    // 00 = Zig-zag scan (MPEG-2 Figure 7-2)
		} else {
			m_PictureParams.bPicScanMethod	= 0;    // 01 = Alternate-vertical (MPEG-2 Figure 7-3),
		}
	}
}

void CDXVADecoderMpeg2::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	while (*((DWORD*)pBuffer) != 0x01010000) {
		pBuffer++;
		nSize--;

		if (nSize <= 0) {
			return;
		}
	}

	memcpy_sse(pDXVABuffer, pBuffer, nSize);
}

void CDXVADecoderMpeg2::Flush()
{
	m_nNextCodecIndex = INT_MIN;

	m_wRefPictureIndex[0]	= NO_REF_FRAME;
	m_wRefPictureIndex[1]	= NO_REF_FRAME;

	m_rtLastStart			= 0;

	__super::Flush();
}

void CDXVADecoderMpeg2::NewSegment()
{
	Flush();
	ResetBuffer();
}


int CDXVADecoderMpeg2::FindOldestFrame()
{
	int nPos = -1;

	for (int i = 0; i < m_nPicEntryNumber; i++) {
		if (!m_pPictureStore[i].bDisplayed && m_pPictureStore[i].bInUse &&
			(m_pPictureStore[i].nCodecSpecific == m_nNextCodecIndex)) {
			m_nNextCodecIndex	= INT_MIN;
			nPos				= i;
		}
	}

	if (nPos != -1) {
		UpdateFrameTime(m_pPictureStore[nPos].rtStart, m_pPictureStore[nPos].rtStop);
	}

	return nPos;
}

void CDXVADecoderMpeg2::UpdateFrameTime(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	if (m_rtLastStart && (rtStart == INVALID_TIME || (rtStart < m_rtLastStart))) {
		rtStart = m_rtLastStart;
	}

	if (rtStart == INVALID_TIME) {
		rtStart = m_rtLastStart;
	}

	REFERENCE_TIME AvgTimePerFrame = m_pFilter->GetDuration() * (m_pFilter->GetFrame()->repeat_pict ? 3 : 2) / 2;
	
	rtStop			= rtStart + (AvgTimePerFrame / m_pFilter->GetRate());
	m_rtLastStart	= rtStop;
}

bool CDXVADecoderMpeg2::FindPicture(int nIndex, int nStartCode)
{
	DWORD dw = 0;

	CheckPointer(m_pMPEG2Buffer, false);

	for (int i = 0; i < m_nMPEG2BufferPos-nIndex; i++) {
		dw = (dw<<8) + m_pMPEG2Buffer[i+nIndex];
		if (i >= 4) {
			if (m_nMPEG2PicEnd == INT_MIN) {
				if ( (dw & 0xffffff00) == 0x00000100 &&
						(dw & 0x000000FF) == (DWORD)nStartCode ) {
					m_nMPEG2PicEnd = i+nIndex-3;
				}
			} else {
				if ( (dw & 0xffffff00) == 0x00000100 &&
						((dw & 0x000000FF) == (DWORD)nStartCode || (dw & 0x000000FF) == 0xB3 )) {
					m_nMPEG2PicEnd = i+nIndex-3;
					return true;
				}
			}
		}

	}

	return false;
}

bool CDXVADecoderMpeg2::AppendBuffer(BYTE* pDataIn, int nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	if (rtStart != INVALID_TIME) {
		PushBufferTime (m_nMPEG2BufferPos, rtStart, rtStop);
	}

	if (m_nMPEG2BufferPos + nSize > m_nMPEG2BufferSize) {
		m_nMPEG2BufferSize	= m_nMPEG2BufferPos + nSize;
		m_pMPEG2Buffer		= (BYTE*)av_realloc(m_pMPEG2Buffer, m_nMPEG2BufferSize);
	}

	memcpy_sse(m_pMPEG2Buffer + m_nMPEG2BufferPos, pDataIn, nSize);

	m_nMPEG2BufferPos += nSize;

	return true;
}

void CDXVADecoderMpeg2::PopBufferTime(int nPos)
{
	int nDestPos	= 0;
	int i			= 0;

	// Shift buffer time list
	while (i < MAX_BUFF_TIME && m_MPEG2BufferTime[i].nBuffPos != INT_MIN) {
		if (m_MPEG2BufferTime[i].nBuffPos >= nPos) {
			m_MPEG2BufferTime[nDestPos].nBuffPos	= m_MPEG2BufferTime[i].nBuffPos - nPos;
			m_MPEG2BufferTime[nDestPos].rtStart		= m_MPEG2BufferTime[i].rtStart;
			m_MPEG2BufferTime[nDestPos].rtStop		= m_MPEG2BufferTime[i].rtStop;
			nDestPos++;
		}
		i++;
	}

	// Free unused slots
	for (i = nDestPos; i < MAX_BUFF_TIME; i++) {
		m_MPEG2BufferTime[i].nBuffPos	= INT_MIN;
		m_MPEG2BufferTime[i].rtStart	= INVALID_TIME;
		m_MPEG2BufferTime[i].rtStop		= INVALID_TIME;
	}
}

void CDXVADecoderMpeg2::PushBufferTime(int nPos, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	for (int i = 0; i < MAX_BUFF_TIME; i++) {
		if (m_MPEG2BufferTime[i].nBuffPos == INT_MIN) {
			m_MPEG2BufferTime[i].nBuffPos	= nPos;
			m_MPEG2BufferTime[i].rtStart	= rtStart;
			m_MPEG2BufferTime[i].rtStop		= rtStop;
			break;
		}
	}
}

void CDXVADecoderMpeg2::ResetBuffer()
{
	TRACE_MPEG2 ("CDXVADecoder::ResetBuffer()\n");

	if (m_pMPEG2Buffer) {
		av_freep(&m_pMPEG2Buffer);
	}
	m_pMPEG2Buffer		= NULL;
	m_nMPEG2BufferSize	= 0;

	m_nMPEG2BufferPos	= 0;
	m_nMPEG2PicEnd		= INT_MIN;

	for (int i = 0; i < MAX_BUFF_TIME; i++) {
		m_MPEG2BufferTime[i].nBuffPos	= INT_MIN;
		m_MPEG2BufferTime[i].rtStart	= INVALID_TIME;
		m_MPEG2BufferTime[i].rtStop		= INVALID_TIME;
	}
}

bool CDXVADecoderMpeg2::ShrinkBuffer()
{
	CheckPointer(m_pMPEG2Buffer, false);

	int nRemaining = m_nMPEG2BufferPos-m_nMPEG2PicEnd;

	PopBufferTime(m_nMPEG2PicEnd);
	memcpy_sse(m_pMPEG2Buffer, m_pMPEG2Buffer + m_nMPEG2PicEnd, nRemaining);
	m_nMPEG2BufferPos = nRemaining;

	m_nMPEG2PicEnd = (m_pMPEG2Buffer[3] == 0x00) ?  0 : INT_MIN;

	return true;
}
