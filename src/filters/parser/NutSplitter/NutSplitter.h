/*
 * $Id
 *
 * (C) 2003-2006 Gabest
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

#pragma once

#include "../BaseSplitter/BaseSplitter.h"
#include "NutFile.h"

#define NutSplitterName L"MPC Nut Splitter"

class __declspec(uuid("90514D6A-76B7-4405-88A8-B4B1EF6061C6"))
CNutSplitterFilter : public CBaseSplitterFilter
{
	CAutoPtr<CNutFile> m_pFile;

protected:
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CNutSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	// IMediaSeeking
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
};

class __declspec(uuid("918B5A9F-DFED-4532-83A9-9B16D83ED73F"))
CNutSourceFilter : public CNutSplitterFilter
{
public:
	CNutSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
