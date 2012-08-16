/*
 * $Id$
 *
 * Copyright (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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

	memcpy(data, &png->data[png->pos], length);

	png->pos += length;
}

bool MPCPngImage::DecompressPNG(struct png_t* png)
{
	if (png_sig_cmp(png->data, 0, 8) == 0) {
		png->pos = 8;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		return NULL;
	}

	png_set_read_fn(png_ptr, (png_voidp)png, read_data_fn);

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return NULL;
	}

	png_set_sig_bytes(png_ptr, 8);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL);

	unsigned int w = info_ptr->width, h = info_ptr->height, b = info_ptr->channels;
	unsigned int x, y, c, len = w * b;
	unsigned char *row, *pic = (unsigned char*)malloc(len * h);

	bool ret = false;

	if (Create(w, -(int)h, b * 8)) {

		for (y = 0; y < h; y++) {

			row = &pic[len * y];

			for (x = 0; x < len; row += b) {

				for (c = 0; c < b; c++) {

					row[c] = info_ptr->row_pointers[y][x++];
				}
			}

			memcpy(GetPixelAddress(0, y), &pic[len * y], len);
		}

		ret = true;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	free(pic);

	return ret;
}

bool MPCPngImage::LoadFromResource(UINT id) {
	bool ret = false;

	CStringA str;
	if (LoadResource(id, str, _T("FILE")) || LoadResource(id, str, _T("PNG"))) {

		struct png_t png;
		png.data = (unsigned char*)(LPCSTR)str;
		png.size = str.GetLength();

		ret = DecompressPNG(&png);
	}

	return ret;
}

bool MPCPngImage::LoadFromFile(CString fn) {
	bool ret = false;

	FILE* fp;
	_tfopen_s(&fp, LoadCurrentPath() + fn + _T(".png"), _T("rb"));
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		rewind(fp);
		void* str = malloc(size);
		fread(str, 1, size, fp);
		fclose(fp);

		struct png_t png;
		png.data = (unsigned char*)str;
		png.size = size;

		ret = DecompressPNG(&png);

		free(str);
	}

	return ret;
}

CString MPCPngImage::LoadCurrentPath()
{
	CString path;
	GetModuleFileName(NULL, path.GetBuffer(_MAX_PATH), _MAX_PATH);
	path.ReleaseBuffer();

	return path.Left(path.ReverseFind('\\') + 1);
}

int MPCPngImage::FileExists(CString fn)
{
	CString path = LoadCurrentPath();

	FILE* fp;
	_tfopen_s(&fp, path + fn + _T(".png"), _T("rb"));
	if (fp) {
		fclose(fp);
		return 1;
	} else {
		_tfopen_s(&fp, path + fn + _T(".bmp"), _T("rb"));
		if (fp) {
			fclose(fp);
			return 1;
		} else {
			return NULL;
		}
	}
}

BYTE* MPCPngImage::BrightnessRGB(int type, BYTE* lpBits, int width, int height, int bpp, int br, int rc, int gc, int bc)
{
	int k = bpp / 8, kbr = 100;
	int size = width * height * k;
	double R, G, B, brn, rcn, gcn, bcn;

	if (br >= 0 && rc >= 0 && gc >= 0 && bc >= 0) {

		brn = (br * 10) / (kbr + (br * 4)) * 0.8;
		rcn = rc / (70 + kbr - br) + brn;
		gcn = gc / (75 + kbr - br) + brn;
		bcn = bc / (80 + kbr - br) + brn;

		if (rcn < 0.5) {
			rcn = 0.5;
		}
		if (gcn < 0.5) {
			gcn = 0.5;
		}
		if (bcn < 0.5) {
			bcn = 0.5;
		}
	}

	for (int i = 0; i < size; i += k) {

		R = lpBits[i];
		G = lpBits[i + 1];
		B = lpBits[i + 2];
/*
		if (br >= 0 && rc >= 0 && gc >= 0 && bc >= 0) {

			R *= rcn;
			G *= gcn;
			B *= bcn;

			if (R > 255) {
				R = 255;
			}
			if (G > 255) {
				G = 255;
			}
			if (B > 255) {
				B = 255;
			}
		}
*/
		if (type == 0) {
			lpBits[i] = R;
			lpBits[i + 2] = B;
		} else if (type == 1) {
			lpBits[i] = B;
			lpBits[i + 2] = R;
		}
		lpBits[i + 1] = G;
	}

	return lpBits;
}

