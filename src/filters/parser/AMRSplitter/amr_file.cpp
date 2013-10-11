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

#include "stdafx.h"
#include "amr_file.h"
#include "AMRSplitter.h"
#include <math.h>

const int AMR_Frame_Size[] = {
	13, 14, 16, 18, 20, 21, 27, 32,
	6, 6, 6, 6, 0, 0, 0, 1
};

//-----------------------------------------------------------------------------
//
//	CAMRFile class
//
//-----------------------------------------------------------------------------

CAMRFile::CAMRFile() :
	duration_10mhz(0),
	reader(NULL)
{
	total_samples	= 0;
	current_sample	= 0;
}

CAMRFile::~CAMRFile()
{
}

// I/O for AMR file
int CAMRFile::Open(CAMRReader *reader)
{
	// each AMR file must begin with 6-byte long code
	uint8 magic[6];
	uint8 magic_expected[6] = { 0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A };

	int ret;

	// keep a local copy of the reader
	this->reader = reader;
	reader->Seek(0);
	ret = reader->Read(magic, 6);
	if (ret < 0) {
		return ret;
	}
	if (memcmp(magic, magic_expected, 6) != 0) {
		return -1;
	}

	seek_table.SetSize(0, 30);
	seek_table.RemoveAll();

	// now parse out all AMR frames
	total_frames = 0;
	CAMRPacket	p;
	for (;;) {
		ret = p.Load(reader);
		if (ret == 0) {
			// each 16 frames a seek point
			if ((total_frames & 0x0f) == 0) {
				seek_table.Add(p.file_position);
			}
			total_frames ++;
		} else {
			break;
		}
	}

	total_samples	= total_frames * 160;				// 160 samples per 20ms frame
	current_sample	= 0;
	duration_10mhz	= total_samples * 10000000 / 8000;	// 8KHz only

	// seek to begin
	Seek(0);

	return 0;
}

// parsing out packets
int CAMRFile::ReadAudioPacket(CAMRPacket *packet, int64 *cur_sample)
{
	// end of file ?
	if (current_sample >= total_samples) {
		return -2;
	}

	// load packet
	int ret = packet->Load(reader);
	if (ret < 0) {
		return ret;
	}

	// return sample number
	if (cur_sample) {
		*cur_sample = current_sample;
	}
	current_sample += 160;

	return 0;
}

int CAMRFile::Seek(int64 seek_sample)
{
	// right now we seek from the beginning
	if (seek_sample > total_samples) {
		seek_sample = total_samples;
	}
	if (seek_sample < 0) {
		seek_sample = 0;
	}

	int seek_index = (seek_sample / 160) >> 4;
	int64 seek_pos = 0;
	if (seek_index >= seek_table.GetCount()) {
		seek_pos	= 6;
		seek_index	= seek_table.GetCount() - 1;
		if (seek_index < 0) {
			seek_index = 0;
		}
	} else {
		seek_pos = seek_table[seek_index];
	}

	reader->Seek(seek_pos);
	current_sample = (seek_index * 16 * 160);
	return 0;
}

//-----------------------------------------------------------------------------
//
//	CAMRPacket
//
//-----------------------------------------------------------------------------

CAMRPacket::CAMRPacket() :
	file_position(0),
	packet_size(0)
{
}

CAMRPacket::~CAMRPacket()
{
}

int CAMRPacket::Load(CAMRReader *reader)
{
	int64	avail;
	int		ret;
	packet_size		= 0;
	file_position	= 0;

	tStart	= 0;
	tStop	= 0;

	reader->GetPosition(&file_position, &avail);

	// end of stream
	if (file_position >= avail) {
		return -2;
	}

	int toread	= min(sizeof(packet), (avail-file_position));
	ret			= reader->Read(packet, toread);
	if (ret < 0) {
		return ret;
	}

	// zistime velkost framu
	int type	= (packet[0] >> 3) & 0x0f;
	packet_size	= AMR_Frame_Size[type];
	if (packet_size == 0) {
		return -2;		// nejaka blbost
	}

	// seekneme na koniec framu
	reader->Seek(file_position + packet_size);
	return 0;
}
