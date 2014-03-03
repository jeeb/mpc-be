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

#include <atlbase.h>
#include "../../../DSUtil/DSUtil.h"

class Packet : public CAtlArray<BYTE>
{
public:
	DWORD TrackNumber;
	BOOL bDiscontinuity, bSyncPoint, bAppendable;
	REFERENCE_TIME rtStart, rtStop;
	AM_MEDIA_TYPE* pmt;
	Packet() {
		TrackNumber = 0;
		pmt = NULL;
		bDiscontinuity = bSyncPoint = bAppendable = FALSE;
		rtStart = rtStop = INVALID_TIME;
	}
	virtual ~Packet() {
		if (pmt) {
			DeleteMediaType(pmt);
		}
	}
	virtual size_t GetDataSize() { return GetCount(); }
	void SetData(const void* ptr, DWORD len) {
		SetCount(len);
		memcpy(GetData(), ptr, len);
	}
};

class CPacketQueue
	: public CCritSec
	, protected CAutoPtrList<Packet>
{
	size_t m_size;

public:
	CPacketQueue();
	void Add(CAutoPtr<Packet> p);
	CAutoPtr<Packet> Remove();
	void RemoveAll();
	size_t GetCount(), GetSize();
};
