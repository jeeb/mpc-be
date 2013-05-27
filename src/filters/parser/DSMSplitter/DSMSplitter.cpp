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

#include "stdafx.h"
#include "DSMSplitter.h"
#include "../../../DSUtil/DSUtil.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_DirectShowMedia},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CDSMSplitterFilter), DSMSplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CDSMSourceFilter), DSMSourceName, MERIT_NORMAL, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDSMSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CDSMSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	CString str;
	str.Format(_T("0,%d,,%%0%dI64x"), DSMSW_SIZE, DSMSW_SIZE*2);
	str.Format(CString(str), DSMSW);

	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_DirectShowMedia,
		str, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_DirectShowMedia);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

CDSMSplitterFile::CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr, IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap)
	: CBaseSplitterFile(pReader, hr, false)
	, m_rtFirst(0)
	, m_rtDuration(0)
{
	if (FAILED(hr)) {
		return;
	}

	hr = Init(res, chap);
}

HRESULT CDSMSplitterFile::Init(IDSMResourceBagImpl& res, IDSMChapterBagImpl& chap)
{
	Seek(0);

	if (BitRead(DSMSW_SIZE<<3) != DSMSW || BitRead(5) != DSMP_FILEINFO) {
		return E_FAIL;
	}

	Seek(0);

	m_mts.RemoveAll();
	m_rtFirst = m_rtDuration = 0;
	m_fim.RemoveAll();
	m_sim.RemoveAll();
	res.ResRemoveAll();
	chap.ChapRemoveAll();

	dsmp_t type;
	UINT64 len;
	int limit = 65536;

	// examine the beginning of the file ...

	while (Sync(type, len, 0)) {
		__int64 pos = GetPos();

		if (type == DSMP_MEDIATYPE) {
			BYTE id;
			CMediaType mt;
			if (Read(len, id, mt)) {
				m_mts[id] = mt;
			}
		} else if (type == DSMP_SAMPLE) {
			Packet p;
			if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
				m_rtFirst = p.rtStart;
				break;
			}
		} else if (type == DSMP_FILEINFO) {
			if ((BYTE)BitRead(8) > DSMF_VERSION) {
				return E_FAIL;
			}
			Read(len-1, m_fim);
		} else if (type == DSMP_STREAMINFO) {
			Read(len-1, m_sim[(BYTE)BitRead(8)]);
		} else if (type == DSMP_SYNCPOINTS) {
			Read(len, m_sps);
		} else if (type == DSMP_RESOURCE) {
			Read(len, res);
		} else if (type == DSMP_CHAPTERS) {
			Read(len, chap);
		}

		Seek(pos + len);
	}

	if (type != DSMP_SAMPLE) {
		return E_FAIL;
	}

	// ... and the end

	if (IsRandomAccess())
		for (int i = 1, j = (int)((GetLength()+limit/2)/limit); i <= j; i++) {
			__int64 seekpos = max(0, (__int64)GetLength()-i*limit);
			Seek(seekpos);

			while (Sync(type, len, limit) && GetPos() < seekpos+limit) {
				__int64 pos = GetPos();

				if (type == DSMP_SAMPLE) {
					Packet p;
					if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
						m_rtDuration = max(m_rtDuration, p.rtStop - m_rtFirst); // max isn't really needed, only for safety
						i = j;
					}
				} else if (type == DSMP_SYNCPOINTS) {
					Read(len, m_sps);
				} else if (type == DSMP_RESOURCE) {
					Read(len, res);
				} else if (type == DSMP_CHAPTERS) {
					Read(len, chap);
				}

				Seek(pos + len);
			}
		}

	if (m_rtFirst < 0) {
		m_rtDuration += m_rtFirst;
		m_rtFirst = 0;
	}

	return m_mts.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDSMSplitterFile::Sync(dsmp_t& type, UINT64& len, __int64 limit)
{
	UINT64 pos;
	return Sync(pos, type, len, limit);
}

