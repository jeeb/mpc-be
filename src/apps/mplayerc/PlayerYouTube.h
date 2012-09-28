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

	if (p) {
		return p - h;
	} else {
		return 0;
	}
}

#define YOUTUBE_URL			_T("youtube.com/watch?")
#define YOUTUBE_FULL_URL		_T("www.youtube.com/watch?v=")
#define YOUTU_BE_URL			_T("youtu.be/")
#define YOUTU_BE_FULL_URL		_T("www.youtu.be/")

static CString PlayerYouTube(CString fn, CString* out_title)
{
	if (out_title) {
		*out_title = _T("");
	}

	CString tmp_fn(CString(fn).MakeLower());

	if ((tmp_fn.Find(YOUTUBE_URL) != -1) || (tmp_fn.Find(YOUTU_BE_URL) != -1)) {

		if (tmp_fn.Find(YOUTU_BE_URL) != -1) {
			fn.Replace(YOUTU_BE_FULL_URL, YOUTUBE_FULL_URL);
			fn.Replace(YOUTU_BE_URL, YOUTUBE_FULL_URL);
		}

		char *out = NULL;

		HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);

		if (s) {

			f = InternetOpenUrl(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);

			if (f) {

				char buf[4096];
				DWORD len, size = 0, end_pos = 0, fs = 12 * sizeof(buf);

				out = (char*)malloc(fs + 1);

				memset(out, 0, fs + 1);

				for (;;) {

					InternetReadFile(f, buf, sizeof(buf), &len);
					size += len;

					if (!len || size > fs) {
						break;
					}

					memcpy(out + (size - len), buf, len);

					if (end_pos) {
						break;
					}
					if (!end_pos && strstr(out, "%26quality")) {
						end_pos++;
					}
				}

				InternetCloseHandle(f);
			} else {
				InternetCloseHandle(s);
				return fn;
			}

			InternetCloseHandle(s);
		} else {
			return fn;
		}

		// get name(title) for output filename
		CString Title;
		int t_start = _strpos(out, "<meta name=\"title\" content=\"");
		if (t_start > 0) {
			t_start += 28;
			int t_stop = _strpos(out + t_start, "\">");
			if (t_stop > 0) {
				char* title	= (char*)malloc(t_stop + 1);
				memset(title, 0, t_stop + 1);
				memcpy(title, out + t_start, t_stop);

				Title = CA2CT(title, CP_UTF8);

				Title = Title.TrimLeft(_T(".")).TrimRight(_T("."));
				Title.Replace(_T("|"), _T("-"));
				Title.Replace(_T("&quot;"), _T("\""));
				Title.Replace(_T("&#39;"), _T("\""));

				free(title);
			}
		}

		if (Title.IsEmpty()) {
			Title = _T("vid");
		}

		DWORD k, lastpos = 0;
		for (;;) {
			k = _strpos(out + lastpos, "%2Curl%3Dhttp%253A%252F%252F");
			if (!k) {
				k = _strpos(out + lastpos, "%26url%3Dhttp%253A%252F%252F");
			}

			if (k) {
			
				k += (lastpos + 9);
				lastpos = k;
				DWORD i = _strpos(out + k, "%26quality");
				if (!i) {
					free(out);
					return fn;
				}

				// skip webm format
				DWORD l = _strpos(out + k, "video%252Fwebm");
				if (l && (l < i)) {
					continue;
				}

				// get extension for output filename
				CString ext;
				l = _strpos(out + k, "video%252Fmp4");
				if (l && (l < i)) {
					ext = _T(".mp4");
				}
				if (ext.IsEmpty()) {
					DWORD l = _strpos(out + k, "video%252Fx-flv");
					if (l && (l < i)) {
						ext = _T(".flv");
					}
				}
				if (ext.IsEmpty()) {
					DWORD l = _strpos(out + k, "video%252F3gpp");
					if (l && (l < i)) {
						ext = _T(".3gp");
					}
				}
				if (ext.IsEmpty()) {
					ext = _T(".mp4");
				}

				char *str1, *str2;

				str1 = (char*)malloc(i + 1);
				str2 = (char*)malloc(i + 1);

				memset(str1, 0, i + 1);
				memset(str2, 0, i + 1);

				memcpy(str1, out + k, i);

				_UrlDecode(str1, str2);
				_UrlDecode(str2, str1);

				CString str(str1);

				free(str1);
				free(str2);

				free(out);

				// need for some url
				str.Replace(_T("&sig="), _T("&signature="));

				// add file name for future ...
				Title.Replace(ext, _T(""));
				str.Append(_T("&title=") + Title + ext);

				if (out_title) {
					*out_title = Title + ext;
				}

				return str;
			} else {
				free(out);

				return fn;
			}
		}
	} else {
		return fn;
	}
}
