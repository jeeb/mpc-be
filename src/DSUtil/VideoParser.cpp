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
#include "VideoParser.h"
#include "NALBitstream.h"

#define REF_SECOND_MULT 10000000LL

struct dirac_source_params {
	unsigned width;
	unsigned height;
	WORD chroma_format;          ///< 0: 444  1: 422  2: 420

	BYTE interlaced;
	BYTE top_field_first;

	BYTE frame_rate_index;       ///< index into dirac_frame_rate[]
	BYTE aspect_ratio_index;     ///< index into dirac_aspect_ratio[]

	WORD clean_width;
	WORD clean_height;
	WORD clean_left_offset;
	WORD clean_right_offset;

	BYTE pixel_range_index;      ///< index into dirac_pixel_range_presets[]
	BYTE color_spec_index;       ///< index into dirac_color_spec_presets[]
};

static const dirac_source_params dirac_source_parameters_defaults[] = {
	{ 640,  480,  2, 0, 0, 1,  1, 640,  480,  0, 0, 1, 0 },
	{ 176,  120,  2, 0, 0, 9,  2, 176,  120,  0, 0, 1, 1 },
	{ 176,  144,  2, 0, 1, 10, 3, 176,  144,  0, 0, 1, 2 },
	{ 352,  240,  2, 0, 0, 9,  2, 352,  240,  0, 0, 1, 1 },
	{ 352,  288,  2, 0, 1, 10, 3, 352,  288,  0, 0, 1, 2 },
	{ 704,  480,  2, 0, 0, 9,  2, 704,  480,  0, 0, 1, 1 },
	{ 704,  576,  2, 0, 1, 10, 3, 704,  576,  0, 0, 1, 2 },
	{ 720,  480,  1, 1, 0, 4,  2, 704,  480,  8, 0, 3, 1 },
	{ 720,  576,  1, 1, 1, 3,  3, 704,  576,  8, 0, 3, 2 },
	{ 1280, 720,  1, 0, 1, 7,  1, 1280, 720,  0, 0, 3, 3 },
	{ 1280, 720,  1, 0, 1, 6,  1, 1280, 720,  0, 0, 3, 3 },
	{ 1920, 1080, 1, 1, 1, 4,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 1920, 1080, 1, 1, 1, 3,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 1920, 1080, 1, 0, 1, 7,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 1920, 1080, 1, 0, 1, 6,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 2048, 1080, 0, 0, 1, 2,  1, 2048, 1080, 0, 0, 4, 4 },
	{ 4096, 2160, 0, 0, 1, 2,  1, 4096, 2160, 0, 0, 4, 4 },
	{ 3840, 2160, 1, 0, 1, 7,  1, 3840, 2160, 0, 0, 3, 3 },
	{ 3840, 2160, 1, 0, 1, 6,  1, 3840, 2160, 0, 0, 3, 3 },
	{ 7680, 4320, 1, 0, 1, 7,  1, 3840, 2160, 0, 0, 3, 3 },
	{ 7680, 4320, 1, 0, 1, 6,  1, 3840, 2160, 0, 0, 3, 3 },
};

static const AV_Rational avpriv_frame_rate_tab[16] = {
	{    0,    0},
	{24000, 1001},
	{   24,    1},
	{   25,    1},
	{30000, 1001},
	{   30,    1},
	{   50,    1},
	{60000, 1001},
	{   60,    1},
	// Xing's 15fps: (9)
	{   15,    1},
	// libmpeg3's "Unofficial economy rates": (10-13)
	{    5,    1},
	{   10,    1},
	{   12,    1},
	{   15,    1},
	{    0,    0},
};

static const AV_Rational dirac_frame_rate[] = {
	{15000, 1001},
	{25, 2},
};

