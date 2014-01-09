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

#include "AudioFile.h"

#define ID_APE 'MAC '

class CAPEFile : public CAudioFile
{
	typedef struct {
		int64_t pos;
		int nblocks;
		int size;
		int skip;
		int64_t pts;
	} APEFrame;

	size_t				m_curentframe;
	CAtlArray<APEFrame>	m_frames;

public:
	CAPEFile();

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(Packet* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return L"APE"; };
};
