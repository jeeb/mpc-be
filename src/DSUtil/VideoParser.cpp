/*
 * $Id$
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

#include "stdafx.h"
#include "VideoParser.h"

boolean ParseDiracHeader(CGolombBuffer gb, unsigned* width, unsigned* height, REFERENCE_TIME* AvgTimePerFrame)
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