bool CDSMSplitterFile::Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, __int64 limit)
{
	BitByteAlign();

	limit += DSMSW_SIZE;

	for (UINT64 id = 0; (id&((1ui64<<(DSMSW_SIZE<<3))-1)) != DSMSW; id = (id << 8) | (BYTE)BitRead(8)) {
		if (limit-- <= 0 || GetRemaining() <= 2) {
			return false;
		}
	}

	syncpos = GetPos() - (DSMSW_SIZE<<3);
	type = (dsmp_t)BitRead(5);
	len = BitRead(((int)BitRead(3)+1)<<3);

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, BYTE& id, CMediaType& mt)
{
	id = (BYTE)BitRead(8);
	ByteRead((BYTE*)&mt.majortype, sizeof(mt.majortype));
	ByteRead((BYTE*)&mt.subtype, sizeof(mt.subtype));
	mt.bFixedSizeSamples = (BOOL)BitRead(1);
	mt.bTemporalCompression = (BOOL)BitRead(1);
	mt.lSampleSize = (ULONG)BitRead(30);
	ByteRead((BYTE*)&mt.formattype, sizeof(mt.formattype));
	len -= 5 + sizeof(GUID)*3;
	ASSERT(len >= 0);
	if (len > 0) {
		mt.AllocFormatBuffer((LONG)len);
		ByteRead(mt.Format(), mt.FormatLength());
	} else {
		mt.ResetFormatBuffer();
	}
	return true;
}

