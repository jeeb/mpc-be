/*
 * $Id$
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

#include "../BaseSplitter/BaseSplitter.h"

#define DTSSplitterName L"MPC DTS AudioCD Splitter"

class CDTSSplitterFile : public CBaseSplitterFileEx
{
	CMediaType m_mt;

	__int64 m_startpos, m_endpos;

	int m_framesize;
	int m_framelength;

	REFERENCE_TIME m_rtDuration;
	REFERENCE_TIME m_AvgTimePerFrame;

	HRESULT Init();

public:
	CDTSSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	const CMediaType& GetMediaType() {
		return m_mt;
	}
	REFERENCE_TIME GetDuration() {
		return IsRandomAccess() ? m_rtDuration : 0;
	}

	__int64 GetStartPos() {
		return m_startpos;
	}
	__int64 GetEndPos() {
		return m_endpos;
	}

	bool GetInfo(int& FrameSize, REFERENCE_TIME& rtDuration);
};

class __declspec(uuid("316A424F-0B92-41BC-87F4-56BD5196B808"))
	CDTSSplitterFilter : public CBaseSplitterFilter
{
	REFERENCE_TIME m_rtStart;

protected:
	CAutoPtr<CDTSSplitterFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	STDMETHODIMP GetDuration(LONGLONG* pDuration);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CDTSSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
};
