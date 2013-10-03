/*
 * $Id:
 *
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

#define RAWSplitterName L"MPC RAW Splitter"
#define RAWSourceName   L"MPC RAW Source"

class __declspec(uuid("486AA463-EE67-4F75-B941-F1FAB217B342"))
	CRAWSplitterFilter : public CBaseSplitterFilter
{
	enum {
		RAW_NONE,
		RAW_MPEG1,
		RAW_MPEG2
	} m_RAWType;

	REFERENCE_TIME m_rtStart;
	REFERENCE_TIME m_AvgTimePerFrame;

protected:
	CAutoPtr<CBaseSplitterFileEx> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CRAWSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("E32A3501-04A9-486B-898B-F5A4C8A4AAAC"))
	CRAWSourceFilter : public CRAWSplitterFilter
{
public:
	CRAWSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};