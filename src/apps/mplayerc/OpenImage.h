/*
 * $Id$
 *
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

#include "PngImage.h"
#include <libwebp/webp/decode.h>

using namespace Gdiplus;

static BYTE* ConvertRGBToBMPBuffer(BYTE* Buffer, int width, int height, long* newsize)
{
	int padding = 0, scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0) {
		padding++;
	}
	int psw = scanlinebytes + padding;
	*newsize = height * psw;
	BYTE* newbuf = new BYTE[*newsize];
	memset(newbuf, 0, *newsize);
	long bufpos = 0, newpos = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < 3 * width; x += 3) {
			bufpos = y * 3 * width + x;
			newpos = (height - y - 1) * psw + x;
			newbuf[newpos] = Buffer[bufpos + 2];
			newbuf[newpos + 1] = Buffer[bufpos + 1];
			newbuf[newpos + 2] = Buffer[bufpos];
		}
	}
	return newbuf;
}

static bool OpenImageCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (wcsstr(tmp_fn, L".bmp")
		|| wcsstr(tmp_fn, L".jpg")
		|| wcsstr(tmp_fn, L".jpeg")
		|| wcsstr(tmp_fn, L".png")
		|| wcsstr(tmp_fn, L".gif")
		|| wcsstr(tmp_fn, L".tif")
		|| wcsstr(tmp_fn, L".tiff")
		|| wcsstr(tmp_fn, L".emf")
		|| wcsstr(tmp_fn, L".ico")
		|| wcsstr(tmp_fn, L".webp")
		|| wcsstr(tmp_fn, L".webpll")) {
		return 1;
	}

	return 0;
}

static HBITMAP OpenImage(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (OpenImageCheck(fn)) {

		HBITMAP hB;
		FILE *fp;
		TCHAR path_fn[_MAX_PATH];
		int type = 0;

		if (wcsstr(fn, L"://")) {
			HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);
			if (s) {
				f = InternetOpenUrlW(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);
				if (f) {
					type++;

					DWORD len;
					char buf[4096];
					TCHAR path[_MAX_PATH];

					GetTempPath(_MAX_PATH, path);
					GetTempFileName(path, _T("mpc_image"), 0, path_fn);
					fp = _tfopen(path_fn, _T("wb+"));

					for (;;) {
						InternetReadFile(f, buf, sizeof(buf), &len);

						if (!len) {
							break;
						}

						fwrite(buf, 1, len, fp);
					}
					InternetCloseHandle(f);
				}
				InternetCloseHandle(s);
				if (!f) {
					return NULL;
				}
			} else {
				return NULL;
			}
		} else {
			fp = _tfopen(fn, _T("rb"));
			if (!fp) {
				return NULL;
			}
			fseek(fp, 0, SEEK_END);
		}

		DWORD fs = ftell(fp);
		rewind(fp);
		void *data = malloc(fs);
		fread(data, 1, fs, fp);
		fclose(fp);

		if (type) {
			_tunlink(path_fn);
		}

		if (wcsstr(tmp_fn, L".webp") || wcsstr(tmp_fn, L".webpll")) {

			WebPDecoderConfig config;
			WebPDecBuffer* const out_buf = &config.output;
			WebPInitDecoderConfig(&config);
			config.options.use_threads = 1;
			WebPDecode((const uint8_t*)data, fs, &config);

			int width = out_buf->width, height = out_buf->height, bit = (out_buf->colorspace == MODE_RGBA ? 32 : 24);
			uint8_t *rgb = out_buf->u.RGBA.rgba;
			size_t slen;
			BYTE *pBits, *bmp = ConvertRGBToBMPBuffer((BYTE*)rgb, width, height, (long*)&slen);

			BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), width, height, 1, bit, BI_RGB, 0, 0, 0, 0, 0}};
			hB = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&pBits, 0, 0);
			memcpy(pBits, bmp, slen);

			WebPFreeDecBuffer(out_buf);

		} else if (wcsstr(tmp_fn, L".png")) {

			struct png_t png;
			png.data = (unsigned char*)data;
			png.size = fs;
			png.pos = 8;

			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
			png_set_read_fn(png_ptr, (png_voidp)&png, read_data_fn);
			png_set_sig_bytes(png_ptr, 8);
			png_infop info_ptr = png_create_info_struct(png_ptr);

			png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, 0);
			png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);

			int width = png_get_image_width(png_ptr, info_ptr), height = png_get_image_height(png_ptr, info_ptr);
			int bit = png_get_channels(png_ptr, info_ptr);
			int memWidth = width * bit;

			BYTE *pBits;
			BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), width, -height, 1, bit * 8, BI_RGB, 0, 0, 0, 0, 0}};
			hB = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&pBits, 0, 0);
			for (int i = 0; i < height; i++) {
				memcpy(pBits + memWidth * i, row_pointers[i], memWidth);
			}

			png_destroy_read_struct(&png_ptr, &info_ptr, 0);

		} else {

			HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, fs);
			LPVOID lpBits = ::GlobalLock(hG);
			memcpy(lpBits, data, fs);

			IStream *s;
			::CreateStreamOnHGlobal(hG, 1, &s);

			ULONG_PTR gdiplusToken;
			GdiplusStartupInput gdiplusStartupInput;
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
			Bitmap *bm = new Bitmap(s);
			bm->GetHBITMAP(0, &hB);
			delete bm;
			GdiplusShutdown(gdiplusToken);

			s->Release();
			::GlobalUnlock(hG);
			::GlobalFree(hG);
		}

		free(data);

		return hB;
	}

	return NULL;
}