HBITMAP MPCPngImage::TypeLoadImage(int type, BYTE** pData, int* width, int* height, int* bpp, FILE* fp, int resid, int br, int rc, int gc, int bc)
{
	HBITMAP hbm;
	BYTE* bmp;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep* row_pointers;

	if (type == 0) {

		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		rewind(fp);
		bmp = (BYTE*)malloc(size);
		fread((void*)bmp, 1, size, fp);

		BITMAPINFO& bi = (BITMAPINFO&)bmp[sizeof(BITMAPFILEHEADER)];

		*width = bi.bmiHeader.biWidth;
		*height = bi.bmiHeader.biHeight;
		*bpp = bi.bmiHeader.biBitCount;

		hbm = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&(*pData), 0, 0);

	} else if (type == 1) {

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);

		if (fp) {
			png_init_io(png_ptr, fp);
		} else {
			HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resid), _T("PNG"));
			HANDLE lRes = LoadResource(NULL, hRes);

			struct png_t png;
			png.data = (unsigned char*)LockResource(lRes);
			png.size = SizeofResource(NULL, hRes);
			png.pos = 8;
			png_set_read_fn(png_ptr, (png_voidp)&png, read_data_fn);
			png_set_sig_bytes(png_ptr, 8);

			UnlockResource(lRes);
			FreeResource(lRes);
		}

		info_ptr = png_create_info_struct(png_ptr);

		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, 0);

		row_pointers = png_get_rows(png_ptr, info_ptr);

		*width = png_get_image_width(png_ptr, info_ptr);
		*height = png_get_image_height(png_ptr, info_ptr);
		*bpp = png_get_channels(png_ptr, info_ptr) * 8;

		BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), *width, -(*height), 1, *bpp, BI_RGB, 0, 0, 0, 0, 0}};

		hbm = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&(*pData), 0, 0);
	}

	int memWidth = (*width) * (*bpp) / 8;

	if (type == 0) {

		int hsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		memcpy(*pData, BrightnessRGB(0, bmp, *width, *height, *bpp, br, rc, gc, bc) + hsize, memWidth * (*height));
		free(bmp);

	} else if (type == 1) {

		for (int i = 0; i < *height; i++) {
			memcpy((*pData) + memWidth * i, BrightnessRGB(1, (BYTE*)row_pointers[i], *width, 1, *bpp, br, rc, gc, bc), memWidth);
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	}

	if (fp) {
		fclose(fp);
	}

	return hbm;
}

HBITMAP MPCPngImage::LoadExternalImage(CString fn, int br, int rc, int gc, int bc)
{
	CString path = LoadCurrentPath();

	BYTE* pData;
	int width, height, bpp;

	FILE* fp;
	_tfopen_s(&fp, path + fn + _T(".png"), _T("rb"));
	if (fp) {
		return TypeLoadImage(1, &pData, &width, &height, &bpp, fp, 0, br, rc, gc, bc);
	} else {
		_tfopen_s(&fp, path + fn + _T(".bmp"), _T("rb"));
		if (fp) {
			return TypeLoadImage(0, &pData, &width, &height, &bpp, fp, 0, br, rc, gc, bc);
		} else {
			if (AfxGetAppSettings().fDisableXPToolbars) {
				return TypeLoadImage(1, &pData, &width, &height, &bpp, NULL, IDB_PLAYERTOOLBAR_PNG, br, rc, gc, bc);
			} else {
				return NULL;
			}
		}
	}
}

void MPCPngImage::LoadExternalGradient(CString fn, CDC* dc, CRect r, int ptop, int br, int rc, int gc, int bc)
{
	CString path = LoadCurrentPath();

	BYTE* pData;
	int width, height, bpp;

	FILE* fp;
	_tfopen_s(&fp, path + fn + _T(".png"), _T("rb"));
	if (fp) {
		TypeLoadImage(1, &pData, &width, &height, &bpp, fp, 0, br, rc, gc, bc);
	} else {
		_tfopen_s(&fp, path + fn + _T(".bmp"), _T("rb"));
		if (fp) {
			TypeLoadImage(0, &pData, &width, &height, &bpp, fp, 0, br, rc, gc, bc);
		}
	}

	if (fp) {

		GRADIENT_RECT gr[1] = {{0, 1}};

		int bt = bpp / 8, st = 2, pa = 255 * 256;
		int sp = bt * ptop, hs = bt * st;

		if (width > height) {
			for (int k = sp, t = 0; t < r.right; k += hs, t += st) {
				TRIVERTEX tv[2] = {
					{t, 0, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
					{t + st, 1, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
				};
				dc->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_H);
			}
		} else {
			for (int k = sp, t = 0; t < r.bottom; k += hs, t += st) {
				TRIVERTEX tv[2] = {
					{r.left, t, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
					{r.right, t + st, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
				};
				dc->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
			}
		}
	}
}