bool ParseDiracHeader(CGolombBuffer gb, unsigned* width, unsigned* height, REFERENCE_TIME* AvgTimePerFrame)
{
	unsigned int version_major = gb.UintGolombRead();
	if (version_major < 2) {
		return false;
	}
	gb.UintGolombRead(); /* version_minor */
	gb.UintGolombRead(); /* profile */
	gb.UintGolombRead(); /* level */
	unsigned int video_format = gb.UintGolombRead();

	dirac_source_params source = dirac_source_parameters_defaults[video_format];

	if (gb.BitRead(1)) {
		source.width	= gb.UintGolombRead();
		source.height	= gb.UintGolombRead();
	}
	if (!source.width || !source.height) {
		return false;
	}

	if (gb.BitRead(1)) {
		source.chroma_format = gb.UintGolombRead();
	}
	if (source.chroma_format > 2) {
		return false;
	}

	if (gb.BitRead(1)) {
		source.interlaced = gb.UintGolombRead();
	}
	if (source.interlaced > 1) {
		return false;
	}

	AV_Rational frame_rate = {0,0};
	if (gb.BitRead(1)) {
		source.frame_rate_index = gb.UintGolombRead();
		if (source.frame_rate_index > 10) {
			return false;
		}
		if (!source.frame_rate_index) {
			frame_rate.num = gb.UintGolombRead();
			frame_rate.den = gb.UintGolombRead();
		}
	}
	if (source.frame_rate_index > 0) {
		if (source.frame_rate_index <= 8) {
			frame_rate = avpriv_frame_rate_tab[source.frame_rate_index];
		} else {
			frame_rate = dirac_frame_rate[source.frame_rate_index-9];
		}
	}
	if (!frame_rate.num || !frame_rate.den) {
		return false;
	}

	*width				= source.width;
	*height				= source.height;
	*AvgTimePerFrame	= REF_SECOND_MULT * frame_rate.den/frame_rate.num;

	return true;
}

int HrdParameters(CGolombBuffer& gb)
{
	UINT64 cnt = gb.UExpGolombRead();	// cpb_cnt_minus1
	if (cnt > 32U) {
		return -1;
	}
	gb.BitRead(4);							// bit_rate_scale
	gb.BitRead(4);							// cpb_size_scale

	for (unsigned int i = 0; i <= cnt; i++ ) {
		gb.UExpGolombRead();				// bit_rate_value_minus1
		gb.UExpGolombRead();				// cpb_size_value_minus1
		gb.BitRead(1);						// cbr_flag
	}

	gb.BitRead(5);							// initial_cpb_removal_delay_length_minus1
	gb.BitRead(5);							// cpb_removal_delay_length_minus1
	gb.BitRead(5);							// dpb_output_delay_length_minus1
	gb.BitRead(5);							// time_offset_length

	return 0;
}

