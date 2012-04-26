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
#include <malloc.h>
#include <afxpriv.h>
#include "PngImage.h"
#include <libpng/png.h>
#include <libpng/pnginfo.h>


struct png_t {
	unsigned char* data;
	png_size_t size, pos;
};

static void read_data_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct png_t* png = (struct png_t*)png_get_progressive_ptr(png_ptr);
	if (png->pos + length > png->size) {
		png_error(png_ptr, "Read Error");
	}
	memcpy(data, &png->data[png->pos], length);
	png->pos += length;
}

static unsigned char* DecompressPNG(struct png_t* png, int* w, int* h)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;

	unsigned char* pic;
	unsigned char* row;
	unsigned int x, y, c;

	if (png_sig_cmp(png->data, 0, 8) != 0) {
		return NULL;
	}

	png->pos = 8;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	// (png_voidp)user_error_ptr, user_error_fn, user_warning_fn);
	if (!png_ptr) {
		return NULL;
	}

	png_set_read_fn(png_ptr, (png_voidp)png, read_data_fn);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return NULL;
	}

	end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return NULL;
	}

#pragma warning(disable:4611)
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return NULL;
	}
#pragma warning(default:4611)

	png_set_sig_bytes(png_ptr, 8);

	png_read_png(
		png_ptr, info_ptr,
		PNG_TRANSFORM_STRIP_16 |
		PNG_TRANSFORM_STRIP_ALPHA |
		PNG_TRANSFORM_PACKING |
		PNG_TRANSFORM_EXPAND |
		PNG_TRANSFORM_BGR,
		NULL);

	if (png_get_channels(png_ptr, info_ptr) != 3) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return NULL;
	}

	pic = (unsigned char*)calloc(info_ptr->width * info_ptr->height, 4);

	*w = info_ptr->width;
	*h = info_ptr->height;

	for (y = 0; y < info_ptr->height; y++) {
		row = &pic[y * info_ptr->width * 4];

		for (x = 0; x < info_ptr->width*3; row += 4) {
			for (c = 0; c < 3; c++) {
				row[c] = info_ptr->row_pointers[y][x++];
			}
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	return pic;
}

bool CPngImage::LoadFromResource(UINT id) {
	bool ret = false;

	CStringA str;
	if (LoadResource(id, str, _T("FILE"))) {
		struct png_t png;
		png.data = (unsigned char*)(LPCSTR)str;
		png.size = str.GetLength();
		int w, h;
		if (BYTE* p = DecompressPNG(&png, &w, &h)) {
			if (Create(w, -h, 32)) {
				for (int y = 0; y < h; y++) {
					memcpy(GetPixelAddress(0, y), &p[w*4*y], w*4);
				}
				ret = true;
			}

			free(p);
		}
	}

	return ret;
}

CString CPngImage::LoadCurrentPath()
{
	CString path;
	GetModuleFileName(AfxGetInstanceHandle(), path.GetBuffer(_MAX_PATH), _MAX_PATH);
	path.ReleaseBuffer();
	return path.Left(path.ReverseFind('\\') + 1);
}

int CPngImage::FileExists(CString fn)
{
	FILE* fp = _tfopen(LoadCurrentPath() + fn + _T(".png"), _T("rb"));
	if (fp) {
		fclose(fp);
		return 1;
	} else {
		return NULL;
	}
}

HBITMAP CPngImage::LoadExternalImage(CString fn)
{
	CString path = LoadCurrentPath();

	FILE* fp = _tfopen(path + fn + _T(".png"), _T("rb"));
	if (fp) {
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR | PNG_TRANSFORM_PACKING, 0);

		png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
		int bpp = png_get_channels(png_ptr, info_ptr) * 8;
		int width = png_get_image_width(png_ptr, info_ptr);
		int memWidth = width * bpp / 8;
		int height = png_get_image_height(png_ptr, info_ptr);
		BYTE* pData;
		BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), width, -height, 1, bpp, BI_RGB, 0, 0, 0, 0, 0}};

		HBITMAP hbm = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&pData, 0, 0);
		for (int i = 0; i < height; i++) {
			memcpy(pData + memWidth * i, row_pointers[i], memWidth);
		}

		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose(fp);

		return hbm;
	} else {
		return (HBITMAP)LoadImage(NULL, path + fn + _T(".bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	}
}

void CPngImage::LoadExternalGradient(CString fn, CDC* dc, CRect r, int ptop)
{
	FILE* fp = _tfopen(LoadCurrentPath() + fn + _T(".png"), _T("rb"));
	if (fp) {
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR | PNG_TRANSFORM_PACKING, 0);

		png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
		int bpp = png_get_channels(png_ptr, info_ptr) * 8;
		int width = png_get_image_width(png_ptr, info_ptr);
		int memWidth = width * bpp / 8;
		int height = png_get_image_height(png_ptr, info_ptr);
		BYTE* pData;
		BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), width, -height, 1, bpp, BI_RGB, 0, 0, 0, 0, 0}};

		HBITMAP hbm = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&pData, 0, 0);
		for (int i = 0; i < height; i++) {
			memcpy(pData + memWidth * i, row_pointers[i], memWidth);
		}

		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose(fp);

		GRADIENT_RECT gr[1] = {{0, 1}};

		int bt = bpp / 8, st = 2, pa = 255 * 256;
		int sp = bt * ptop, hs = bt * st, sz = width * height * bt;

		for (int k = sp, t = 0; t < r.bottom; k += hs, t += st) {
			TRIVERTEX tv[2] = {
				{r.left, t, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
				{r.right, t + st, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
			};
			dc->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
		}
	}
}
