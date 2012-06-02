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

#pragma once

#include "DSUtil.h"
#include "GolombBuffer.h"

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

boolean ParseDiracHeader(CGolombBuffer gb, unsigned* width, unsigned* height, REFERENCE_TIME* AvgTimePerFrame);