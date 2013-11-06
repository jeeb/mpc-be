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

#include <dxva.h>
#include "DXVADecoder.h"

#define MAX_SLICE		1024 // Max slice number for Mpeg2 streams

#define MAX_BUFF_TIME	20

typedef struct {
	REFERENCE_TIME	rtStart;
	REFERENCE_TIME	rtStop;
	int				nBuffPos;
} BUFFER_TIME;


class CDXVADecoderMpeg2 : public CDXVADecoder
{
public:
	CDXVADecoderMpeg2(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderMpeg2(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderMpeg2(void);

	// === Public functions
	virtual HRESULT DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void	CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void	Flush();
	virtual void	NewSegment();

protected :

	HRESULT			DecodeFrameInternal(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual int		FindOldestFrame();
private:
	DXVA_PictureParameters	m_PictureParams;
	DXVA_QmatrixData		m_QMatrixData;
	WORD					m_wRefPictureIndex[2];
	DXVA_SliceInfo			m_SliceInfo[MAX_SLICE];
	int						m_nSliceCount;

	int						m_nNextCodecIndex;

	REFERENCE_TIME			m_rtLastStart;

	bool 					m_bFrame_repeat_pict;

	// Private functions
	void					Init();
	void					UpdatePictureParams(int nSurfaceIndex);
	void					UpdateFrameTime (REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);

protected:
	BYTE*			m_pMPEG2Buffer;
	int				m_nMPEG2BufferSize;

	int				m_nMPEG2BufferPos;
	int				m_nMPEG2PicEnd;
	BUFFER_TIME		m_MPEG2BufferTime[MAX_BUFF_TIME];

	bool			FindPicture(int nIndex, int nStartCode);
	bool			AppendBuffer(BYTE* pDataIn, int nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	void			PopBufferTime(int nPos);
	void			PushBufferTime(int nPos, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	void			ResetBuffer();
	bool			ShrinkBuffer();
};