bool ParseAVCHeader(CGolombBuffer gb, avc_hdr& h, bool fullscan)
{
	static BYTE profiles[] = {44, 66, 77, 88, 100, 110, 118, 122, 128, 144, 244};
	static BYTE levels[] = {10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51};

	memset((void*)&h, 0, sizeof(h));

	h.profile = (BYTE)gb.BitRead(8);
	bool b_ident = false;
	for (int i = 0; i<sizeof(profiles); i++) {
		if (h.profile == profiles[i]) {
			b_ident = true;
			break;
		}
	}
	if (!b_ident)
		return false;

	gb.BitRead(8);
	h.level = (BYTE)gb.BitRead(8);
	b_ident = false;
	for (int i = 0; i<sizeof(levels); i++) {
		if (h.level == levels[i]) {
			b_ident = true;
			break;
		}
	}
	if (!b_ident)
		return false;

	UINT64 sps_id = gb.UExpGolombRead();	// seq_parameter_set_id
	if (sps_id >= 32) {
		return false;
	}

	UINT64 chroma_format_idc = 0;
	if (h.profile >= 100) {					// high profile
		chroma_format_idc = gb.UExpGolombRead();
		if (chroma_format_idc == 3) {		// chroma_format_idc
			gb.BitRead(1);					// residue_transform_flag
		}

		gb.UExpGolombRead();				// bit_depth_luma_minus8
		gb.UExpGolombRead();				// bit_depth_chroma_minus8

		gb.BitRead(1);						// qpprime_y_zero_transform_bypass_flag

		if (gb.BitRead(1)) {				// seq_scaling_matrix_present_flag
			for (int i = 0; i < 8; i++) {
				if (gb.BitRead(1)) {		// seq_scaling_list_present_flag
					for (int j = 0, size = i < 6 ? 16 : 64, next = 8; j < size && next != 0; ++j) {
						next = (next + gb.SExpGolombRead() + 256) & 255;
					}
				}
			}
		}
	}

	gb.UExpGolombRead();					// log2_max_frame_num_minus4

	UINT64 pic_order_cnt_type = gb.UExpGolombRead();

	if (pic_order_cnt_type == 0) {
		gb.UExpGolombRead();				// log2_max_pic_order_cnt_lsb_minus4
	} else if (pic_order_cnt_type == 1) {
		gb.BitRead(1);						// delta_pic_order_always_zero_flag
		gb.SExpGolombRead();				// offset_for_non_ref_pic
		gb.SExpGolombRead();				// offset_for_top_to_bottom_field
		UINT64 num_ref_frames_in_pic_order_cnt_cycle = gb.UExpGolombRead();
		if (num_ref_frames_in_pic_order_cnt_cycle >= 256) {
			return false;
		}
		for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
			gb.SExpGolombRead();			// offset_for_ref_frame[i]
		}
	} else if (pic_order_cnt_type != 2) {
		return false;
	}

	UINT64 ref_frame_count = gb.UExpGolombRead();	// num_ref_frames
	if (ref_frame_count > 30) {
		return false;
	}
	gb.BitRead(1);									// gaps_in_frame_num_value_allowed_flag

	UINT64 pic_width_in_mbs_minus1 = gb.UExpGolombRead();
	UINT64 pic_height_in_map_units_minus1 = gb.UExpGolombRead();
	h.interlaced = !(BYTE)gb.BitRead(1);

	if (h.interlaced) {
		gb.BitRead(1);								// mb_adaptive_frame_field_flag
	}

	BYTE direct_8x8_inference_flag = (BYTE)gb.BitRead(1); // direct_8x8_inference_flag
	if (h.interlaced && !direct_8x8_inference_flag) {
		return false;
	}

	if (gb.BitRead(1)) {									// frame_cropping_flag
		h.crop_left   = (unsigned int)gb.UExpGolombRead();	// frame_cropping_rect_left_offset
		h.crop_right  = (unsigned int)gb.UExpGolombRead();	// frame_cropping_rect_right_offset
		h.crop_top    = (unsigned int)gb.UExpGolombRead();	// frame_cropping_rect_top_offset
		h.crop_bottom = (unsigned int)gb.UExpGolombRead();	// frame_cropping_rect_bottom_offset
	}

	if (gb.BitRead(1)) {							// vui_parameters_present_flag
		if (gb.BitRead(1)) {						// aspect_ratio_info_present_flag
			BYTE aspect_ratio_idc = (BYTE)gb.BitRead(8); // aspect_ratio_idc
			if (255 == aspect_ratio_idc) {
				h.sar.num = (WORD)gb.BitRead(16);	// sar_width
				h.sar.den = (WORD)gb.BitRead(16);	// sar_height
			} else if (aspect_ratio_idc < 17) {
				h.sar.num = pixel_aspect[aspect_ratio_idc][0];
				h.sar.den = pixel_aspect[aspect_ratio_idc][1];
			} else {
				return false;
			}
		} else {
			h.sar.num = 1;
			h.sar.den = 1;
		}

		if (gb.BitRead(1)) {				// overscan_info_present_flag
			gb.BitRead(1);					// overscan_appropriate_flag
		}

		if (gb.BitRead(1)) {				// video_signal_type_present_flag
			gb.BitRead(3);					// video_format
			gb.BitRead(1);					// video_full_range_flag
			if (gb.BitRead(1)) {			// colour_description_present_flag
				gb.BitRead(8);				// colour_primaries
				gb.BitRead(8);				// transfer_characteristics
				gb.BitRead(8);				// matrix_coefficients
			}
		}
		if (gb.BitRead(1)) {				// chroma_location_info_present_flag
			gb.UExpGolombRead();			// chroma_sample_loc_type_top_field
			gb.UExpGolombRead();			// chroma_sample_loc_type_bottom_field
		}
		if (gb.BitRead(1)) {				// timing_info_present_flag
			__int64 num_units_in_tick	= gb.BitRead(32);
			__int64 time_scale			= gb.BitRead(32);
			/*long fixed_frame_rate_flag	= */gb.BitRead(1);

			// Trick for weird parameters
			if ((num_units_in_tick < 1000) || (num_units_in_tick > 1001)) {
				if ((time_scale % num_units_in_tick != 0) && ((time_scale*1001) % num_units_in_tick == 0)) {
					time_scale			= (time_scale * 1001) / num_units_in_tick;
					num_units_in_tick	= 1001;
				} else {
					time_scale			= (time_scale * 1000) / num_units_in_tick;
					num_units_in_tick	= 1000;
				}
			}
			time_scale = time_scale / 2;	// VUI consider fields even for progressive stream : divide by 2!

			if (time_scale) {
				h.AvgTimePerFrame = (10000000I64*num_units_in_tick)/time_scale;
			}
		}

		if (fullscan) {
			bool nalflag = !!gb.BitRead(1);		// nal_hrd_parameters_present_flag
			if (nalflag) {
				if (HrdParameters(gb)<0) {
					return false;
				}
			}
			bool vlcflag = !!gb.BitRead(1);		// vlc_hrd_parameters_present_flag
			if (vlcflag) {
				if (HrdParameters(gb)<0) {
					return false;
				}
			}
			if (nalflag || vlcflag) {
				gb.BitRead(1);					// low_delay_hrd_flag
			}

			gb.BitRead(1);						// pic_struct_present_flag
			if (gb.BitRead(1)) {				// bitstream_restriction_flag
				gb.BitRead(1);					// motion_vectors_over_pic_boundaries_flag
				gb.UExpGolombRead();			// max_bytes_per_pic_denom
				gb.UExpGolombRead();			// max_bits_per_mb_denom
				gb.UExpGolombRead();			// log2_max_mv_length_horizontal
				gb.UExpGolombRead();			// log2_max_mv_length_vertical
				UINT64 num_reorder_frames = gb.UExpGolombRead(); // num_reorder_frames
				gb.UExpGolombRead();			// max_dec_frame_buffering

				if (gb.GetSize() < gb.GetPos()) {
					num_reorder_frames = 0;
				}
				if (num_reorder_frames > 16U) {
					return false;
				}
			}
		}
	}

	if (!h.sar.num) h.sar.num = 1;
	if (!h.sar.den) h.sar.den = 1;

	unsigned int mb_Width	= (unsigned int)pic_width_in_mbs_minus1 + 1;
	unsigned int mb_Height	= ((unsigned int)pic_height_in_map_units_minus1 + 1) * (2 - !h.interlaced);
	BYTE CHROMA444 = (chroma_format_idc == 3);

	h.width = 16 * mb_Width - (2u>>CHROMA444) * min(h.crop_right, (8u<<CHROMA444)-1);
	if (!h.interlaced) {
		h.height = 16 * mb_Height - (2u>>CHROMA444) * min(h.crop_bottom, (8u<<CHROMA444)-1);
	} else {
		h.height = 16 * mb_Height - (4u>>CHROMA444) * min(h.crop_bottom, (8u<<CHROMA444)-1);
	}

	if (h.height<100 || h.width<100) {
		return false;
	}

	if (h.height == 1088) {
		h.height = 1080;	// Prevent blur lines
	}

	return true;
}

