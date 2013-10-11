/*
 * Copyright (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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

#include <atlimage.h>
#include <libpng/png.h>

struct png_t {
	unsigned char* data;
	png_size_t size, pos;
};

static void read_data_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct png_t* png = (struct png_t*)png_get_progressive_ptr(png_ptr);
	memcpy(data, &png->data[png->pos], length);
	png->pos += length;
}

class MPCPngImage : public CImage
{
public:
	bool	DecompressPNG(struct png_t* png);
	bool	LoadFromResource(UINT id);

	CString	LoadCurrentPath();
	bool	FileExists(CString& fn, bool bInclJPEG = false);

	BYTE*	BrightnessRGB(int type, BYTE* lpBits, int width, int height, int bpp, int br, int rc, int gc, int bc);
	HBITMAP	TypeLoadImage(int type, BYTE** pData, int* width, int* height, int* bpp, FILE* fp, int resid, int br, int rc, int gc, int bc);
	HBITMAP	LoadExternalImage(CString fn, int resid, int type, int br, int rc, int gc, int bc);
	void	LoadExternalGradient(CString fn, CDC* dc, CRect r, int ptop, int br, int rc, int gc, int bc);
};
