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

#include "../../../DSUtil/DSUtil.h"

class CAMRReader;

//-----------------------------------------------------------------------------
//
//	CAMRPacket
//
//-----------------------------------------------------------------------------

class CAMRPacket
{
public:
	int64		file_position;		// absolute file position (in bytes)
	uint8		packet[64];			// we own this one
	int32		packet_size;		// whole packet size	
	REFERENCE_TIME tStart, tStop;

public:
	CAMRPacket();
	virtual ~CAMRPacket();

	// loading packets
	int Load(CAMRReader *reader);
};

//-----------------------------------------------------------------------------
//
//	CAMRFile class
//
//-----------------------------------------------------------------------------

class CAMRFile
{
public:
	int64			duration_10mhz;			// total file duration
	int64			total_frames;			// total number of AMR frames

	// internals
	CAMRReader		*reader;				// file reader interface

	// current position
	int64			total_samples;
	int64			current_sample;
	CArray<int64>	seek_table;

public:
	CAMRFile();
	virtual ~CAMRFile();

	// I/O for AMR file
	int Open(CAMRReader *reader);

	// parsing out packets
	int ReadAudioPacket(CAMRPacket *packet, int64 *cur_sample);
	int Seek(int64 seek_sample);
};
