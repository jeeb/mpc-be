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

class CDXVADecoderVC1 : public CDXVADecoder
{
public:
	CDXVADecoderVC1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderVC1(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderVC1();

	// === Public functions
	virtual HRESULT DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void	CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void	Flush();

private:
	DXVA_PictureParameters		m_PictureParams;
	DXVA_SliceInfo				m_SliceInfo;
	WORD						m_wRefPictureIndex[2];

	int							m_nDelayedSurfaceIndex;
	REFERENCE_TIME				m_rtStartDelayed;
	REFERENCE_TIME				m_rtStopDelayed;

	BOOL 						m_bFrame_repeat_pict;

	// Private functions
	void						Init();
	HRESULT						DisplayStatus();
	BYTE*						FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize);
};
