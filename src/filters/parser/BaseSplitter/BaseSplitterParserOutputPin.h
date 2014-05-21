/*
 * (C) 2003-2006 Gabest
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

#include "../../../DSUtil/Packet.h"
#include "BaseSplitterOutputPin.h"

class CBaseSplitterParserOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	CAutoPtr<Packet>		m_p;
	CAutoPtrList<Packet>	m_pl;
	
	bool	m_fHasAccessUnitDelimiters;
	bool	m_bFlushed;
	int		m_truehd_framelength;

	WORD	m_nChannels;
	DWORD	m_nSamplesPerSec;
	WORD	m_wBitsPerSample;

protected:
	HRESULT DeliverPacket(CAutoPtr<Packet> p);
	HRESULT DeliverEndFlush();

	HRESULT Flush();

	void InitPacket(Packet* pSource);
	void InitAudioParams();

	HRESULT ParseAAC(CAutoPtr<Packet> p);
	HRESULT ParseAnnexB(CAutoPtr<Packet> p);
	HRESULT ParseVC1(CAutoPtr<Packet> p);
	HRESULT ParseHDMVLPCM(CAutoPtr<Packet> p);
	HRESULT ParseAC3(CAutoPtr<Packet> p);
	HRESULT ParseTrueHD(CAutoPtr<Packet> p);
	HRESULT ParseDirac(CAutoPtr<Packet> p);
	HRESULT ParseVobSub(CAutoPtr<Packet> p);

public:
	CBaseSplitterParserOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int QueueMaxPackets = 1);
	virtual ~CBaseSplitterParserOutputPin();
};
