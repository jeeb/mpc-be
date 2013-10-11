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

#include <afxinet.h>
#include <libwebp/webp/decode.h>
#include <stb_image/stb_image.h>
#include "DIB.h"

using namespace Gdiplus;

static BYTE* ConvertRGBToBMPBuffer(BYTE* Buffer, int width, int height, int bpp, long* newsize)
{
	int padding = 0, scanlinebytes = width * bpp;
	while ((scanlinebytes + padding) % 4 != 0) {
		padding++;
	}
	int psw = scanlinebytes + padding;
	*newsize = height * psw;
	BYTE* newbuf = DNew BYTE[*newsize];
	memset(newbuf, 0, *newsize);
	long bufpos = 0, newpos = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < scanlinebytes; x += bpp) {
			bufpos = y * scanlinebytes + x;
			newpos = (height - y - 1) * psw + x;
			newbuf[newpos] = Buffer[bufpos + 2];
			newbuf[newpos + 1] = Buffer[bufpos + 1];
			newbuf[newpos + 2] = Buffer[bufpos];
		}
	}
	return newbuf;
}

#define GRAPHIC_FMT L"*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff;*.emf;*.ico;*.webp;*.webpll;*.psd;*.tga;*.gif"

static TCHAR* extimages[] = {
	L".bmp",  L".jpg",    L".jpeg", L".png",
	L".tif",  L".tiff",   L".emf",  L".ico",
	L".webp", L".webpll", L".psd",  L".tga"
};

static bool OpenImageCheck(CString fn)
{
	CString ext = CPath(fn).GetExtension().MakeLower();
	if (ext.IsEmpty()) {
		return false;
	}

	for (size_t i = 0; i < _countof(extimages); i++) {
		if (ext == extimages[i]) {
			return true;
		}
	}

	return false;
}

static HBITMAP SaveImageDIB(CString out, ULONG quality, bool mode, BYTE* pBuf, size_t pSize)
{
	HBITMAP hB = NULL;

	HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, pSize);
	BYTE* lpBits = (BYTE*)::GlobalLock(hG);
	memcpy(lpBits, pBuf, pSize);

	IStream *s;
	::CreateStreamOnHGlobal(hG, 1, &s);

	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	Bitmap *bm = new Bitmap(s);

	if (mode) {
		bm->GetHBITMAP(0, &hB);
	} else {
		CStringW format;
		CString ext = CPath(out).GetExtension().MakeLower();

		if (ext == L".bmp") {
			format = L"image/bmp";
		} else if (ext == L".jpg") {
			format = L"image/jpeg";
		} else if (ext == L".png") {
			format = L"image/png";
		} else if (ext == L".tif") {
			format = L"image/tiff";
		}
		GdiplusConvert(bm, out, format, quality, 0, NULL, 0);
	}

	delete bm;
	GdiplusShutdown(gdiplusToken);

	s->Release();
	::GlobalUnlock(hG);
	::GlobalFree(hG);

	return hB;
}

static HBITMAP OpenImageDIB(CString fn, CString out, ULONG quality, bool mode)
{
	if (OpenImageCheck(fn)) {

		CString ext = CPath(fn).GetExtension().MakeLower();

		HBITMAP hB = NULL;
		FILE *fp;
		TCHAR path_fn[_MAX_PATH];
		int type = 0, sih = sizeof(BITMAPINFOHEADER);

		if (wcsstr(fn, L"://")) {
			HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);
			if (s) {
				f = InternetOpenUrlW(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
				if (f) {
					type = 1;

					DWORD len;
					char buf[8192];
					TCHAR path[_MAX_PATH];

					GetTempPath(_MAX_PATH, path);
					GetTempFileName(path, _T("mpc_image"), 0, path_fn);
					fp = _tfopen(path_fn, _T("wb+"));

					for (;;) {
						InternetReadFile(f, buf, sizeof(buf), &len);

						if (!len) {
							break;
						}

						fwrite(buf, len, 1, fp);
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
		BYTE *data = (BYTE*)malloc(fs), *pBuf = NULL;
		fread(data, fs, 1, fp);
		fclose(fp);

		if (type) {
			_tunlink(path_fn);
		}

		BITMAPFILEHEADER bfh;
		if (!mode) {
			bfh.bfType = 0x4d42;
			bfh.bfOffBits = sizeof(bfh) + sih;
			bfh.bfReserved1 = bfh.bfReserved2 = 0;
		}

		if (ext == L".webp" || ext == L".webpll") {

			WebPDecoderConfig config;
			WebPDecBuffer* const out_buf = &config.output;
			WebPInitDecoderConfig(&config);
			config.options.use_threads = 1;
			WebPDecode((const uint8_t*)data, fs, &config);

			int width = out_buf->width, height = out_buf->height, bit = (out_buf->colorspace == MODE_RGBA ? 32 : 24);
			size_t slen;
			BYTE *bmp = ConvertRGBToBMPBuffer((BYTE*)out_buf->u.RGBA.rgba, width, height, 3, (long*)&slen);

			BITMAPINFO bi = {{sih, width, height, 1, bit, BI_RGB, 0, 0, 0, 0, 0}};

			if (mode) {
				hB = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&pBuf, 0, 0);
				memcpy(pBuf, bmp, slen);
			} else {
				bfh.bfSize = bfh.bfOffBits + slen;
				pBuf = (BYTE*)malloc(bfh.bfSize);
				memset(pBuf, 0, bfh.bfSize);
				memcpy(pBuf, &bfh, sizeof(bfh));
				memcpy(pBuf + sizeof(bfh), &bi.bmiHeader, sih);
				memcpy(pBuf + bfh.bfOffBits, bmp, slen);
				SaveImageDIB(out, quality, 0, pBuf, bfh.bfSize);
				free(pBuf);
			}

			WebPFreeDecBuffer(out_buf);

		} else if (ext == L".psd" || ext == L".tga") {

			int width, height, n, bpp = 4;
			BYTE *lpBits = (BYTE*)stbi_load_from_memory((const stbi_uc*)data, fs, &width, &height, &n, bpp);
			size_t slen;
			BYTE *bmp = ConvertRGBToBMPBuffer(lpBits, width, height, bpp, (long*)&slen);

			BITMAPINFO bi = {{sih, width, height, 1, bpp * 8, BI_RGB, 0, 0, 0, 0, 0}};

			if (mode) {
				hB = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&pBuf, 0, 0);
				memcpy(pBuf, bmp, slen);
			} else {
				bfh.bfSize = bfh.bfOffBits + slen;
				pBuf = (BYTE*)malloc(bfh.bfSize);
				memset(pBuf, 0, bfh.bfSize);
				memcpy(pBuf, &bfh, sizeof(bfh));
				memcpy(pBuf + sizeof(bfh), &bi.bmiHeader, sih);
				memcpy(pBuf + bfh.bfOffBits, bmp, slen);
				SaveImageDIB(out, quality, 0, pBuf, bfh.bfSize);
				free(pBuf);
			}

		} else {
			hB = SaveImageDIB(out, quality, mode, data, fs);
		}

		free(data);

		return hB;
	}

	return NULL;
}

static HBITMAP OpenImage(CString fn)
{
	return OpenImageDIB(fn, L"", 0, 1);
}
