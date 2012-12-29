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

#include "stdafx.h"
#include <afxinet.h>
#include "PlayerYouTube.h"

CString PlayerYouTube(CString fn, CString* out_title)
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

		int match_start	= 0;
		int match_len	= 0;

		char *out = NULL;
		HINTERNET f, s = InternetOpen(0, 0, 0, 0, 0);
		if (s) {
			f = InternetOpenUrl(s, fn, 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (f) {
				DWORD dwBytesRead	= 0;
				DWORD dataSize		= 0;

				do {
					char buffer[4096];
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
						break;
					}

					char *tempData = DNew char[dataSize + dwBytesRead];
					memcpy(tempData, out, dataSize);
					memcpy(tempData + dataSize, buffer, dwBytesRead);
					delete[] out;
					out = tempData;
					dataSize += dwBytesRead;

					// optimization - to not download the entire page
					if (!match_start) {
						match_start	= strpos(out, MATCH_START);
					} else {
						match_len	= strpos(out + match_start, MATCH_END);
					};

					if (match_start && match_len) {
						match_start += strlen(MATCH_START);
						match_len	-= strlen(MATCH_START);
						break;
					}

				} while (dwBytesRead);

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);

			if (!f) {
				return fn;
			}
		} else {
			return fn;
		}

		if (!match_start || !match_len) {
			if (strpos(out, "http://www.youtube.com")) {
				// This is looks like Youtube page, but this page doesn't contains necessary information about video, so may be you have to register on google.com to view it.
				fn.Empty();
			}
			delete[] out;
			return fn;
		}

		// get name(title) for output filename
		CString Title;
		int t_start = strpos(out, "<title>");
		if (t_start > 0) {
			t_start += 7;
			int t_stop = strpos(out + t_start, "</title>");
			if (t_stop > 0) {
				char* title = DNew char[t_stop + 1];
				memset(title, 0, t_stop + 1);
				memcpy(title, out + t_start, t_stop);

				Title = UTF8To16(title);
				Title = Title.TrimLeft(_T(".")).TrimRight(_T("."));

				Title.Replace(_T(" - YouTube"), _T(""));
				Title.Replace(_T(":"), _T(" -"));
				Title.Replace(_T("|"), _T("-"));
				Title.Replace(_T("—"), _T("-"));
				Title.Replace(_T("--"), _T("-"));
				Title.Replace(_T("  "), _T(" "));

				Title.Replace(_T("&quot;"), _T("\""));
				Title.Replace(_T("&amp;"), _T("&"));
				Title.Replace(_T("&#39;"), _T("\""));

				delete [] title;
			}
		}

		if (Title.IsEmpty()) {
			Title = _T("video");
		}

		char *str1 = DNew char[match_len + 1];
		memset(str1, 0, match_len + 1);
		memcpy(str1, out + match_start, match_len);
				
		CString str = UTF8To16(UrlDecode(UrlDecode(CStringA(str1))));
		delete [] str1;
		delete [] out;

		if (str.Find(URL_DELIMETER) == -1) {
			return fn;
		}

		str.Delete(0, str.Find(URL_DELIMETER) + CString(URL_DELIMETER).GetLength());

		CAtlList<CString> sl;

		Explode(str, URL_DELIMETER, sl);

		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			str = sl.GetNext(pos);

			// little fix
			str.Replace(_T(","), _T("&"));

			int sigStart = str.Find(_T("&sig"));
			if (sigStart <= 0) {
				continue;
			}

			int sigEnd = str.Find(_T("&"), sigStart + 1);
			if (sigEnd == -1) {
				sigEnd = str.GetLength();
			}

			CMapStringToString UrlFields;
			{
				// extract fields/params from url
				int p = str.Find(_T("videoplayback?"));
				if (p > 0) {
					CString templateStr = str.Mid(p + 14, str.GetLength() - p - 14);
					CAtlList<CString> sl;
					Explode(templateStr, sl, '&');
					POSITION pos = sl.GetHeadPosition();
					while (pos) {
						CAtlList<CString> sl2;
						Explode(sl.GetNext(pos), sl2, '=');
						CString rValue;
						if (sl2.GetCount() == 2 && !UrlFields.Lookup(sl2.GetHead(), rValue)) {
							UrlFields[sl2.GetHead()] = sl2.GetTail();
						}
					}
				} else {
					continue;
				}
			}

			CString ext;

			CString itagValueStr;
			UrlFields.Lookup(_T("itag"), itagValueStr);
			if (itagValueStr.IsEmpty()) {
				continue;
			}

			// format of the video - http://en.wikipedia.org/wiki/YouTube#Quality_and_codecs
			int itagValue = 0;
			if (_stscanf_s(itagValueStr, _T("%d"), &itagValue) == 1) {
				YOUTUBE_PROFILES youtubePtofile = getProfile(itagValue);
				if (youtubePtofile.iTag == 0 || youtubePtofile.Container == _T("WebM") || youtubePtofile.Profile == _T("3D")) {
					continue;
				}

				ext.Format(_T(".%s"), youtubePtofile.Container);
				ext.MakeLower();
			}

			if (!itagValue) {
				continue;
			}

			if (ext.IsEmpty()) {
				continue;
			}

			str = str.Left(sigEnd);

			// it is necessary for the proper formation of links to the video file
			str.Replace(_T("&sig="), _T("&signature="));

			// add file name for future
			Title.Replace(ext, _T(""));
			str.Append(_T("&title="));
			str.Append(Title);//str.Append(CString(UrlEncode(UTF16To8(Title))));
			str.Append(ext);

			if (out_title) {
				*out_title = Title + ext;
			}

			return str;
		}

		return fn;
	}

	return fn;
}
