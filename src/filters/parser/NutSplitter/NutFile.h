/*
 * $Id
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
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

#include "../BaseSplitter/BaseSplitter.h"

class CNutFile : public CBaseSplitterFile
{
	HRESULT Init();

public:

#pragma pack(push, 1)

	typedef UINT64 vint;
	typedef INT64 svint;
	typedef CAtlArray<BYTE> binary;
	typedef CAtlArray<BYTE> string;

	struct packet_header
	{
		__int64 pos;
		vint fptr, bptr;
		UINT32 checksum;
	};

	struct main_header
	{
		vint version;
		vint stream_count;
	};

	struct codec_specific
	{
		vint type;
		binary data;
	};

	struct video_stream_header
	{
		vint width, height;
		vint sample_width, sample_height;
		vint colorspace_type;
	};

	struct audio_stream_header
	{
		vint samplerate_mul;
		vint channel_count;
	};

	struct stream_header
	{
		vint stream_id;
		vint stream_class;
		string fourcc;
		vint average_bitrate;
		string language_code;
		vint time_base_nom;
		vint time_base_denom;
		vint msb_timestamp_shift;
		vint shuffle_type;
		int fixed_fps:1;
		int index_flag:1;
		int reserved:6;
		CAutoPtrList<codec_specific> cs;
		union {video_stream_header vsh; audio_stream_header ash;};
		vint msb_timestamp;
	};

	struct frame_header
	{
		BYTE zero_bit:1;
		BYTE priority:2;
		BYTE checksum_flag:1;
		BYTE msb_timestamp_flag:2;
		BYTE subpacket_type:2;
		BYTE reserved:1;
	};

	struct index_entry
	{
		vint timestamp;
		vint position;
	};

	struct index_header
	{
        vint stream_id;
		CAtlArray<index_entry> ie;
	};

	struct info_header
	{
		// TODO
		vint dummy;
	};

#pragma pack(pop)

	#define NUTM 0xF9526A624E55544Dui64
	#define NUTS 0xD667773F4E555453ui64
	#define NUTK 0xCB8630874E55544Bui64
	#define NUTX 0xEBFCDE0E4E555458ui64
	#define NUTI 0xA37B64354E555449ui64

	enum {SC_VIDEO = 0, SC_AUDIO = 32, SC_SUBTITLE = 64};

	void Read(vint& v);
	void Read(svint& sv);
	void Read(binary& b);
	void Read(packet_header& ph);
	void Read(main_header& mh);
	void Read(stream_header& sh);
	void Read(video_stream_header& vsh);
	void Read(audio_stream_header& ash);
	void Read(index_header& ih);
	void Read(info_header& ih);

public:

	CNutFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	main_header m_mh;
	CAutoPtrList<stream_header> m_streams;
};
