/*
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

#include "stdafx.h"
#include <MMReg.h>
#include "DTSSplitterFile.h"
#include "../../../DSUtil/AudioParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#pragma warning(push)
#pragma warning(disable: 4005)
#include <stdint.h>
#pragma warning(pop)

#define SYNC_OFFESET 0x122c

static bool isDTSSync(const DWORD sync)
{
	if (sync == 0x0180fe7f || //16 bits and big endian bitstream
		sync == 0x80017ffe || //16 bits and little endian bitstream
		sync == 0x00e8ff1f || //14 bits and big endian bitstream
		sync == 0xe8001fff) { //14 bits and little endian bitstream
		return true;
	} else {
		return false;
	}
}

static DWORD ParseWAVECDHeader(const BYTE wh[44])
{
	if (*(DWORD*)wh != 0x46464952 //"RIFF"
			|| *(DWORDLONG*)(wh+8) != 0x20746d6645564157 //"WAVEfmt "
			|| *(DWORD*)(wh+36) != 0x61746164) { //"data"
		return 0;
	}
	PCMWAVEFORMAT pcmwf = *(PCMWAVEFORMAT*)(wh+20);
	if (pcmwf.wf.wFormatTag != 1
			|| pcmwf.wf.nChannels != 2
			|| pcmwf.wf.nSamplesPerSec != 44100
			|| pcmwf.wf.nAvgBytesPerSec != 176400
			|| pcmwf.wf.nBlockAlign != 4
			|| pcmwf.wBitsPerSample != 16) {
		return 0;
	}
	return *(DWORD*)(wh+40); //return size of "data"
}

//

CDTSSplitterFile::CDTSSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr, false)
	, m_rtDuration(0)
	, m_startpos(0)
	, m_endpos(0)
	, m_framesize(0)
	, m_framelength(0)
	, m_AvgTimePerFrame(0)
{
	if (SUCCEEDED(hr)) {
		hr = Init();
	}
}

HRESULT CDTSSplitterFile::Init()
{
	Seek(0);
	m_startpos = 0;
	m_endpos = GetLength();

	BYTE buf[44];
	if(SUCCEEDED(ByteRead((BYTE*)buf, 44))) {
		DWORD ret = ParseWAVECDHeader(buf);
		if (!ret) {
			return E_FAIL;
		}
	} else {
		return E_FAIL;
	}

	DWORD id, dts_offset = 0;
	if(SUCCEEDED(ByteRead((BYTE*)&id, sizeof(id))) && isDTSSync(id)) {
		dts_offset = GetPos() - 4;
	}

	if (!dts_offset) {
		dts_offset = SYNC_OFFESET;
	}

	Seek(dts_offset);
	if(SUCCEEDED(ByteRead((BYTE*)&id, sizeof(id))) && isDTSSync(id)) {
		Seek(dts_offset);
		BYTE buf[16];
		if(FAILED(ByteRead(buf, 16))) {
			return E_FAIL;
		}

		// DTS header
		audioframe_t aframe;
		m_framesize = ParseDTSHeader(buf, &aframe);
		if (m_framesize == 0) {
			return E_FAIL;
		}
		m_framelength = aframe.samples;

		Seek(dts_offset + m_framesize);
		if(SUCCEEDED(ByteRead((BYTE*)&id, sizeof(id))) && isDTSSync(id)) {

			int bitrate = int ((m_framesize) * 8i64 * aframe.samplerate / m_framelength);

			m_AvgTimePerFrame = bitrate ? (10000000i64 * m_framesize * 8 / bitrate) : 0;

			m_rtDuration = m_AvgTimePerFrame * (GetLength() - SYNC_OFFESET) / m_framesize;

			TRACE(_T("CDTSSplitterFile::Init() : Duration : %ws\n"), ReftimeToString(m_rtDuration));

			memset(&m_mt, 0, sizeof(m_mt));

			m_mt.majortype			= MEDIATYPE_Audio;
			m_mt.subtype			= MEDIASUBTYPE_DTS;
			m_mt.formattype			= FORMAT_WaveFormatEx;
			WAVEFORMATEX* wfe		= (WAVEFORMATEX*)m_mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
			wfe->wFormatTag			= WAVE_FORMAT_DTS;
			wfe->nChannels			= aframe.channels;
			wfe->nSamplesPerSec		= aframe.samplerate;
			wfe->nAvgBytesPerSec	= (bitrate + 4) /8;
			wfe->nBlockAlign		= m_framesize < WORD_MAX ? m_framesize : WORD_MAX;
			wfe->wBitsPerSample		= 0;
			wfe->cbSize = 0;

			Seek(dts_offset);

			return S_OK;
		}
	}

	return E_FAIL;
}

bool CDTSSplitterFile::GetInfo(int& FrameSize, REFERENCE_TIME& rtDuration)
{
	FrameSize  = m_framesize;
	rtDuration = m_AvgTimePerFrame;

	return false;
}
