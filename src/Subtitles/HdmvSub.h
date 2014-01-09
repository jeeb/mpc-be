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

#include "BaseSub.h"

#define MAX_WINDOWS 64
class HDMV_WindowDefinition
{
public:
	SHORT					m_compositionNumber;
	SHORT					m_palette_id_ref;
	BYTE					m_nObjectNumber;
	CompositionObject*		Objects[MAX_WINDOWS];

	HDMV_WindowDefinition() {
		Reset();

		for (int i = 0; i < MAX_WINDOWS; i++) {
			Objects[i] = NULL;
		}
	}

	~HDMV_WindowDefinition() {
		for (int i = 0; i < MAX_WINDOWS; i++) {
			SAFE_DELETE(Objects[i]);
		}
	}

	void Reset() {
		m_compositionNumber	= -1;
		m_palette_id_ref	= -1;
		m_nObjectNumber		= 0;
	}
};

class CGolombBuffer;

class CHdmvSub : public CBaseSub
{
public:
	enum HDMV_SEGMENT_TYPE {
		NO_SEGMENT			= 0xFFFF,
		PALETTE				= 0x14,
		OBJECT				= 0x15,
		PRESENTATION_SEG	= 0x16,
		WINDOW_DEF			= 0x17,
		INTERACTIVE_SEG		= 0x18,
		END_OF_DISPLAY		= 0x80,
		HDMV_SUB1			= 0x81,
		HDMV_SUB2			= 0x82
	};

	struct VIDEO_DESCRIPTOR {
		SHORT		nVideoWidth;
		SHORT		nVideoHeight;
		BYTE		bFrameRate;
	};

	struct COMPOSITION_DESCRIPTOR {
		SHORT		nNumber;
		BYTE		bState;
	};

	struct SEQUENCE_DESCRIPTOR {
		BYTE		bFirstIn  : 1;
		BYTE		bLastIn	  : 1;
		BYTE		bReserved : 8;
	};

	struct HDMV_CLUT {
		int				pSize;
		HDMV_PALETTE*	Palette;

		HDMV_CLUT() {
			pSize	= 0;
			Palette	= NULL;
		}
	};

	CHdmvSub();
	~CHdmvSub();

	virtual HRESULT			ParseSample (IMediaSample* pSample);
	HRESULT					ParseSample (BYTE* pData, int lSampleLen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);

	virtual POSITION		GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld = false);
	virtual POSITION		GetNext(POSITION pos) {
		m_pObjects.GetNext(pos);
		return pos;
	};

	virtual REFERENCE_TIME	GetStart(POSITION nPos) {
		CompositionObject* pObject = m_pObjects.GetAt(nPos);
		return pObject != NULL ? pObject->m_rtStart : INVALID_TIME;
	};

	virtual REFERENCE_TIME	GetStop(POSITION nPos) {
		CompositionObject* pObject = m_pObjects.GetAt(nPos);
		return pObject != NULL ? pObject->m_rtStop : INVALID_TIME;
	};

	virtual void			Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox);
	virtual HRESULT			GetTextureSize (POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft);
	virtual void			Reset();

	virtual void			CleanOld(REFERENCE_TIME rt);
	virtual HRESULT			EndOfStream() { return S_OK; }

private :
	HDMV_SEGMENT_TYPE				m_nCurSegment;
	BYTE*							m_pSegBuffer;
	int								m_nTotalSegBuffer;
	int								m_nSegBufferPos;
	int								m_nSegSize;

	VIDEO_DESCRIPTOR				m_VideoDescriptor;

	CAtlList<CompositionObject*>	m_pObjects;
	HDMV_WindowDefinition*			m_pCurrentWindow;
	CompositionObject				m_ParsedObjects[MAX_WINDOWS];
	
	HDMV_CLUT						m_CLUT[256];
	HDMV_CLUT						m_DefaultCLUT;

	int								m_nColorNumber;

	void				ParsePresentationSegment(CGolombBuffer* pGBuffer, REFERENCE_TIME rtTime);
	void				ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize);
	void				ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize);

	void				ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor);
	void				ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor);
	void				ParseCompositionObject(CGolombBuffer* pGBuffer, CompositionObject* pCompositionObject);

	void				AllocSegment(int nSize);

	CompositionObject*	FindObject(REFERENCE_TIME rt);
};
