/*
 * (C) 2014 see Authors.txt
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
#include "AMRFile.h"

static const uint8_t AMR_packed_size[16] = { 13, 14, 16, 18, 20, 21, 27, 32, 6, 1, 1, 1, 1, 1, 1, 1 };
static const uint8_t AMRWB_packed_size[16] = { 18, 24, 33, 37, 41, 47, 51, 59, 61, 6, 6, 0, 0, 0, 1, 1 };

//
// CAMRFile
//

CAMRFile::CAMRFile()
	: CAudioFile()
	, m_framelen(0)
	, m_isAMRWB(false)
	, m_currentframe(0)
{
	
}

CAMRFile::~CAMRFile()
{
}

HRESULT CAMRFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	m_pFile->Seek(0);
	BYTE data[9];
	if (m_pFile->ByteRead(data, 9) != S_OK) {
		return E_FAIL;
	}

	if (memcmp(data, AMR_header, 6) == 0) {
		m_pFile->Seek(6);

		m_isAMRWB = false;
		m_subtype = MEDIASUBTYPE_SAMR;
		m_samplerate = 8000;
		m_framelen   = 160;
	}
	else if (memcmp(data, AMRWB_header, 9) == 0) {
		m_isAMRWB = true;
		m_subtype = MEDIASUBTYPE_SAWB;
		m_samplerate = 16000;
		m_framelen   = 320;
	}
	else {
		return E_FAIL;
	}

	m_channels = 1;
	m_layout   = SPEAKER_FRONT_CENTER;
	m_startpos = m_pFile->GetPos();

	m_seek_table.SetSize(0, 512);
	m_seek_table.RemoveAll();
	BYTE toc = 0;
	while (m_pFile->ByteRead(&toc, 1) == S_OK) {
		unsigned mode = (toc >> 3) & 0x0F;
		int size = m_isAMRWB ? AMRWB_packed_size[mode] : AMR_packed_size[mode];

		if (size == 1) {
			continue;
		}
		if (!size || m_pFile->GetRemaining() < size - 1) {
			break;
		}


		m_endpos = m_pFile->GetPos() - 1;
		m_pFile->Seek(m_endpos + size);
		m_seek_table.Add(m_endpos);
	}
	if (m_endpos <= m_startpos) {
		return E_FAIL;
	}

	m_pFile->Seek(m_startpos);
	m_currentframe = 0;
	m_rtduration = 10000000i64 * m_framelen * m_seek_table.GetCount() / m_samplerate;

	return S_OK;
}

REFERENCE_TIME CAMRFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_currentframe = 0;
		m_pFile->Seek(m_seek_table[0]);
		return 0;
	}

	__int64 samples = rt * m_samplerate / 10000000;
	m_currentframe = samples / m_framelen;

	m_pFile->Seek(m_seek_table[m_currentframe]);
	rt = 10000000i64 * m_currentframe * m_framelen / m_samplerate;

	return rt;
}

int CAMRFile::GetAudioFrame(Packet* packet, REFERENCE_TIME rtStart)
{
	if (m_currentframe >= m_seek_table.GetCount()) {
		return 0;
	}

	packet->rtStart = 10000000i64 * m_currentframe * m_framelen / m_samplerate;
	m_pFile->Seek(m_seek_table[m_currentframe]);

	m_currentframe++;

	int size;
	if (m_currentframe == m_seek_table.GetCount()) {
		size = m_endpos - m_seek_table[m_currentframe - 1];
	} else {
		size = m_seek_table[m_currentframe] - m_seek_table[m_currentframe - 1];
	}
	if (size > 61) { size = 61; }

	packet->rtStop  = 10000000i64 * m_currentframe * m_framelen / m_samplerate;

	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->GetData(), size) != S_OK) {
		return 0;
	}

	return size;
}
