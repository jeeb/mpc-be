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
#include "VideoParser.h"

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
