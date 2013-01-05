/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
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
#include <memory.h>
#include "BitBltFunction.h"

bool BitBltFromP016ToP016(size_t w, size_t h, BYTE* dstY, BYTE* dstUV, int dstPitch, BYTE* srcY, BYTE* srcUV, int srcPitch)
{
	// Copy Y plane
	for (size_t row = 0; row < h; row++)
	{
		BYTE* src = srcY + row * srcPitch;
		BYTE* dst = dstY + row * dstPitch;
		
		memcpy(dst, src, dstPitch);
	}

	// Copy UV plane. UV plane is half height.
	for (size_t row = 0; row < h/2; row++)
	{
		BYTE* src = srcUV + row * srcPitch;
		BYTE* dst = dstUV + row * dstPitch;

		memcpy(dst, src, dstPitch);
	}

	return true;
}
