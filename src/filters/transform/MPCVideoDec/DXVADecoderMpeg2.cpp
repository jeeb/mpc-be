/*
 * $Id$
 *
 * (C) 2006-2012 see Authors.txt
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
#include "MPCVideoDecFilter.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

#if 0
	#define TRACE_MPEG2 TRACE
#else
	#define TRACE_MPEG2(...)
#endif

CDXVADecoderMpeg2::CDXVADecoderMpeg2 (CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder (pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderMpeg2::CDXVADecoderMpeg2 (CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder (pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	Init();
}

CDXVADecoderMpeg2::~CDXVADecoderMpeg2(void)
{
	NewSegment();
}

void CDXVADecoderMpeg2::Init()
{
	memset (&m_PictureParams,	0, sizeof(m_PictureParams));
	memset (&m_SliceInfo,		0, sizeof(m_SliceInfo));
	memset (&m_QMatrixData,		0, sizeof(m_QMatrixData));

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

	m_pMPEG2Buffer		= NULL;
	m_nMPEG2BufferSize	= 0;
	ResetBuffer();

}

HRESULT CDXVADecoderMpeg2::DecodeFrame (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	TRACE_MPEG2 ("CDXVADecoderMpeg2::DecodeFrame() : %d\n", nSize);

	AppendBuffer (pDataIn, nSize, rtStart, rtStop);
	HRESULT hr = S_OK;

	while (FindPicture (max (m_nMPEG2BufferPos-int(nSize)-4, 0), 0x00)) {
		if (m_MPEG2BufferTime[0].nBuffPos != INT_MIN && m_MPEG2BufferTime[0].nBuffPos < m_nMPEG2PicEnd) {
			rtStart = m_MPEG2BufferTime[0].rtStart;
			rtStop  = m_MPEG2BufferTime[0].rtStop;
		} else {
			rtStart = rtStop = _I64_MIN;
		}

		hr = DecodeFrameInternal (m_pMPEG2Buffer, m_nMPEG2PicEnd, rtStart, rtStop);
		ShrinkBuffer();
	}

	return hr;
}

HRESULT CDXVADecoderMpeg2::DecodeFrameInternal (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr;
	int		nFieldType = -1;
	int		nSliceType = -1;

	FFMpeg2DecodeFrame (&m_PictureParams, &m_QMatrixData, m_SliceInfo, &m_nSliceCount, m_pFilter->GetAVCtx(),
						m_pFilter->GetFrame(), &m_nNextCodecIndex, &nFieldType, &nSliceType, pDataIn, nSize);

	if (m_PictureParams.bSecondField && !m_bSecondField) {
		m_bSecondField = true;
	}

	if (!m_bSecondFieldPrev && !m_PictureParams.bSecondField) {
		m_bSecondField = false;	
	}

	m_bSecondFieldPrev = m_PictureParams.bSecondField;

	// Wait I frame after a flush
	if (m_bFlushed && (!m_PictureParams.bPicIntra || (m_bSecondField && m_PictureParams.bSecondField))) {
		TRACE_MPEG2 ("CDXVADecoderMpeg2::DecodeFrame() : Flush - wait I frame, SecondField = %d\n", m_PictureParams.bSecondField);
		return S_FALSE;
	}

	if (!m_bSecondField || (m_bSecondField && !m_PictureParams.bSecondField)) {
		m_pSampleToDeliver	= NULL;
		CHECK_HR (GetFreeSurfaceIndex (m_nSurfaceIndex, &m_pSampleToDeliver, rtStart, rtStop));
		m_rtStart			= rtStart;
		m_rtStop			= rtStop;
		UpdatePictureParams(m_nSurfaceIndex);	
	}

	if (m_pSampleToDeliver == NULL) {
		return S_FALSE;
	}

	CHECK_HR (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));

	TRACE_MPEG2 ("CDXVADecoderMpeg2::DecodeFrame() : Surf = %d, PictureType = %d, SecondField = %d, m_nNextCodecIndex = %d, rtStart = [%I64d]\n", m_nSurfaceIndex, nSliceType, m_PictureParams.bSecondField, m_nNextCodecIndex, rtStart);

	CHECK_HR (AddExecuteBuffer (DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));

	CHECK_HR (AddExecuteBuffer (DXVA2_InverseQuantizationMatrixBufferType, sizeof(m_QMatrixData), &m_QMatrixData));

	// Send bitstream to accelerator
	CHECK_HR (AddExecuteBuffer (DXVA2_SliceControlBufferType, sizeof (DXVA_SliceInfo)*m_nSliceCount, &m_SliceInfo));
	CHECK_HR (AddExecuteBuffer (DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize));

	// Decode frame
	CHECK_HR (Execute());
	CHECK_HR (EndFrame(m_nSurfaceIndex));

	if (!m_bSecondField || (m_bSecondField && m_PictureParams.bSecondField)) {
		AddToStore (m_nSurfaceIndex, m_pSampleToDeliver, (m_PictureParams.bPicBackwardPrediction != 1), m_rtStart, m_rtStop,
					false, (FF_FIELD_TYPE)nFieldType, (FF_SLICE_TYPE)nSliceType, FFGetCodedPicture(m_pFilter->GetAVCtx()));
		hr = DisplayNextFrame();
	}
		
	m_bFlushed = false;
	return hr;
}

void CDXVADecoderMpeg2::UpdatePictureParams(int nSurfaceIndex)
{
	DXVA2_ConfigPictureDecode*	cpd = GetDXVA2Config();		// Ok for DXVA1 too (parameters have been copied)

	m_PictureParams.wDecodedPictureIndex	= nSurfaceIndex;

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
		if (m_PictureParams.bBPPminus1 == 7 && m_PictureParams.bPicIntra && cpd->ConfigResidDiffHost)
			// Shall be 1 if BPP is 8 and bPicIntra is 1 and bConfigResidDiffHost is 1
		{
			m_PictureParams.bPicSpatialResid8 = 1;
		} else
			// Shall be 1 if bConfigSpatialResid8 is 1
		{
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

	memcpy_sse (pDXVABuffer, pBuffer, nSize);
}

void CDXVADecoderMpeg2::Flush()
{
	m_nNextCodecIndex = INT_MIN;

	if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
		RemoveRefFrame (m_wRefPictureIndex[0]);
	}
	if (m_wRefPictureIndex[1] != NO_REF_FRAME) {
		RemoveRefFrame (m_wRefPictureIndex[1]);
	}

	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;

	m_nSurfaceIndex		= 0;
	m_pSampleToDeliver	= NULL;
	m_bSecondField		= false;
	m_bSecondFieldPrev	= false;
	m_rtStart			= _I64_MIN;
	m_rtStop			= _I64_MIN;

	m_rtLastStart		= 0;

	m_FrameCount		= 0;
	m_rtLastValidStart	= 0;
	m_rtAvrTimePerFrame	= 0;

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

	for (int i=0; i<m_nPicEntryNumber; i++) {
		if (!m_pPictureStore[i].bDisplayed &&
				m_pPictureStore[i].bInUse &&
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

void CDXVADecoderMpeg2::UpdateFrameTime (REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	m_FrameCount++;
	if (rtStart != _I64_MIN) {
		if (m_FrameCount > 5) { // TODO - possible minimal count of frame without timestamp is different ... i think 5 is optimal
			m_rtAvrTimePerFrame = abs(rtStart - m_rtLastValidStart)/m_FrameCount;
		}

		m_rtLastValidStart	= rtStart;
		m_FrameCount		= 0;
	}

	if (m_rtLastStart && (rtStart == _I64_MIN || (rtStart < m_rtLastStart))) {
		rtStart = m_rtLastStart;
	}

	TRACE_MPEG2 ("UpdateFrameTime() : {AvrTimePerFrame = %10I64d, Calculated AvrTimePerFrame = %10I64d}\n", m_pFilter->GetAvrTimePerFrame(), m_rtAvrTimePerFrame);

	REFERENCE_TIME Duration	= m_rtAvrTimePerFrame ? m_rtAvrTimePerFrame : m_pFilter->GetAvrTimePerFrame();
	rtStop					= rtStart + (Duration / m_pFilter->GetRate());

	m_rtLastStart = rtStop;
}

bool CDXVADecoderMpeg2::FindPicture(int nIndex, int nStartCode)
{
	DWORD dw = 0;

	CheckPointer(m_pMPEG2Buffer, false);

	for (int i=0; i<m_nMPEG2BufferPos-nIndex; i++) {
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

bool CDXVADecoderMpeg2::AppendBuffer (BYTE* pDataIn, int nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	if (rtStart != _I64_MIN) {
		PushBufferTime (m_nMPEG2BufferPos, rtStart, rtStop);
	}

	if (m_nMPEG2BufferPos+nSize+FF_INPUT_BUFFER_PADDING_SIZE > m_nMPEG2BufferSize) {
		m_nMPEG2BufferSize	= m_nMPEG2BufferPos+nSize+FF_INPUT_BUFFER_PADDING_SIZE;
		m_pMPEG2Buffer		= (BYTE*)av_realloc(m_pMPEG2Buffer, m_nMPEG2BufferSize);
	}

	memcpy_sse(m_pMPEG2Buffer+m_nMPEG2BufferPos, pDataIn, nSize);

	m_nMPEG2BufferPos += nSize;

	return true;
}

void CDXVADecoderMpeg2::PopBufferTime(int nPos)
{
	int nDestPos	= 0;
	int i			= 0;

	// Shift buffer time list
	while (i<MAX_BUFF_TIME && m_MPEG2BufferTime[i].nBuffPos!=INT_MIN) {
		if (m_MPEG2BufferTime[i].nBuffPos >= nPos) {
			m_MPEG2BufferTime[nDestPos].nBuffPos	= m_MPEG2BufferTime[i].nBuffPos - nPos;
			m_MPEG2BufferTime[nDestPos].rtStart		= m_MPEG2BufferTime[i].rtStart;
			m_MPEG2BufferTime[nDestPos].rtStop		= m_MPEG2BufferTime[i].rtStop;
			nDestPos++;
		}
		i++;
	}

	// Free unused slots
	for (i=nDestPos; i<MAX_BUFF_TIME; i++) {
		m_MPEG2BufferTime[i].nBuffPos	= INT_MIN;
		m_MPEG2BufferTime[i].rtStart	= _I64_MIN;
		m_MPEG2BufferTime[i].rtStop		= _I64_MIN;
	}
}

void CDXVADecoderMpeg2::PushBufferTime(int nPos, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	for (int i=0; i<MAX_BUFF_TIME; i++) {
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

	for (int i=0; i<MAX_BUFF_TIME; i++) {
		m_MPEG2BufferTime[i].nBuffPos	= INT_MIN;
		m_MPEG2BufferTime[i].rtStart	= _I64_MIN;
		m_MPEG2BufferTime[i].rtStop		= _I64_MIN;
	}
}

bool CDXVADecoderMpeg2::ShrinkBuffer()
{
	CheckPointer(m_pMPEG2Buffer, false);

	int nRemaining = m_nMPEG2BufferPos-m_nMPEG2PicEnd;

	PopBufferTime (m_nMPEG2PicEnd);
	memcpy_sse (m_pMPEG2Buffer, m_pMPEG2Buffer+m_nMPEG2PicEnd, nRemaining);
	m_nMPEG2BufferPos = nRemaining;

	m_nMPEG2PicEnd = (m_pMPEG2Buffer[3] == 0x00) ?  0 : INT_MIN;

	return true;
}
