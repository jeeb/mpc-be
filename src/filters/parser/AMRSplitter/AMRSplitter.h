/*
 *
 * Adaptation for MPC-BE (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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

#include "amr_file.h"

#define AMRSplitterName	L"MPC AMR Splitter"

// {24FA7933-FE18-46a9-914A-C2AA0DBACE93}
static const GUID CLSID_AMRSplitter = 
	{ 0x24fa7933, 0xfe18, 0x46a9, { 0x91, 0x4a, 0xc2, 0xaa, 0xd, 0xba, 0xce, 0x93 } };

class CAMRSplitter;

//-----------------------------------------------------------------------------
//
//	CAMRInputPin class
//
//-----------------------------------------------------------------------------
class CAMRInputPin : public CBaseInputPin
{
public:
	// parent splitter
	CAMRSplitter	*demux;
	IAsyncReader	*reader;

public:
	CAMRInputPin(TCHAR *pObjectName, CAMRSplitter *pDemux, HRESULT *phr, LPCWSTR pName);
	virtual ~CAMRInputPin();

	// Get hold of IAsyncReader interface
	HRESULT CompleteConnect(IPin *pReceivePin);
	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect();
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);

	// IMemInputPIn
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate);

	HRESULT Inactive();	
public:
	// Helpers
	CMediaType &CurrentMediaType() { return m_mt; }
	IAsyncReader *Reader() { return reader; }
};

//-----------------------------------------------------------------------------
//
//	DataPacketAMR class
//
//-----------------------------------------------------------------------------
class DataPacketAMR
{
public:
	int type;

	enum {
		PACKET_TYPE_EOS			= 0,
		PACKET_TYPE_DATA		= 1,
		PACKET_TYPE_NEW_SEGMENT	= 2
	};

	REFERENCE_TIME	rtStart, rtStop;
	double			rate;
	bool			has_time;
	BOOL			sync_point;
	BOOL			discontinuity;
	BYTE			*buf;
	int				size;

public:
	DataPacketAMR();
	virtual ~DataPacketAMR();
};

typedef CArray<CMediaType> CMediaTypes;

//-----------------------------------------------------------------------------
//
//	CAMROutputPin class
//
//-----------------------------------------------------------------------------
class CAMROutputPin : 
	public CBaseOutputPin, 
	public IMediaSeeking, 
	public CAMThread
{
public:
	enum {CMD_EXIT, CMD_STOP, CMD_RUN};

	// parser
	CAMRSplitter		*demux;
	int					stream_index;
	CMediaTypes			mt_types;

	// buffer queue
	int					buffers;
	CList<DataPacketAMR*>	queue;
	CCritSec			lock_queue;
	CAMEvent			ev_can_read;
	CAMEvent			ev_can_write;
	CAMEvent			ev_abort;
	int					buffer_time_ms;

	// time stamps
	REFERENCE_TIME		rtStart;
	REFERENCE_TIME		rtStop;
	double				rate;
	bool				active;
	bool				discontinuity;
	bool				eos_delivered;

public:
	CAMROutputPin(TCHAR *pObjectName, CAMRSplitter *pDemux, HRESULT *phr, LPCWSTR pName, int iBuffers);
	virtual ~CAMROutputPin();

	// IUNKNOWN
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// MediaType stuff
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin *pReceivePin);

	// Activation/Deactivation
	HRESULT Active();
	HRESULT Inactive();
	HRESULT DoNewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate);

	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

	// packet delivery mechanism
	HRESULT DeliverPacket(CAMRPacket &packet);
	HRESULT DeliverDataPacketAMR(DataPacketAMR &packet);
	HRESULT DoEndOfStream();

	// delivery thread
	virtual DWORD ThreadProc();
	void FlushQueue();
	int GetDataPacketAMR(DataPacketAMR **packet);

	// IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

public:
	CMediaType &CurrentMediaType() { return m_mt; }
	IMemAllocator *Allocator() { return m_pAllocator; }
	REFERENCE_TIME CurrentStart() { return rtStart; }
	REFERENCE_TIME CurrentStop() { return rtStop; }
	double CurrentRate() { return rate; }
	__int64 GetBufferTime_MS();
};

//-----------------------------------------------------------------------------
//
//	CAMRReader class
//
//-----------------------------------------------------------------------------

class CAMRReader
{
protected:
	IAsyncReader	*reader;
	__int64			position;

public:
	CAMRReader(IAsyncReader *rd);
	virtual ~CAMRReader();

	virtual int GetSize(__int64 *avail, __int64 *total);
	virtual int GetPosition(__int64 *pos, __int64 *avail);
	virtual int Seek(__int64 pos);
	virtual int Read(void *buf, int size);

	void BeginFlush()	{ if (reader) reader->BeginFlush(); }
	void EndFlush()		{ if (reader) reader->EndFlush(); }
};

//-----------------------------------------------------------------------------
//
//	CAMRSplitter class
//
//-----------------------------------------------------------------------------
class __declspec(uuid("24FA7933-FE18-46a9-914A-C2AA0DBACE93"))
    CAMRSplitter : 
	public CBaseFilter,
	public CAMThread,
	public IMediaSeeking
{
public:
	enum {CMD_EXIT, CMD_STOP, CMD_RUN};

	CCritSec					lock_filter;
	CAMRInputPin				*input;
	CArray<CAMROutputPin*>		output;
	CArray<CAMROutputPin*>		retired;
	CAMRReader					*reader;
	CAMRFile					*file;

	CAMEvent					ev_abort;

	// times
	REFERENCE_TIME				rtCurrent;
	REFERENCE_TIME				rtStop;
	double						rate;

public:
	// constructor
	CAMRSplitter(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAMRSplitter();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	// override this to publicise our interfaces
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// CBaseFilter
	virtual int GetPinCount();
	virtual CBasePin *GetPin(int n);

	// Output pins
	HRESULT AddOutputPin(CAMROutputPin *pPin);
	virtual HRESULT RemoveOutputPins();
	CAMROutputPin *FindStream(int stream_no);

	// check that we can support this input type
	virtual HRESULT CheckInputType(const CMediaType* mtIn);
	virtual HRESULT CheckConnect(PIN_DIRECTION Dir, IPin *pPin);
	virtual HRESULT BreakConnect(PIN_DIRECTION Dir, CBasePin *pCaller);
	virtual HRESULT CompleteConnect(PIN_DIRECTION Dir, CBasePin *pCaller, IPin *pReceivePin);
	virtual HRESULT ConfigureMediaType(CAMROutputPin *pin);

	// Demuxing thread
	virtual DWORD ThreadProc();

	// activate / deactivate filter
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	// IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	STDMETHODIMP SetPositionsInternal(int iD, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

	virtual HRESULT DoNewSeek();
};
