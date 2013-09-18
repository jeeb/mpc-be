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

#pragma warning(disable: 4005 4244)
extern "C" {
	#include <stdint.h>
}

#define TAKSplitterName L"MPC TAK Splitter"
#define TAKSourceName   L"MPC TAK Source"

class __declspec(uuid("AA04C78C-3671-43F6-ABFE-6C265BAB2345"))
	CTAKSplitterFilter : public CBaseSplitterFilter
{
	__int64 m_startpos;
	__int64 m_endpos;
	
	REFERENCE_TIME m_rtStart;
	DWORD m_nAvgBytesPerSec;

	int     m_samplerate;
	int     m_bitdepth;
	int     m_channels;
	DWORD   m_layout;
	__int64 m_samples;
	int     m_framelen;
	int     m_totalframes;

	bool ParseTAKStreamInfo(BYTE* buff, int size);
	bool Sync(int& nFrameNumber);

protected:
	CAutoPtr<CBaseSplitterFileEx> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CTAKSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("A98DC1BA-E70D-47A0-BAD1-28C64A859FB1"))
	CTAKSourceFilter : public CTAKSplitterFilter
{
public:
	CTAKSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};