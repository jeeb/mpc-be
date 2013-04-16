/*
 * $Id$
 *
 * (C) 2011-2013 see Authors.txt
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

#include <WinDef.h>

#define RIFF_DWORD          0x46464952

#define AC3_SYNC_WORD           0x770b
#define AAC_LATM_SYNC_WORD       0x2b7
#define TRUEHD_SYNC_WORD    0xba6f72f8
#define MLP_SYNC_WORD       0xbb6f72f8
#define IEC61937_SYNC_WORD  0x4e1ff872

#define DTS_SYNC_WORD       0x0180fe7f
#define DTSHD_SYNC_WORD     0x25205864

#define EAC3_FRAME_TYPE_INDEPENDENT  0
#define EAC3_FRAME_TYPE_DEPENDENT    1
#define EAC3_FRAME_TYPE_AC3_CONVERT  2
#define EAC3_FRAME_TYPE_RESERVED     3

int GetAC3FrameSize  (const BYTE* buf); // AC3
int GetEAC3FrameSize (const BYTE* buf); // E-AC3
int GetMLPFrameSize  (const BYTE* buf); // TrueHD and MLP
int GetDTSFrameSize  (const BYTE* buf); // DTS
int GetDTSHDFrameSize(const BYTE* buf); // DTS-HD
int GetADTSFrameSize (const BYTE* buf, int* headersize);

int ParseAC3Header     (const BYTE* buf, int* samplerate, int* channels, int* framelength, int* bitrate);
int ParseEAC3Header    (const BYTE* buf, int* samplerate, int* channels, int* framelength, int* frametype);
int ParseMLPHeader     (const BYTE* buf, int* samplerate, int* channels, int* framelength, WORD* bitdepth, bool* isTrueHD); // TrueHD and MLP
int ParseDTSHeader     (const BYTE* buf, int* samplerate, int* channels, int* framelength, int* tr_bitrate);
int ParseHdmvLPCMHeader(const BYTE* buf, int* samplerate, int* channels);
int ParseADTSAACHeader (const BYTE* buf, int* samplerate, int* channels, int* framelength, int* headersize);
bool ParseAACLatmHeader(const BYTE* buf, int len, int* samplerate, int* channels, BYTE* extra, unsigned int* extralen);

DWORD GetDefChannelMask(WORD nChannels);
DWORD GetVorbisChannelMask(WORD nChannels);