bool ParseHEVCHeader(BYTE* headerData, int headerSize, hevc_hdr& h)
{
	// find HEVC SPS in AVCDecoderConfigurationRecord struct
	enum hevc_nal_unit_type_e { NAL_UNIT_SPS = 33 };

	if ((headerData[5] & 0xe0) != 0xe0) { // reserved = 111b
		return false;
	}

	int sps_num = headerData[5] & 0x1f;       // numOfSequenceParameterSets
	int sps_pos = 6;
	bool has_sps = false;
	while (sps_num-- > 0) {
		int sps_len = (headerData[sps_pos] << 8) + headerData[sps_pos+1];
		sps_pos += 2;
		if (sps_pos + sps_len >= headerSize) {
			return false;
		}
		hevc_nal_unit_type_e nal_type = (hevc_nal_unit_type_e)((headerData[sps_pos] >> 1) & 0x3f);
		if (nal_type == NAL_UNIT_SPS) {
			has_sps = true;
			break;
		}
		sps_pos += sps_len;
	}
	if (!has_sps || sps_pos >= headerSize) {
		return false; // SPS not found!
	}
	
	// decode SPS
	NALBitstream bs(headerData + sps_pos, headerSize - sps_pos);
	bs.GetWord(16); // skip NAL header
	bs.GetWord(4);  // video_parameter_set_id
	int sps_max_sub_layers_minus1 = (int)bs.GetWord(3);

	bs.GetWord(1);  // sps_temporal_id_nesting_flag

	// profile_tier_level( 1, sps_max_sub_layers_minus1 )
	{
		int i, j;

		bs.GetWord(2); // XXX_profile_space[]
		bs.GetWord(1); // XXX_tier_flag[]
		h.profile = bs.GetWord(5); // XXX_profile_idc[]

		for (j = 0; j < 32; j++) {
			bs.GetWord(1);  // XXX_profile_compatibility_flag[][j]
		}
		// HM9.1
		if (h.fourcc == MAKEFOURCC('H','M','9','1')) {
			bs.GetWord(16); // XXX_reserved_zero_16bits[]
		}
		// HM10.0 / HM12.0
		else {
			bs.GetWord(1);  //(uiCode, "general_progressive_source_flag");
			bs.GetWord(1);  //(uiCode, "general_interlaced_source_flag");
			bs.GetWord(1);  //(uiCode, "general_non_packed_constraint_flag");
			bs.GetWord(1);  //(uiCode, "general_frame_only_constraint_flag");
			bs.GetWord(16); //(16, uiCode, "XXX_reserved_zero_44bits[0..15]");
			bs.GetWord(16); //(16, uiCode, "XXX_reserved_zero_44bits[16..31]");
			bs.GetWord(12); //(12, uiCode, "XXX_reserved_zero_44bits[32..43]");
		}

		h.level = bs.GetWord(8);	// general_level_idc

		// HM9.1
		if (h.fourcc == MAKEFOURCC('H','M','9','1')) {
			for (i = 0; i < sps_max_sub_layers_minus1; i++) {
				int sub_layer_profile_present_flag, sub_layer_level_present_flag;
				sub_layer_profile_present_flag = (int)bs.GetWord(1); // sub_layer_profile_present_flag[i]
				sub_layer_level_present_flag = (int)bs.GetWord(1);   // sub_layer_level_present_flag[i]
		
				if (sub_layer_profile_present_flag) {
					bs.GetWord(2); // XXX_profile_space[]
					bs.GetWord(1); // XXX_tier_flag[]
					bs.GetWord(5); // XXX_profile_idc[]
					for (j = 0; j < 32; j++) {
						bs.GetWord(1); // XXX_profile_compatibility_flag[][j]
					}
					bs.GetWord(16);    // XXX_reserved_zero_16bits[]
				}
				if (sub_layer_level_present_flag) {
					bs.GetWord(8);     // sub_layer_level_idc[i]
				}
			}
		}
		//  HM10.0 / HM12.0
		else {
			bool subLayerProfilePresentFlag[6]	= {0};
			bool subLayerLevelPresentFlag[6]	= {0};
			for (i = 0; i < sps_max_sub_layers_minus1; i++) {
				subLayerProfilePresentFlag[i] = bs.GetWord(1) != 0; //( uiCode, "sub_layer_profile_present_flag[i]" );
				subLayerLevelPresentFlag[i]   = bs.GetWord(1) != 0; //( uiCode, "sub_layer_level_present_flag[i]"   ); 
			}
			if (sps_max_sub_layers_minus1 > 0) {
				for (i = sps_max_sub_layers_minus1; i < 8; i++) {
					bs.GetWord(2); //(2, uiCode, "reserved_zero_2bits")
				}
			}
			for (i = 0; i < sps_max_sub_layers_minus1; i++) {
				if (1 && subLayerProfilePresentFlag[i]) {
					bs.GetWord(2); //( 2 , uiCode, "XXX_profile_space[]");
					bs.GetWord(1); //( uiCode, "XXX_tier_flag[]"    );
					bs.GetWord(5); //( 5 , uiCode, "XXX_profile_idc[]"  );

					for (j = 0; j < 32; j++) {
						bs.GetWord(1); //(  uiCode, "XXX_profile_compatibility_flag[][j]");
					}
					bs.GetWord(1);  //(uiCode, "general_progressive_source_flag");
					bs.GetWord(1);  //(uiCode, "general_interlaced_source_flag");
					bs.GetWord(1);  //(uiCode, "general_non_packed_constraint_flag");
					bs.GetWord(1);  //(uiCode, "general_frame_only_constraint_flag");
					bs.GetWord(16); //(16, uiCode, "XXX_reserved_zero_44bits[0..15]");
					bs.GetWord(16); //(16, uiCode, "XXX_reserved_zero_44bits[16..31]");
					bs.GetWord(12); //(12, uiCode, "XXX_reserved_zero_44bits[32..43]");
				}
				if (subLayerLevelPresentFlag[i]) {
					bs.GetWord(8);  //( 8, uiCode, "sub_layer_level_idc[i]" );
				}
			}
		}
	}

	bs.GetUE(); // seq_parameter_set_id

	int chroma_format_idc = (int)bs.GetUE(); // chroma_format_idc
	if (chroma_format_idc == 3) {
		bs.GetWord(1); // separate_colour_plane_flag
	}

	h.width  = bs.GetUE();
	h.height = bs.GetUE();

	h.sar.cx = h.width;
	h.sar.cy = h.height;
	ReduceDim(h.sar);

	h.nal_length_size = (headerData[4] & 0x03) + 1;

	return true;
}

