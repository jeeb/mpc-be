/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
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

#include <atlbase.h>
#include <atlcoll.h>
#include <dsm/dsm.h>
#include "../../../DSUtil/DSMPropertyBag.h"
#include "../BaseSplitter/BaseSplitter.h"

#define DSMSplitterName L"MPC DSM Splitter"
#define DSMSourceName   L"MPC DSM Source"

class CDSMSplitterFile : public CBaseSplitterFile
{
	HRESULT Init(IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap);

public:
	CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr, IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap);

	CAtlMap<BYTE, CMediaType> m_mts;
	REFERENCE_TIME m_rtFirst, m_rtDuration;

	struct SyncPoint {
		REFERENCE_TIME rt;
		__int64 fp;
	};
	CAtlArray<SyncPoint> m_sps;

	typedef CAtlMap<CStringA, CStringW, CStringElementTraits<CStringA>, CStringElementTraits<CStringW> > CStreamInfoMap;
	CStreamInfoMap m_fim;
	CAtlMap<BYTE, CStreamInfoMap> m_sim;

	bool Sync(dsmp_t& type, UINT64& len, __int64 limit = 65536);
	bool Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, __int64 limit = 65536);
	bool Read(__int64 len, BYTE& id, CMediaType& mt);
	bool Read(__int64 len, Packet* p, bool fData = true);
	bool Read(__int64 len, CAtlArray<SyncPoint>& sps);
	bool Read(__int64 len, CStreamInfoMap& im);
	bool Read(__int64 len, IDSMResourceBagImpl& res);
	bool Read(__int64 len, IDSMChapterBagImpl& chap);
	__int64 Read(__int64 len, CStringW& str);

	__int64 FindSyncPoint(REFERENCE_TIME rt);
};

class __declspec(uuid("0912B4DD-A30A-4568-B590-7179EBB420EC"))
	CDSMSplitterFilter : public CBaseSplitterFilter
{
protected:
	CAutoPtr<CDSMSplitterFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CDSMSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CDSMSplitterFilter();

	// CBaseFilter
	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);

	// IKeyFrameInfo

	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

class __declspec(uuid("803E8280-F3CE-4201-982C-8CD8FB512004"))
	CDSMSourceFilter : public CDSMSplitterFilter
{
public:
	CDSMSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
