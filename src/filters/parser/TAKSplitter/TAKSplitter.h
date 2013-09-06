/*
 * $Id:
 *
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

#pragma warning(disable: 4005 4244)
extern "C" {
	#include <ffmpeg/libavutil/channel_layout.h>
}

#define TAKSplitterName L"MPC TAK Splitter"
#define TAKSourceName   L"MPC TAK Source"

static const INT64 tak_channel_layouts[] = {
	0,
	AV_CH_FRONT_LEFT,
	AV_CH_FRONT_RIGHT,
	AV_CH_FRONT_CENTER,
	AV_CH_LOW_FREQUENCY,
	AV_CH_BACK_LEFT,
	AV_CH_BACK_RIGHT,
	AV_CH_FRONT_LEFT_OF_CENTER,
	AV_CH_FRONT_RIGHT_OF_CENTER,
	AV_CH_BACK_CENTER,
	AV_CH_SIDE_LEFT,
	AV_CH_SIDE_RIGHT,
	AV_CH_TOP_CENTER,
	AV_CH_TOP_FRONT_LEFT,
	AV_CH_TOP_FRONT_CENTER,
	AV_CH_TOP_FRONT_RIGHT,
	AV_CH_TOP_BACK_LEFT,
	AV_CH_TOP_BACK_CENTER,
	AV_CH_TOP_BACK_RIGHT,
};

static const uint16_t frame_duration_type_quants[] = {
	3, 4, 6, 8, 4096, 8192, 16384, 512, 1024, 2048,
};

#define TAK_FORMAT_DATA_TYPE_BITS               3
#define TAK_FORMAT_SAMPLE_RATE_BITS            18
#define TAK_FORMAT_BPS_BITS                     5
#define TAK_FORMAT_CHANNEL_BITS                 4
#define TAK_FORMAT_VALID_BITS                   5
#define TAK_FORMAT_CH_LAYOUT_BITS               6
#define TAK_SIZE_FRAME_DURATION_BITS            4
#define TAK_SIZE_SAMPLES_NUM_BITS              35
#define TAK_LAST_FRAME_POS_BITS                40
#define TAK_LAST_FRAME_SIZE_BITS               24
#define TAK_ENCODER_CODEC_BITS                  6
#define TAK_ENCODER_PROFILE_BITS                4
#define TAK_ENCODER_VERSION_BITS               24
#define TAK_SAMPLE_RATE_MIN                  6000
#define TAK_CHANNELS_MIN                        1
#define TAK_BPS_MIN                             8
#define TAK_FRAME_HEADER_FLAGS_BITS             3
#define TAK_FRAME_HEADER_SYNC_ID           0xA0FF
#define TAK_FRAME_HEADER_SYNC_ID_BITS          16
#define TAK_FRAME_HEADER_SAMPLE_COUNT_BITS     14
#define TAK_FRAME_HEADER_NO_BITS               21
#define TAK_FRAME_DURATION_QUANT_SHIFT          5
#define TAK_CRC24_BITS                         24


#define TAK_FRAME_FLAG_IS_LAST                0x1
#define TAK_FRAME_FLAG_HAS_INFO               0x2
#define TAK_FRAME_FLAG_HAS_METADATA           0x4

#define TAK_MAX_CHANNELS               (1 << TAK_FORMAT_CHANNEL_BITS)

#define TAK_MIN_FRAME_HEADER_BITS      (TAK_FRAME_HEADER_SYNC_ID_BITS + \
										TAK_FRAME_HEADER_FLAGS_BITS   + \
										TAK_FRAME_HEADER_NO_BITS      + \
										TAK_CRC24_BITS)

#define TAK_MIN_FRAME_HEADER_LAST_BITS (TAK_MIN_FRAME_HEADER_BITS + 2 + \
										TAK_FRAME_HEADER_SAMPLE_COUNT_BITS)

#define TAK_ENCODER_BITS               (TAK_ENCODER_CODEC_BITS + \
										TAK_ENCODER_PROFILE_BITS)

#define TAK_SIZE_BITS                  (TAK_SIZE_SAMPLES_NUM_BITS + \
										TAK_SIZE_FRAME_DURATION_BITS)

#define TAK_FORMAT_BITS                (TAK_FORMAT_DATA_TYPE_BITS   + \
										TAK_FORMAT_SAMPLE_RATE_BITS + \
										TAK_FORMAT_BPS_BITS         + \
										TAK_FORMAT_CHANNEL_BITS + 1 + \
										TAK_FORMAT_VALID_BITS   + 1 + \
										TAK_FORMAT_CH_LAYOUT_BITS   * \
										TAK_MAX_CHANNELS)

#define TAK_STREAMINFO_BITS            (TAK_ENCODER_BITS + \
										TAK_SIZE_BITS    + \
										TAK_FORMAT_BITS)

#define TAK_MAX_FRAME_HEADER_BITS      (TAK_MIN_FRAME_HEADER_LAST_BITS + \
										TAK_STREAMINFO_BITS + 31)

#define TAK_STREAMINFO_BYTES           ((TAK_STREAMINFO_BITS       + 7) / 8)
#define TAK_MAX_FRAME_HEADER_BYTES     ((TAK_MAX_FRAME_HEADER_BITS + 7) / 8)
#define TAK_MIN_FRAME_HEADER_BYTES     ((TAK_MIN_FRAME_HEADER_BITS + 7) / 8)


enum TAKCodecType {
	TAK_CODEC_MONO_STEREO  = 2,
	TAK_CODEC_MULTICHANNEL = 4,
};

enum TAKMetaDataType {
	TAK_METADATA_END = 0,
	TAK_METADATA_STREAMINFO,
	TAK_METADATA_SEEKTABLE,
	TAK_METADATA_SIMPLE_WAVE_DATA,
	TAK_METADATA_ENCODER,
	TAK_METADATA_PADDING,
	TAK_METADATA_MD5,
	TAK_METADATA_LAST_FRAME,
};

enum TAKFrameSizeType {
	TAK_FST_94ms = 0,
	TAK_FST_125ms,
	TAK_FST_188ms,
	TAK_FST_250ms,
	TAK_FST_4096,
	TAK_FST_8192,
	TAK_FST_16384,
	TAK_FST_512,
	TAK_FST_1024,
	TAK_FST_2048,
};

typedef struct TAKStreamInfo {
	int					flags;
	enum TAKCodecType	codec;
	int					data_type;
	int					sample_rate;
	int					channels;
	int					bps;
	int					frame_num;
	int					frame_samples;
	int					last_frame_samples;
	UINT64				ch_layout;
	INT64				samples;
} TAKStreamInfo;

class __declspec(uuid("AA04C78C-3671-43F6-ABFE-6C265BAB2345"))
	CTAKSplitterFilter : public CBaseSplitterFilter
{
	REFERENCE_TIME m_rtStart;
	DWORD m_nAvgBytesPerSec;

	__int64 m_endpos;

protected:
	CAutoPtr<CBaseSplitterFileEx> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	void ParseTAKStreamInfo(CGolombBuffer gb, TAKStreamInfo& si);

public:
	CTAKSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("A98DC1BA-E70D-47A0-BAD1-28C64A859FB1"))
	CTAKSourceFilter : public CTAKSplitterFilter
{
public:
	CTAKSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};