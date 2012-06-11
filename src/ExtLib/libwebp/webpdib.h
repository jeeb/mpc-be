/*
 * $Id: webpdib.h 498 2012-06-05 23:09:29Z exodus8 $
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

#include <libwebp/webp/encode.h>

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