void CreateHEVCSequenceHeader(BYTE* seq_data, int seq_size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader)
{
	BYTE* src = seq_data;
	BYTE* dst = (BYTE*)dwSequenceHeader;
	BYTE* src_end = src + seq_size;
	BYTE* dst_end = dst + seq_size;
	int spsCount = *(src++) & 0x1F;
	int ppsCount = -1;

	cbSequenceHeader = 0;

	while (src < src_end - 1) {
		if (spsCount == 0 && ppsCount == -1) {
			ppsCount = *(src++);
			continue;
		}

		if (spsCount > 0) {
			spsCount--;
		} else if (ppsCount > 0) {
			ppsCount--;
		} else {
			break;
		}

		int len = ((src[0] << 8) | src[1]) + 2;
		if (src + len > src_end || dst + len > dst_end) {
			ASSERT(0);
			break;
		}
		memcpy(dst, src, len);
		src += len;
		dst += len;
		cbSequenceHeader += len;
	}
}

////

bool ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params)
{
	if (size < 20) { // 8 + 12
		return false;
	}

	NALBitstream bs(data, size);

	// seq_parameter_set_rbsp
	bs.GetWord(4);		// sps_video_parameter_set_id
	int sps_max_sub_layers_minus1 = bs.GetWord(3);
	bs.GetWord(1);		// sps_temporal_id_nesting_flag
	// profile_tier_level( sps_max_sub_layers_minus1 )
	{
		bs.GetWord(2);	// general_profile_space
		bs.GetWord(1);	// general_tier_flag
		bs.GetWord(5);	// general_profile_idc
		bs.GetWord(32);	// general_profile_compatibility_flag[32]
		bs.GetWord(1);	// general_progressive_source_flag
		bs.GetWord(1);	// general_interlaced_source_flag
		bs.GetWord(1);	// general_non_packed_constraint_flag
		bs.GetWord(1);	// general_frame_only_constraint_flag
		bs.GetWord(44);	// general_reserved_zero_44bits
		bs.GetWord(8);	// general_level_idc
		uint8* sub_layer_profile_present_flag = new uint8[sps_max_sub_layers_minus1];
		uint8* sub_layer_level_present_flag   = new uint8[sps_max_sub_layers_minus1];
		for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = bs.GetWord(1);
			sub_layer_level_present_flag[i]   = bs.GetWord(1);
		}
		if (sps_max_sub_layers_minus1 > 0) {
			for (int i = sps_max_sub_layers_minus1; i < 8; i++) {
				uint8 reserved_zero_2bits = bs.GetWord(2);
			}
		}
		for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
			if (sub_layer_profile_present_flag[i]) {
				bs.GetWord(2);	// sub_layer_profile_space[i]
				bs.GetWord(1);	// sub_layer_tier_flag[i]
				bs.GetWord(5);	// sub_layer_profile_idc[i]
				bs.GetWord(32);	// sub_layer_profile_compatibility_flag[i][32]
				bs.GetWord(1);	// sub_layer_progressive_source_flag[i]
				bs.GetWord(1);	// sub_layer_interlaced_source_flag[i]
				bs.GetWord(1);	// sub_layer_non_packed_constraint_flag[i]
				bs.GetWord(1);	// sub_layer_frame_only_constraint_flag[i]
				bs.GetWord(44);	// sub_layer_reserved_zero_44bits[i]
			}
			if (sub_layer_level_present_flag[i]) {
				bs.GetWord(8);	// sub_layer_level_idc[i]
			}
		}
		delete[] sub_layer_profile_present_flag;
		delete[] sub_layer_level_present_flag;
	}
	uint32 sps_seq_parameter_set_id = bs.GetUE();
	uint32 chroma_format_idc        = bs.GetUE();
	if (chroma_format_idc == 3) {
		bs.GetWord(1); // separate_colour_plane_flag
	}
	params.width  = bs.GetUE();	// pic_width_in_luma_samples
	params.height = bs.GetUE();	// pic_height_in_luma_samples
	if (bs.GetWord(1)) {	// conformance_window_flag
		bs.GetUE();			// conf_win_left_offset
		bs.GetUE();			// conf_win_right_offset
		bs.GetUE();			// conf_win_top_offset
		bs.GetUE();			// conf_win_bottom_offset
	}
	uint32 bit_depth_luma_minus8   = bs.GetUE();
	uint32 bit_depth_chroma_minus8 = bs.GetUE();
	//...

	return true;
}

