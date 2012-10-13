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

#define YOUTUBE_URL			_T("youtube.com/watch?")
#define YOUTUBE_FULL_URL	_T("www.youtube.com/watch?v=")
#define YOUTU_BE_URL		_T("youtu.be/")
#define YOUTU_BE_FULL_URL	_T("www.youtu.be/")

static int strpos(char* h, char* n)
{
	char* p = strstr(h, n);

	if (p) {
		return p - h;
	}

	return 0;
}

static CString PlayerYouTube(CString fn, CString* out_title)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(YOUTUBE_URL) != -1 || tmp_fn.Find(YOUTU_BE_URL) != -1) {

		if (tmp_fn.Find(YOUTU_BE_URL) != -1) {
			fn.Replace(YOUTU_BE_FULL_URL, YOUTUBE_FULL_URL);
			fn.Replace(YOUTU_BE_URL, YOUTUBE_FULL_URL);
		}

		if (out_title) {
			*out_title = _T("");
		}

		char *out = NULL;

		HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);

		if (s) {

			f = InternetOpenUrl(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);

			if (f) {

				char buf[4096];
				DWORD len, size = 0, end_pos = 0, fs = 14 * sizeof(buf);

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
			}

			InternetCloseHandle(s);

			if (!f) {
				return fn;
			}
		} else {
			return fn;
		}

		// get name(title) for output filename
		CString Title;
		int t_start = strpos(out, "<meta name=\"title\" content=\"");
		if (t_start > 0) {
			t_start += 28;
			int t_stop = strpos(out + t_start, "\">");
			if (t_stop > 0) {
				char* title	= (char*)malloc(t_stop + 1);
				memset(title, 0, t_stop + 1);
				memcpy(title, out + t_start, t_stop);

				Title = CA2CT(title, CP_UTF8);

				Title = Title.TrimLeft(_T(".")).TrimRight(_T("."));
				Title.Replace(_T("|"), _T("-"));
				Title.Replace(_T("&quot;"), _T("\""));
				Title.Replace(_T("&amp;"), _T("&"));
				Title.Replace(_T("&#39;"), _T("\""));

				free(title);
			}
		}

		if (Title.IsEmpty()) {
			Title = _T("vid");
		}

		DWORD i, k, l, lastpos = 0;
		for (;;) {
			k = strpos(out + lastpos, "%2Curl%3Dhttp%253A%252F%252F");
			if (!k) {
				k = strpos(out + lastpos, "%26url%3Dhttp%253A%252F%252F");
			}

			if (k) {

				k += (lastpos + 9);
				lastpos = k;
				i = strpos(out + k, "%26quality");
				if (!i) {
					free(out);
					return fn;
				}

				// skip stereo3d format
				l = strpos(out + k, "%26stereo3d");
				if (l && (l < i)) {
					continue;
				}

				// skip webm format
				l = strpos(out + k, "video%252Fwebm");
				if (l && (l < i)) {
					continue;
				}

				// get extension for output filename
				CString ext;
				l = strpos(out + k, "video%252Fmp4");
				if (l && (l < i)) {
					ext = _T(".mp4");
				}
				if (ext.IsEmpty()) {
					l = strpos(out + k, "video%252Fx-flv");
					if (l && (l < i)) {
						ext = _T(".flv");
					}
				}
				if (ext.IsEmpty()) {
					l = strpos(out + k, "video%252F3gpp");
					if (l && (l < i)) {
						ext = _T(".3gp");
					}
				}
				if (ext.IsEmpty()) {
					ext = _T(".mp4");
				}

				char *str1;

				str1 = (char*)malloc(i + 1);

				memset(str1, 0, i + 1);

				memcpy(str1, out + k, i);

				CString str = UTF8To16(UrlDecode(UrlDecode(CStringA(str1))));

				free(str1);

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
	}

	return fn;
}
