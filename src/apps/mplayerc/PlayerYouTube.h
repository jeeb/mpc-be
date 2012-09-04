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

#pragma once

#include <afxinet.h>

static int _HexPairValue(const char* code)
{
	int value = 0;
	const char* pch = code;

	for (;;) {
		int digit = *pch++;

		if (digit >= '0' && digit <= '9') {
			value += digit - '0';
		} else if (digit >= 'A' && digit <= 'F') {
			value += digit - 'A' + 10;
		} else if (digit >= 'a' && digit <= 'f') {
			value += digit - 'a' + 10;
		} else {
			return -1;
		}

		if (pch == code + 2) {
			return value;
		}

		value <<= 4;
	}
}

static int _UrlDecode(const char* source, char* dest)
{
	char* start = dest;

	while (*source) {
		switch (*source)
		{
		case '+':
			*(dest++) = ' ';
			break;
		case '%':
			if (source[1] && source[2]) {
				int value = _HexPairValue(source + 1);

				if (value >= 0) {
					*(dest++) = value;
					source += 2;
				} else {
					*dest++ = '?';
				}
			} else {
				*dest++ = '?';
			}
			break;
		default:
			*dest++ = *source;
		}
		source++;
	}
	*dest = 0;

	return dest - start;
}

static int _strpos(char* h, char* n)
{
	char* p = strstr(h, n);

	if(p) {
		return p - h;
	} else {
		return 0;
	}
}

CString PlayerYouTube(CString fname)
{
	if (wcsstr(fname, L"youtube.com/watch?")) {

		return _T(""); // TODO - disable playback from youtube unlike fix parser;

		char *out, buf[4096], str1[1024], str2[1024];
		DWORD len, size = 0, fs = 10 * sizeof(buf);

		HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);

		if (s) {

			f = InternetOpenUrlW(s, fname, 0, 0, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);

			if (f) {

				wchar_t path[_MAX_PATH], fn[_MAX_PATH];

				GetTempPathW(_MAX_PATH, path);
				GetTempFileNameW(path, _T("mpc_youtube"), 0, fn);

				FILE* fp;
				_wfopen_s(&fp, fn, _T("wt+"));

				for (;;) {

					InternetReadFile(f, buf, sizeof(buf), &len);
					size += len;

					if (!len || size > fs) {
						break;
					}

					fwrite(buf, len, 1, fp);
				}

				rewind(fp);
				out = (char*)malloc(size);
				fread(out, size, 1, fp);
				fclose(fp);

				_wunlink(fn);

				InternetCloseHandle(f);
			}

			InternetCloseHandle(s);
		}

		char *start		= out;
		DWORD start_pos	= 0;
		DWORD stop_pos	= 0;

		for(;;) {
			start_pos = _strpos(start, "url%3Dhttp%253A%252F%252F");
			if (!start_pos) {
				free(out);
				return fname;
			}
			start += (start_pos + 6);

			stop_pos = _strpos(start, "%26quality");
			if (!stop_pos) {
				free(out);
				return fname;
			}

			DWORD webm_pos = _strpos(start, "video%252Fwebm"); // skip webm video;
			if (webm_pos && webm_pos < stop_pos) {
				continue;
			}

			break;
		}

		memset(str1, 0, sizeof(str1));
		memset(str2, 0, sizeof(str2));

		memcpy(str1, start, stop_pos);

		_UrlDecode(str1, str2);
		_UrlDecode(str2, str1);

		free(out);

		return CString(str1);
	} else {
		return fname;
	}
}