enum nal_unit_type_e {
	NAL_UNIT_VPS = 32,
	NAL_UNIT_SPS = 33,
};

bool ParseHEVCDecoderConfigurationRecord(BYTE* data, int size, vc_params_t& params)
{
	params.clear();
	if (size < 23) {
		return false;
	}
	CGolombBuffer gb(data, size);

	// HEVCDecoderConfigurationRecord
	if (gb.BitRead(8) != 1) {	// configurationVersion = 1
		return false;
	}
	gb.BitRead(2);					// general_profile_space
	gb.BitRead(1);					// general_tier_flag
	params.profile = gb.BitRead(5);	// general_profile_idc
	gb.BitRead(32);					// general_profile_compatibility_flags
	gb.BitRead(48);					// general_constraint_indicator_flags
	params.level   = gb.BitRead(8);	// general_level_idc
	if (gb.BitRead(4) != 15) {		// reserved = ‘1111’b
		return false;
	}
	gb.BitRead(12);				// min_spatial_segmentation_idc
	if (gb.BitRead(6) != 63) {	// reserved = ‘111111’b
		return false;
	}
	gb.BitRead(2);				// parallelismType
	if (gb.BitRead(6) != 63) {	// reserved = ‘111111’b
		return false;
	}
	uint8 chromaFormat = gb.BitRead(2); // 0 = monochrome, 1 = 4:2:0, 2 = 4:2:2, 3 = 4:4:4
	if (gb.BitRead(5) != 31) {	// reserved = ‘11111’b
		return false;
	}
	uint8 bitDepthLumaMinus8 = gb.BitRead(3);
	if (gb.BitRead(5) != 31) {	// reserved = ‘11111’b
		return false;
	}
	uint8 bitDepthChromaMinus8 = gb.BitRead(3);
	gb.BitRead(16);				// avgFrameRate
	gb.BitRead(2);				// constantFrameRate
	gb.BitRead(3);				// numTemporalLayers
	gb.BitRead(1);				// temporalIdNested
	params.nal_length_size = gb.BitRead(2) + 1;	// lengthSizeMinusOne
	int numOfArrays = gb.BitRead(8);

	int sps_num = 0;
	int sps_len = 0;

	for (int j = 0; j < numOfArrays; j++) {
		gb.BitRead(1);				// array_completeness
		if (gb.BitRead(1) != 0) {	// reserved = 0
			return false;
		}
		int NAL_unit_type = gb.BitRead(6);
		int numNalus      = gb.BitRead(16);
		if (NAL_unit_type == (int)NAL_UNIT_SPS) {
			sps_num = numNalus;
			sps_len = gb.BitRead(16);
			break;
		}
		for (int i = 0; i < numNalus; i++) {
			int nalUnitLength = gb.BitRead(16);
			gb.SkipBytes(nalUnitLength);
		}
	}

	if (!sps_num) {
		return false;
	}

	if (!ParseSequenceParameterSet(gb.GetBufferPos(), sps_len, params)) {
		return false;
	}

	return true;
}