bool CDSMSplitterFile::Read(__int64 len, Packet* p, bool fData)
{
	if (!p) {
		return false;
	}

	p->TrackNumber = (DWORD)BitRead(8);
	p->bSyncPoint = (BOOL)BitRead(1);
	bool fSign = !!BitRead(1);
	int iTimeStamp = (int)BitRead(3);
	int iDuration = (int)BitRead(3);

	if (fSign && !iTimeStamp) {
		ASSERT(!iDuration);
		p->rtStart = INVALID_TIME;
		p->rtStop = INVALID_TIME + 1;
	} else {
		p->rtStart = (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		p->rtStop = p->rtStart + BitRead(iDuration<<3);
	}

	if (fData) {
		p->SetCount((INT_PTR)len - (2 + iTimeStamp + iDuration));
		ByteRead(p->GetData(), p->GetCount());
	}

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, CAtlArray<SyncPoint>& sps)
{
	SyncPoint sp = {0, 0};
	sps.RemoveAll();

	while (len > 0) {
		bool fSign = !!BitRead(1);
		int iTimeStamp = (int)BitRead(3);
		int iFilePos = (int)BitRead(3);
		BitRead(1); // reserved

		sp.rt += (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		sp.fp += BitRead(iFilePos<<3);
		sps.Add(sp);

		len -= 1 + iTimeStamp + iFilePos;
	}

	if (len != 0) {
		sps.RemoveAll();
		return false;
	}

	// TODO: sort sps

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, CStreamInfoMap& im)
{
	while (len >= 5) {
		CStringA key;
		ByteRead((BYTE*)key.GetBufferSetLength(4), 4);
		len -= 4;
		len -= Read(len, im[key]);
	}

	return len == 0;
}

bool CDSMSplitterFile::Read(__int64 len, IDSMResourceBagImpl& res)
{
	BYTE compression = (BYTE)BitRead(2);
	BYTE reserved = (BYTE)BitRead(6);
	UNREFERENCED_PARAMETER(reserved);
	len--;

	CDSMResource r;
	len -= Read(len, r.name);
	len -= Read(len, r.desc);
	len -= Read(len, r.mime);

	if (compression != 0) {
		return false;    // TODO
	}

	r.data.SetCount((size_t)len);
	ByteRead(r.data.GetData(), r.data.GetCount());

	res += r;

	return true;
}

bool CDSMSplitterFile::Read(__int64 len, IDSMChapterBagImpl& chap)
{
	CDSMChapter c(0, L"");

	while (len > 0) {
		bool fSign = !!BitRead(1);
		int iTimeStamp = (int)BitRead(3);
		BitRead(4); // reserved
		len--;

		c.rt += (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		len -= iTimeStamp;
		len -= Read(len, c.name);

		chap += c;
	}

	chap.ChapSort();

	return len == 0;
}

__int64 CDSMSplitterFile::Read(__int64 len, CStringW& str)
{
	char c;
	CStringA s;
	__int64 i = 0;
	while (i++ < len && (c = (char)BitRead(8)) != 0) {
		s += c;
	}
	str = UTF8To16(s);
	return i;
}

__int64 CDSMSplitterFile::FindSyncPoint(REFERENCE_TIME rt)
{
	if (/*!m_sps.IsEmpty()*/ m_sps.GetCount() > 1) {
		int i = range_bsearch(m_sps, m_rtFirst + rt);
		return i >= 0 ? m_sps[i].fp : 0;
	}

	if (m_rtDuration <= 0 || rt <= m_rtFirst) {
		return 0;
	}

	// ok, do the hard way then

	dsmp_t type;
	UINT64 syncpos, len;

	// 1. find some boundaries close to rt's position (minpos, maxpos)

	__int64 minpos = 0, maxpos = GetLength();

	for (int i = 0; i < 10 && (maxpos - minpos) >= 1024*1024; i++) {
		Seek((minpos + maxpos) / 2);

		while (GetPos() < maxpos) {
			if (!Sync(syncpos, type, len)) {
				continue;
			}

			__int64 pos = GetPos();

			if (type == DSMP_SAMPLE) {
				Packet p;
				if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
					REFERENCE_TIME dt = (p.rtStart -= m_rtFirst) - rt;
					if (dt >= 0) {
						maxpos = max((__int64)syncpos - 65536, minpos);
					} else {
						minpos = syncpos;
					}
					break;
				}
			}

			Seek(pos + len);
		}
	}

	// 2. find the first packet just after rt (maxpos)

	Seek(minpos);

	while (GetRemaining()) {
		if (!Sync(syncpos, type, len)) {
			continue;
		}

		__int64 pos = GetPos();

		if (type == DSMP_SAMPLE) {
			Packet p;
			if (Read(len, &p, false) && p.rtStart != INVALID_TIME) {
				REFERENCE_TIME dt = (p.rtStart -= m_rtFirst) - rt;
				if (dt >= 0) {
					maxpos = (__int64)syncpos;
					break;
				}
			}
		}

		Seek(pos + len);
	}

	// 3. iterate backwards from maxpos and find at least one syncpoint for every stream, except for subtitle streams

	CAtlMap<BYTE,BYTE> ids;

	{
		POSITION pos = m_mts.GetStartPosition();
		while (pos) {
			BYTE id;
			CMediaType mt;
			m_mts.GetNextAssoc(pos, id, mt);
			if (mt.majortype != MEDIATYPE_Text && mt.majortype != MEDIATYPE_Subtitle) {
				ids[id] = 0;
			}
		}
	}

	__int64 ret = maxpos;

	while (maxpos > 0 && !ids.IsEmpty()) {
		minpos = max(0, maxpos - 65536);

		Seek(minpos);

		while (Sync(syncpos, type, len) && GetPos() < maxpos) {
			UINT64 pos = GetPos();

			if (type == DSMP_SAMPLE) {
				Packet p;
				if (Read(len, &p, false) && p.rtStart != INVALID_TIME && p.bSyncPoint) {
					BYTE id = (BYTE)p.TrackNumber, tmp;
					if (ids.Lookup(id, tmp)) {
						ids.RemoveKey((BYTE)p.TrackNumber);
						ret = min(ret, (__int64)syncpos);
					}
				}
			}

			Seek(pos + len);
		}

		maxpos = minpos;
	}

	return ret;
}

//
// CDSMSplitterFilter
//

CDSMSplitterFilter::CDSMSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CDSMSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

CDSMSplitterFilter::~CDSMSplitterFilter()
{
}

STDMETHODIMP CDSMSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, DSMSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

static int compare_id(const void* id1, const void* id2)
{
	return (int)*(BYTE*)id1 - (int)*(BYTE*)id2;
}

HRESULT CDSMSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CDSMSplitterFile(pAsyncReader, hr, *this, *this));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->m_rtDuration;

	CAtlArray<BYTE> ids;

	POSITION pos = m_pFile->m_mts.GetStartPosition();
	while (pos) {
		BYTE id;
		CMediaType mt;
		m_pFile->m_mts.GetNextAssoc(pos, id, mt);
		ids.Add(id);
	}

	qsort(ids.GetData(), ids.GetCount(), sizeof(BYTE), compare_id);

	for (size_t i = 0; i < ids.GetCount(); i++) {
		BYTE id = ids[i];
		CMediaType& mt = m_pFile->m_mts[id];

		CStringW name, lang;
		name.Format(L"Output %02d", id);

		CAtlArray<CMediaType> mts;
		mts.Add(mt);

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, name, this, this, &hr));

		name.Empty();

		pos = m_pFile->m_sim[id].GetStartPosition();
		while (pos) {
			CStringA key;
			CStringW value;
			m_pFile->m_sim[id].GetNextAssoc(pos, key, value);
			pPinOut->SetProperty(CStringW(key), value);

			if (key == "NAME") {
				name = value;
			}
			if (key == "LANG") if ((lang = ISO6392ToLanguage(CStringA(CString(value)))).IsEmpty()) {
					lang = value;
				}
		}

		if (!name.IsEmpty() || !lang.IsEmpty()) {
			if (!name.IsEmpty()) {
				if (!lang.IsEmpty()) {
					name += L" (" + lang + L")";
				}
			} else if (!lang.IsEmpty()) {
				name = lang;
			}
			pPinOut->SetName(name);
		}

		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));
	}

	pos = m_pFile->m_fim.GetStartPosition();
	while (pos) {
		CStringA key;
		CStringW value;
		m_pFile->m_fim.GetNextAssoc(pos, key, value);
		SetProperty(CStringW(key), value);
	}

	for (size_t i = 0; i < m_resources.GetCount(); i++) {
		const CDSMResource& r = m_resources[i];
		if (r.mime == "application/x-truetype-font" ||
				r.mime == "application/x-font-ttf" ||
				r.mime == "application/vnd.ms-opentype") {
			//m_fontinst.InstallFont(r.data);
			m_fontinst.InstallFontMemory(r.data.GetData(), r.data.GetCount());
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDSMSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CDSMSplitterFilter");
	return true;
}

void CDSMSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_pFile->Seek(m_pFile->FindSyncPoint(rt));
}

bool CDSMSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining()) {
		dsmp_t type;
		UINT64 len;

		if (!m_pFile->Sync(type, len)) {
			continue;
		}

		__int64 pos = m_pFile->GetPos();

		if (type == DSMP_SAMPLE) {
			CAutoPtr<Packet> p(DNew Packet());
			if (m_pFile->Read(len, p)) {
				if (p->rtStart != INVALID_TIME) {
					p->rtStart -= m_pFile->m_rtFirst;
					p->rtStop -= m_pFile->m_rtFirst;
				}

				hr = DeliverPacket(p);
			}
		}

		m_pFile->Seek(pos + len);
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CDSMSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);
	nKFs = m_pFile->m_sps.GetCount();
	return S_OK;
}

STDMETHODIMP CDSMSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	// these aren't really the keyframes, but quicky accessable points in the stream
	for (nKFs = 0; nKFs < m_pFile->m_sps.GetCount(); nKFs++) {
		pKFs[nKFs] = m_pFile->m_sps[nKFs].rt;
	}

	return S_OK;
}

//
// CDSMSourceFilter
//

CDSMSourceFilter::CDSMSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CDSMSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
