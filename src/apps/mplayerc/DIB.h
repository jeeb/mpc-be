/*
 * $Id: DIB.h 498 2012-06-05 23:09:29Z exodus8 $
 *
 * Adaptation for MPC-BE (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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

#include <libpng/png.h>
#include <libwebp/webp/encode.h>

void BMPDIB(LPCTSTR fn, BYTE* pData)
{
	FILE* fp;
	_tfopen_s(&fp, fn, _T("wb"));
	if (fp) {
		BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;
		bih->biBitCount = 24;

		int width = bih->biWidth, height = abs(bih->biHeight), sih = sizeof(BITMAPINFOHEADER);
		int line, stride = width * 3 * sizeof(BYTE), len = width * height * 3;

		BYTE* rgb = (BYTE*)malloc(stride * height);
		BYTE *p, *src = pData + sih;

		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				line = (3 * x) + (stride * y);
				p = src + (width * 4 * y) + (4 * x);
				rgb[line] = p[0];
				rgb[line + 1] = p[1];
				rgb[line + 2] = p[2];
			}
		}

		BITMAPFILEHEADER bfh;
		bfh.bfType = 0x4d42;
		bfh.bfOffBits = sizeof(bfh) + sih;
		bfh.bfSize = bfh.bfOffBits + len;
		bfh.bfReserved1 = bfh.bfReserved2 = 0;

		fwrite(&bfh, 1, sizeof(bfh), fp);
		fwrite(bih, 1, sih, fp);
		fwrite(rgb, 1, len, fp);

		fclose(fp);
		free(rgb);
	}
}

void PNGDIB(LPCTSTR fn, BYTE* pData, int level)
{
	FILE* fp;
	_tfopen_s(&fp, fn, _T("wb"));
	if (fp) {
		BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

		int line, width = bih->biWidth, height = abs(bih->biHeight);

		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_set_compression_level(png_ptr, level);
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 0, 0);
		png_write_info(png_ptr, info_ptr);

		png_bytep row_ptr = (png_bytep)malloc(width * 3);
		BYTE *p, *src = pData + sizeof(BITMAPINFOHEADER);

		for(int y = height - 1; y >= 0; y--) {
			for(int x = 0; x < width; x++) {
				line = (3 * x);
				p = src + (width * 4 * y) + (4 * x);
				row_ptr[line] = (png_byte)p[2];
				row_ptr[line + 1] = (png_byte)p[1];
				row_ptr[line + 2] = (png_byte)p[0];
			}
			png_write_row(png_ptr, row_ptr);
		}
		png_write_end(png_ptr, info_ptr);

		fclose(fp);
		free(row_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
}

int WebPWriter(const uint8_t* data, size_t size, const WebPPicture* const pic)
{
	return fwrite(data, size, 1, (FILE*)pic->custom_ptr);
}

void WebPDIB(LPCTSTR fn, BYTE* pData, float quality)
{
	FILE* fp;
	_tfopen_s(&fp, fn, _T("wb"));
	if (fp) {
		BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

		int width = bih->biWidth, height = abs(bih->biHeight);
		int line, stride = width * 3 * sizeof(uint8_t);

		uint8_t* rgb = (uint8_t*)malloc(stride * height);
		BYTE *p, *src = pData + sizeof(BITMAPINFOHEADER);

		for(int y = 0, j = height - 1; y < height; y++, j--) {
			for(int x = 0; x < width; x++) {
				line = (3 * x) + (stride * j);
				p = src + (width * 4 * y) + (4 * x);
				rgb[line] = (uint8_t)p[2];
				rgb[line + 1] = (uint8_t)p[1];
				rgb[line + 2] = (uint8_t)p[0];
			}
		}

		WebPConfig config;
		WebPConfigInit(&config);
		config.quality = quality;

		WebPPicture picture;
		WebPPictureInit(&picture);

		picture.width = width;
		picture.height = height;
		WebPPictureImportRGB(&picture, rgb, stride);

		picture.writer = WebPWriter;
		picture.custom_ptr = (void*)fp;
		WebPEncode(&config, &picture);

		fclose(fp);
		free(rgb);
		WebPPictureFree(&picture);
	}
}
