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

#include <libpng/png.h>
#include <libwebp/webp/encode.h>

using namespace Gdiplus;

static int GetEncoderClsid(CStringW format, CLSID *pClsid)
{
	UINT num = 0, size = 0;
	GetImageEncodersSize(&num, &size);
	if (size == 0) {
		return -1;
	}
	ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)malloc(size);
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; j++) {
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

static void BMPDIB(LPCTSTR fn, BYTE* pData, CStringW format, ULONG quality)
{
	FILE* fp;
	_tfopen_s(&fp, fn, _T("wb"));
	if (fp) {
		BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

		int width = bih->biWidth, height = abs(bih->biHeight), bit = 24;
		int stride = (width * bit + 31) / 32 * 4, sih = sizeof(BITMAPINFOHEADER);
		int line, len = stride * height;

		BYTE* rgb = (BYTE*)malloc(len);
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

		BITMAPINFOHEADER header;
		header.biSize = sih;
		header.biWidth = width;
		header.biHeight = height;
		header.biPlanes = 1;
		header.biBitCount = bit;
		header.biCompression = BI_RGB;
		header.biSizeImage = 0;
		header.biXPelsPerMeter = header.biYPelsPerMeter = 0;
		header.biClrUsed = header.biClrImportant = 0;

		if (format == L"") {
			fwrite(&bfh, sizeof(bfh), 1, fp);
			fwrite(&header, sih, 1, fp);
			fwrite(rgb, len, 1, fp);
			fclose(fp);
		} else {
			fclose(fp);

			HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, bfh.bfOffBits + len);
			LPVOID lpBits = ::GlobalLock(hG);

			memcpy((BYTE*)lpBits, &bfh, sizeof(bfh));
			memcpy((BYTE*)lpBits + sizeof(bfh), &header, sih);
			memcpy((BYTE*)lpBits + bfh.bfOffBits, rgb, len);

			IStream *s;
			::CreateStreamOnHGlobal(hG, 1, &s);

			ULONG_PTR gdiplusToken;
			GdiplusStartupInput gdiplusStartupInput;
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
			Bitmap *bm = new Bitmap(s);

			CLSID encoderClsid = CLSID_NULL;
			GetEncoderClsid(format, &encoderClsid);

			EncoderParameters encoderParameters;
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].NumberOfValues = 1;
			encoderParameters.Parameter[0].Value = &quality;
			encoderParameters.Parameter[0].Guid = EncoderQuality;
			encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;

			bm->Save(CStringW(fn), &encoderClsid, &encoderParameters);

			delete bm;
			GdiplusShutdown(gdiplusToken);

			s->Release();
			::GlobalUnlock(hG);
			::GlobalFree(hG);
		}

		free(rgb);
	}
}

static void PNGDIB(LPCTSTR fn, BYTE* pData, int level)
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

static int WebPWriter(const uint8_t* data, size_t size, const WebPPicture* const pic)
{
	return fwrite(data, size, 1, (FILE*)pic->custom_ptr);
}

static void WebPDIB(LPCTSTR fn, BYTE* pData, float quality)
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

		WebPPicture picture;
		WebPPictureInit(&picture);

		if (quality) {
			config.quality = quality;
		} else {
			config.lossless = 1;
			picture.use_argb = 1;
		}

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
