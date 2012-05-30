/*
 * $Id$
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 *
 * This file is part of MPC-BE.
 * YOU CANNOT USE THIS FILE WITHOUT AUTHOR PERMISSION!
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

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#pragma warning(push)
#pragma warning(disable: 4005)
#include <stdint.h>
#pragma warning(pop)
#include <libdca/include/dts.h>

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
	: CBaseSplitterFileEx(pAsyncReader, hr, DEFAULT_CACHE_LENGTH, false)
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
		dts_state_t* m_dts_state;
		m_dts_state = dts_init(0);
		int flags, targeted_bitrate;
		int m_samplerate = 0;
		if ((m_framesize = dts_syncinfo(m_dts_state, buf, &flags, &m_samplerate, &targeted_bitrate, &m_framelength)) < 96) { //minimal valid fsize = 96
			return E_FAIL;
		}

		Seek(dts_offset + m_framesize);
		if(SUCCEEDED(ByteRead((BYTE*)&id, sizeof(id))) && isDTSSync(id)) {

			int m_bitrate = int ((m_framesize) * 8i64 * m_samplerate / m_framelength);

			const int channels[16] = {1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8};
			int m_channels = 0;
			if (flags & 0x70) {//unknown number of channels
				m_channels = 6;
			} else {
				m_channels = channels[flags & 0x0f];
				if (flags & DCA_LFE) {
					m_channels += 1; //+LFE
				}
			}

			m_AvgTimePerFrame = m_bitrate ? (10000000i64 * m_framesize * 8 / m_bitrate) : 0;

			m_rtDuration = m_AvgTimePerFrame * (GetLength() - SYNC_OFFESET) / m_framesize;

			TRACE(_T("CDTSSplitterFile::Init() : Duration : %ws\n"), ReftimeToString(m_rtDuration));

			memset(&m_mt, 0, sizeof(m_mt));

			m_mt.majortype			= MEDIATYPE_Audio;
			m_mt.subtype			= MEDIASUBTYPE_DTS;
			m_mt.formattype			= FORMAT_WaveFormatEx;
			WAVEFORMATEX* wfe		= (WAVEFORMATEX*)m_mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
			wfe->wFormatTag			= WAVE_FORMAT_DTS;
			wfe->nChannels			= m_channels;
			wfe->nSamplesPerSec		= m_samplerate;
			wfe->nAvgBytesPerSec	= (m_bitrate + 4) /8;
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
	FrameSize	= m_framesize;
	rtDuration	= m_AvgTimePerFrame;

	return false;
}
