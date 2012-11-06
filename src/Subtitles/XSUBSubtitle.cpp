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
#include "XSUBSubtitle.h"
#include "../DSUtil/GolombBuffer.h"

CXSUBSubtitle::CXSUBSubtitle(CCritSec* pLock, const CString& name, LCID lcid)
	: CSubPicProviderImpl(pLock)
	, m_name(name)
	, m_lcid(lcid)
	, m_rtStart(0)
{
}

CXSUBSubtitle::~CXSUBSubtitle(void)
{
	Reset();
}

STDMETHODIMP CXSUBSubtitle::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = NULL;

	return
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(POSITION) CXSUBSubtitle::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld)
{
	CAutoLock cAutoLock(&m_csCritSec);

	if (CleanOld) {
		CXSUBSubtitle::CleanOld(rt/* - m_rtStart*/);
	}

	return m_pObjects.GetHeadPosition();
}

STDMETHODIMP_(POSITION) CXSUBSubtitle::GetNext(POSITION pos)
{
	CAutoLock cAutoLock(&m_csCritSec);

	m_pObjects.GetNext(pos);
	return pos;
}

STDMETHODIMP_(REFERENCE_TIME) CXSUBSubtitle::GetStart(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);

	CompositionObject* pObject = m_pObjects.GetAt(pos);
	return pObject!=NULL ? pObject->m_rtStart/* + m_rtStart */: INVALID_TIME;
}

STDMETHODIMP_(REFERENCE_TIME) CXSUBSubtitle::GetStop(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);

	CompositionObject* pObject = m_pObjects.GetAt(pos);
	return pObject!=NULL ? pObject->m_rtStop/* + m_rtStart */: INVALID_TIME;
}

STDMETHODIMP_(bool) CXSUBSubtitle::IsAnimated(POSITION pos)
{
	return false;
}

STDMETHODIMP CXSUBSubtitle::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CAutoLock cAutoLock(&m_csCritSec);
	
	/*
	Render (spd, rt - m_rtStart, bbox);
	*/

	CleanOld(rt - /*m_rtStart - */60*10000000i64); // Cleanup subtitles older than 1 minute ...

	return S_OK;
}

STDMETHODIMP CXSUBSubtitle::GetTextureSize (POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
	CAutoLock cAutoLock(&m_csCritSec);
	HRESULT hr = E_FAIL;//m_pSub->GetTextureSize(pos, MaxTextureSize, VideoSize, VideoTopLeft);
	return hr;
};

STDMETHODIMP_(SUBTITLE_TYPE) CXSUBSubtitle::GetType(POSITION pos)
{
	return ST_XSUB;
}

// IPersist

STDMETHODIMP CXSUBSubtitle::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CXSUBSubtitle::GetStreamCount()
{
	return (1);
}

STDMETHODIMP CXSUBSubtitle::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	if (iStream != 0) {
		return E_INVALIDARG;
	}

	if (ppName) {
		*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR));
		if (!(*ppName)) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s (*ppName, m_name.GetLength()+1, CStringW(m_name));
	}

	if (pLCID) {
		*pLCID = m_lcid;
	}

	return S_OK;
}

STDMETHODIMP_(int) CXSUBSubtitle::GetStream()
{
	return(0);
}

STDMETHODIMP CXSUBSubtitle::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CXSUBSubtitle::Reload()
{
	return S_OK;
}

HRESULT CXSUBSubtitle::ParseSample (IMediaSample* pSample)
{
	CAutoLock cAutoLock(&m_csCritSec);
	HRESULT		hr = E_FAIL;

	CheckPointer (pSample, E_POINTER);
	REFERENCE_TIME	rtStart = INVALID_TIME, rtStop = INVALID_TIME;
	BYTE*			pData = NULL;
	int				lSampleLen;

	hr = pSample->GetPointer(&pData);
	if (FAILED(hr) || pData == NULL) {
		return hr;
	}
	lSampleLen = pSample->GetActualDataLength();
	if (lSampleLen < (27 + 7 * 2 + 4 * 3)) {
		return E_FAIL;
	}

	if (pData[0] != '[' || pData[13] != '-' || pData[26] != ']') {
		return E_FAIL;
	}

	CString tmp((char*)pData, 26);
	rtStart	= StringToReftime(tmp.Mid(1, 12));
	rtStop	= StringToReftime(tmp.Mid(14, 12));

	if (rtStop <= rtStart) {
		return E_FAIL;
	}

	CompositionObject*	pSub = DNew CompositionObject;
	pSub->m_rtStart	= rtStart;
	pSub->m_rtStop	= rtStop;

	CGolombBuffer gb(pData + 27, lSampleLen - 27);
	pSub->m_width				= gb.ReadShortLE();
	pSub->m_height				= gb.ReadShortLE();
	pSub->m_horizontal_position	= gb.ReadShortLE();
	pSub->m_vertical_position	= gb.ReadShortLE();
	// skip bottom right position
	gb.ReadShortLE();
	gb.ReadShortLE();
	// length of the RLE data
	gb.ReadShortLE();
	// Palette, 4 color entries
	HDMV_PALETTE Palette[4];
	for (int entry=0; entry < 4; entry++) {
		Palette[entry].entry_id	= entry;
		Palette[entry].Y	= gb.ReadByte();	// red
		Palette[entry].Cr	= gb.ReadByte();	// green
		Palette[entry].Cb	= gb.ReadByte();	// blue
	}

	pSub->SetPalette(4, Palette, false);

	int RLESize = gb.GetSize() - gb.GetPos();
	pSub->SetRLEData(gb.GetBufferPos(), RLESize, RLESize);

	m_pObjects.AddTail(pSub);

	hr = S_OK;

	return hr;
}

HRESULT CXSUBSubtitle::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csCritSec);

	Reset();
	m_rtStart = tStart;
	return S_OK;
}

void CXSUBSubtitle::Reset()
{
	CompositionObject* pObject;
	while (m_pObjects.GetCount() > 0) {
		pObject = m_pObjects.RemoveHead();
		if (pObject) {
			delete pObject;
		}
	}

}
void CXSUBSubtitle::CleanOld(REFERENCE_TIME rt)
{
	CompositionObject* pObject_old;

	while (m_pObjects.GetCount()>0) {
		pObject_old = m_pObjects.GetHead();
		if (pObject_old->m_rtStop < rt) {
			m_pObjects.RemoveHead();
			delete pObject_old;
		} else {
			break;
		}
	}
}