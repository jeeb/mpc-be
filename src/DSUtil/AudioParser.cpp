/*
 * $Id$
 *
 * (C) 2011-2012 see Authors.txt
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
#include "AudioParser.h"
#include "GolombBuffer.h"

#include "DSUtil.h"

#define AC3_CHANNEL                  0
#define AC3_MONO                     1
#define AC3_STEREO                   2
#define AC3_3F                       3
#define AC3_2F1R                     4
#define AC3_3F1R                     5
#define AC3_2F2R                     6
#define AC3_3F2R                     7
#define AC3_CHANNEL1                 8
#define AC3_CHANNEL2                 9
#define AC3_DOLBY                   10
#define AC3_CHANNEL_MASK            15
#define AC3_LFE                     16

int GetAC3FrameSize(const BYTE *buf)
{
	if (*(WORD*)buf != AC3_SYNC_WORD) // syncword
		return 0;

	int frame_size;

	if (buf[5] >> 3 <= 10) {   // Normal AC-3
		static const int rates[] = {32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};

		int frmsizecod = buf[4] & 0x3F;
		if (frmsizecod >= 38)
			return 0;

		int rate  = rates[frmsizecod >> 1];
		switch (buf[4] & 0xc0) {
			case 0:
				frame_size = 4 * rate;
				break;
			case 0x40:
				frame_size = 2 * (320 * rate / 147 + (frmsizecod & 1));
				break;
			case 0x80:
				frame_size = 6 * rate;
				break;
			default:
				return 0;
		}
	} else {   /// Enhanced AC-3
		frame_size = (((buf[2] & 0x03) << 8) + buf[3] + 1) * 2;
	}
	return frame_size;
}

int GetMLPFrameSize(const BYTE *buf)
{
	DWORD sync = *(DWORD*)(buf+4);
	if (sync == TRUEHD_SYNC_WORD || sync == MLP_SYNC_WORD) {
		return (((buf[0] << 8) | buf[1]) & 0xfff) * 2;
	}
	return 0;
}


int ParseAC3Header(const BYTE *buf, int *samplerate, int *channels, int *framelength, int *bitrate)
{
	if (*(WORD*)buf != AC3_SYNC_WORD) // syncword
		return 0;

	if (buf[5] >> 3 >= 11)   // bsid
		return 0;

	static const int rates[] = {32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
	static const unsigned char lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};
	static const unsigned char halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};

	int frmsizecod = buf[4] & 0x3F;
	if (frmsizecod >= 38)
		return 0;

	int half = halfrate[buf[5] >> 3];
	int rate  = rates[frmsizecod >> 1];
	*bitrate  = (rate * 1000) >> half;
	int frame_size;
	switch (buf[4] & 0xc0) {
		case 0:
			*samplerate = 48000 >> half;
			frame_size  = 4 * rate;
			break;
		case 0x40:
			*samplerate = 44100 >> half;
			frame_size  = 2 * (320 * rate / 147 + (frmsizecod & 1));
			break;
		case 0x80:
			*samplerate = 32000 >> half;
			frame_size  = 6 * rate;
			break;
		default:
			return 0;
	}

	unsigned char acmod    = buf[6] >> 5;
	unsigned char flags = ((((buf[6] & 0xf8) == 0x50) ? AC3_DOLBY : acmod) | ((buf[6] & lfeon[acmod]) ? AC3_LFE : 0));
	switch (flags & AC3_CHANNEL_MASK) {
		case AC3_MONO:
			*channels = 1;
			break;
		case AC3_CHANNEL:
		case AC3_STEREO:
		case AC3_CHANNEL1:
		case AC3_CHANNEL2:
		case AC3_DOLBY:
			*channels = 2;
			break;
		case AC3_2F1R:
		case AC3_3F:
			*channels = 3;
			break;
		case AC3_3F1R:
		case AC3_2F2R:
			*channels = 4;
			break;
		case AC3_3F2R:
			*channels = 5;
			break;
	}
	if (flags & AC3_LFE) (*channels)++;

	*framelength = 1536;
	return frame_size;
}

int ParseEAC3Header(const BYTE *buf, int *samplerate, int *channels, int *framelength, int *frametype)
{
	if (*(WORD*)buf != AC3_SYNC_WORD) // syncword
		return 0;

	if (buf[5] >> 3 <= 10)   // bsid
		return 0;

	static const int sample_rates[] = { 48000, 44100, 32000, 24000, 22050, 16000 };
	static const int channels_tbl[]     = { 2, 1, 2, 3, 3, 4, 4, 5 };
	static const int samples_tbl[]      = { 256, 512, 768, 1536 };

	int frame_size = (((buf[2] & 0x03) << 8) + buf[3] + 1) * 2;

	int fscod  =  buf[4] >> 6;
	int fscod2 = (buf[4] >> 4) & 0x03;

	if (fscod == 0x03 && fscod2 == 0x03)
		return 0;

	int acmod = (buf[4] >> 1) & 0x07;
	int lfeon =  buf[4] & 0x01;

	*frametype    = (buf[2] >> 6) & 0x03;
	if (*frametype == EAC3_FRAME_TYPE_RESERVED)
		return 0;
	//int sub_stream_id = (buf[2] >> 3) & 0x07;
	*samplerate       = sample_rates[fscod == 0x03 ? 3 + fscod2 : fscod];
	*channels         = channels_tbl[acmod] + lfeon;
	*framelength      = (fscod == 0x03) ? 1536 : samples_tbl[fscod2];

	return frame_size;
}

int ParseMLPHeader(const BYTE *buf, int *samplerate, int *channels, int *framelength, bool *isTrueHD)
{
	static const int sampling_rates[]           = { 48000, 96000, 192000, 0, 0, 0, 0, 0, 44100, 88200, 176400, 0, 0, 0, 0, 0 };
	static const unsigned char mlp_channels[32] = {     1,     2,      3, 4, 3, 4, 5, 3,     4,     5,      4, 5, 6, 4, 5, 4,
	                                                    5,     6,      5, 5, 6, 0, 0, 0,     0,     0,      0, 0, 0, 0, 0, 0 };
	static const int channel_count[13] = {//   LR    C   LFE  LRs LRvh  LRc LRrs  Cs   Ts  LRsd  LRw  Cvh  LFE2
	                                            2,   1,   1,   2,   2,   2,   2,   1,   1,   2,   2,   1,   1
	};

	DWORD sync = *(DWORD*)(buf+4);
	if (sync == TRUEHD_SYNC_WORD) {
		*isTrueHD = true;
	} else if (sync == MLP_SYNC_WORD) {
		*isTrueHD = false;
	} else {
		return 0;
	}

	int frame_size  = (((buf[0] << 8) | buf[1]) & 0xfff) * 2;

	if (isTrueHD) {
		*samplerate             = sampling_rates[buf[8] >> 4];
		*framelength            = 40 << ((buf[8] >> 4) & 0x07);
		int chanmap_substream_1 = ((buf[ 9] & 0x0f) << 1) | (buf[10] >> 7);
		int chanmap_substream_2 = ((buf[10] & 0x1f) << 8) |  buf[11];
		int channel_map         = chanmap_substream_2 ? chanmap_substream_2 : chanmap_substream_1;
		*channels = 0;
		for (int i = 0; i < 13; ++i)
			*channels += channel_count[i] * ((channel_map >> i) & 1);
	} else {
		*samplerate  = sampling_rates[buf[9] >> 4];
		*framelength = 40 << ((buf[9] >> 4) & 0x07);
		*channels    = mlp_channels[buf[11] & 0x1f];
	}

	return frame_size;
}

int ParseHdmvLPCMHeader(const BYTE *buf, int *samplerate, int *channels)
{
	*samplerate = 0;
	*channels   = 0;

	int frame_size = buf[0] << 8 | buf[1];
	frame_size += 4; // add header size;

	static int channels_layout[] = {0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8, 0, 0, 0, 0};
	BYTE channel_layout = buf[2] >> 4;
	*channels           = channels_layout[channel_layout];
	if (!*channels) {
		return 0;
	}

	static int bitspersample[] = {0, 16, 20, 24};
	int bits_per_sample = bitspersample[buf[3] >> 6];
	if (!(bits_per_sample == 16 || bits_per_sample == 24)) {
		return 0;
	}

	static int freq[] = {0, 48000, 0, 0, 96000, 192000};
	*samplerate = freq[buf[2] & 0x0f];
	if (!(*samplerate == 48000 || *samplerate == 96000 || *samplerate == 192000)) {
		return 0;
	}

	return frame_size;
}

inline UINT64 LatmGetValue(CGolombBuffer gb) {
	int length = gb.BitRead(2);
	UINT64 value = 0;

	for (int i=0; i<=length; i++) {
		value <<= 8;
		value |= gb.BitRead(8);
	}

	return value;
}

bool ReadAudioConfig(CGolombBuffer gb, int* samplingFrequency, int* channelConfiguration)
{
	static int channels_layout[] = {0, 1, 2, 3, 4, 5, 6, 8};
	static int freq[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

	int sbr_present = -1;

	int audioObjectType = gb.BitRead(5);
	if (audioObjectType == 31) {
		audioObjectType = 32 + gb.BitRead(6);
	}
	int samplingFrequencyIndex = gb.BitRead(4);
	*samplingFrequency = 0;
	*samplingFrequency = freq[samplingFrequencyIndex];
	if (samplingFrequencyIndex == 0x0f) {
		*samplingFrequency = gb.BitRead(24);
	}
	*channelConfiguration = 0;
	int channelconfig = gb.BitRead(4);
	if (channelconfig < 8) {
		*channelConfiguration = channels_layout[channelconfig];
	}

	if (audioObjectType == 5) {
		sbr_present = 1;

		samplingFrequencyIndex = gb.BitRead(4);
		*samplingFrequency = freq[samplingFrequencyIndex];
		if (samplingFrequencyIndex == 0x0f) {
			*samplingFrequency = gb.BitRead(24);
		}

		audioObjectType = gb.BitRead(5);
		if (audioObjectType == 31) {
			audioObjectType = 32 + gb.BitRead(6);
		}

		if (audioObjectType == 22) {
			gb.BitRead(4); // ext_chan_config
		}
	}

	if (sbr_present == -1) {
		if (*samplingFrequency <= 24000) {
			*samplingFrequency *= 2;
		}			
	}

	return true;
}

bool StreamMuxConfig(CGolombBuffer gb, int* samplingFrequency, int* channelConfiguration, int* nExtraPos)
{
	*nExtraPos = 0;

	BYTE audio_mux_version_A = 0;
	BYTE audio_mux_version = gb.BitRead(1);
	if (audio_mux_version == 1) {
		audio_mux_version_A = gb.BitRead(1);
	}

	if (!audio_mux_version_A) {
		if (audio_mux_version == 1) {
			LatmGetValue(gb); // taraFullness
		}
		gb.BitRead(1); // all_same_framing
		gb.BitRead(6); // numSubFrames
		gb.BitRead(4); // numProgram
		gb.BitRead(3); //int numLayer

		if (!audio_mux_version) {
			// audio specific config.
			*nExtraPos = gb.GetPos();
			return ReadAudioConfig(gb, samplingFrequency, channelConfiguration);
		}
	} else {
		return false;
	}

	return true;
}

bool ParseAACLatmHeader(const BYTE *buf, int len, int *samplerate, int *channels, BYTE *extra, unsigned int* extralen)
{
	CGolombBuffer gb((BYTE* )buf, len);

	if (gb.BitRead(11) != 0x2b7) {
		return false;
	}

	*samplerate	= 0;
	*channels	= 0;
	if (extralen) {
		*extralen	= 0;
	}

	int nExtraPos = 0;

	gb.BitRead(13); // muxlength
	BYTE use_same_mux = gb.BitRead(1);
	if (!use_same_mux) {
		bool ret = StreamMuxConfig(gb, samplerate, channels, &nExtraPos);
		if (!ret) {
			return ret;
		}
	} else {
		return false;
	}

	if (*samplerate > 96000 || (*channels < 1 || *channels > 7)) {
		return false;
	}

	if (extralen && extra && nExtraPos) {
		*extralen = 4; // max size of extradata ... TODO - calculate/detect right extralen.
		gb.Reset();
		gb.SkipBytes(nExtraPos);
		gb.ReadBuffer(extra, 4);
	}

	return true;
}