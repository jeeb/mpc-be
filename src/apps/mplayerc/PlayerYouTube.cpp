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
#include "PlayerYouTube.h"
#include "PlayerVimeo.h"

bool PlayerYouTubeCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if ((tmp_fn.Find(YOUTUBE_URL) != -1 && tmp_fn.Find(_T("&list=")) < 0) || tmp_fn.Find(YOUTU_BE_URL) != -1) {
		return 1;
	}

	return 0;
}

CString PlayerYouTube(CString fn, CString* out_Title, CString* out_Author)
{
	if (out_Title) {
		*out_Title = _T("");
	}
	if (out_Author) {
		*out_Author = _T("");
	}

	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(VIMEO_URL) != -1) {
		if (out_Title) {
			*out_Title = PlayerVimeoTitle(fn);
		}
		return PlayerVimeo(fn);
	}

	if (PlayerYouTubeCheck(fn)) {

#ifdef _DEBUG
		LOG2FILE(_T("------"));
		LOG2FILE(_T("Youtube parser"));
#endif
		char* final	= NULL;

		CString str;
		CString Title, Author;

		int match_start	= 0;
		int match_len	= 0;

		final = NULL;
		HINTERNET f, s = InternetOpen(L"MPC-BE Youtube Downloader", 0, NULL, NULL, 0);
		if (s) {
			CString link = fn;
			if (link.Find(YOUTU_BE_URL) != -1) {
				link.Replace(YOUTU_BE_URL, YOUTUBE_URL);
				link.Replace(_T("watch?"), _T("watch?v="));
			}
			f = InternetOpenUrl(s, link, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (f) {
				char *out			= NULL;
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
						match_len	= strpos(out + match_start + strlen(MATCH_START), MATCH_END);
					}

					if (match_start && match_len) {
						match_start += strlen(MATCH_START);
						match_len	-= strlen(MATCH_START);
						break;
					}
				} while (dwBytesRead);

				final = DNew char[dataSize + 1];
				memset(final, 0, dataSize + 1);
				memcpy(final, out, dataSize);
				delete [] out;

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);

			if (!f) {
				return fn;
			}
		} else {
			return fn;
		}

		if (!final) {
			return fn;
		}

		if (!match_start || !match_len) {
			if (strstr(final, "http://www.youtube.com")) {
				// This is looks like Youtube page, but this page doesn't contains necessary information about video, so may be you have to register on google.com to view it.
				fn.Empty();
			}
			delete[] final;
			return fn;
		}

		int t_start = strpos(final, "<title>");
		if (t_start > 0) {
			t_start += 7;
			int t_stop = strpos(final + t_start, "</title>");
			if (t_stop > 0) {
				char* title = DNew char[t_stop + 1];
				memset(title, 0, t_stop + 1);
				memcpy(title, final + t_start, t_stop);

				Title = PlayerYouTubeTitle(title);
				Title.Replace(_T(" - YouTube"), _T(""));

				delete [] title;
			}
		}

		char *tmp = DNew char[match_len + 1];
		memset(tmp, 0, match_len + 1);
		memcpy(tmp, final + match_start, match_len);

		// because separator is a ',', then replace it with '~' to avoid matches
		for (size_t i = 0; i < strlen(tmp); i++) {
			if (tmp[i] == ',') {
				tmp[i] = '~';
			}
		}
		str = UTF8To16(UrlDecode(UrlDecode(CStringA(tmp))));
		delete [] tmp;

		// Character Representation
		str.Replace(_T("\\u0022"), _T("\""));
		str.Replace(_T("\\u0026"), _T("&"));
		str.Replace(_T("\\u0027"), _T("'"));
		str.Replace(_T("\\u003c"), _T("<"));
		str.Replace(_T("\\u003e"), _T(">"));

#ifdef _DEBUG
		LOG2FILE(_T("Source = \'%s\'"), str);
#endif

		CAtlList<CString> sl;
		Explode(str, sl, '~');

#ifdef _DEBUG
		{
			LOG2FILE(_T("	==> \'%s\'"), str);
			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				LOG2FILE(_T("		=== >\'%s\'"), sl.GetNext(pos));
			}
		}
#endif

		CString tag;
		tag.Format(_T("itag=%d"), AfxGetAppSettings().iYoutubeTag);
		boolean match_itag = AfxGetAppSettings().iYoutubeTag != 0;

again:

		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			str = sl.GetNext(pos);
			//str.Trim(_T("&,="));

			if (match_itag && (str.Find(tag) == -1)) {
				continue;
			}

#ifdef _DEBUG
			LOG2FILE(_T("	trying \'%s\'"), str);
#endif

			CMapStringToString UrlFields;
			{
				// extract fields/params from url
				CString templateStr = str;
				CAtlList<CString> sl2;

				CString tmp;
				for (int i = 0; i < templateStr.GetLength(); i++) {
					if (templateStr[i] == '&' || templateStr[i] == '?') {
						sl2.AddTail(tmp);
						tmp.Empty();
					} else {
						tmp += templateStr[i];
					}
				}
				if (!tmp.IsEmpty()) {
					sl2.AddTail(tmp);
				}

				POSITION pos = sl2.GetHeadPosition();
				while (pos) {
					CString tmp = sl2.GetNext(pos);
					int sepPos = tmp.Find('=');
					if (sepPos <= 0 || sepPos == tmp.GetLength()-1) {
						continue;
					}

					CString TagKey		= tmp.Left(sepPos);
					CString TagValue	= tmp.Mid(sepPos + 1);

					UrlFields[TagKey] = TagValue;
#ifdef _DEBUG
					LOG2FILE(_T("			==> Tag \'%s\' = \'%s\'"), TagKey, TagValue);
#endif
				}
			}

			CString itagValueStr;
			UrlFields.Lookup(_T("itag"), itagValueStr);
			if (itagValueStr.IsEmpty()) {
				continue;
			}

			CString ext;
			// format of the video - http://en.wikipedia.org/wiki/YouTube#Quality_and_codecs
			int itagValue = 0;
			if (_stscanf_s(itagValueStr, _T("%d"), &itagValue) == 1) {
				YOUTUBE_PROFILES youtubePtofile = getProfile(itagValue);
				if (youtubePtofile.iTag == 0
					|| (youtubePtofile.Container == _T("WebM") && !match_itag)
					|| youtubePtofile.Profile == _T("3D")) {
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

			CString url = UrlFields[_T("url")] + _T("?");

			CString sparams = UrlFields[_T("sparams")];
			CAtlList<CString> slparams;
			Explode(sparams, slparams, ',');
			{
				slparams.AddTail(_T("sparams"));
				slparams.AddTail(_T("mt"));
				slparams.AddTail(_T("sver"));
				slparams.AddTail(_T("mv"));
				slparams.AddTail(_T("ms"));
				slparams.AddTail(_T("fexp"));
				slparams.AddTail(_T("key"));
				slparams.AddTail(_T("sig"));

				/*
				CString tagTmp;
				UrlFields.Lookup(_T("newshard"), tagTmp);
				if (!tagTmp.IsEmpty()) {
					slparams.AddTail(_T("newshard"));
				}
				*/
			}

			POSITION pos = slparams.GetHeadPosition();
			while (pos) {
				CString param = slparams.GetNext(pos);
				if (param == _T("sig")) {
					url.AppendFormat(_T("signature=%s&"), UrlFields[param]);
				} else {
					url.AppendFormat(_T("%s=%s&"), param, UrlFields[param]);
				}
			}
			url.Trim(_T("&"));

#ifdef _DEBUG
			LOG2FILE(_T("final url = \'%s\'"), url);
			LOG2FILE(_T("------"), url);
#endif

			Title.Replace(ext, _T(""));

			if (out_Title) {
				*out_Title = Title + ext;
			}
			if (out_Author) {
				*out_Author = Author;
			}

			return url;
		}

		if (match_itag) {
			match_itag = false;
			goto again;
		}
	}

	return fn;
}

bool PlayerYouTubePlaylistCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if ((tmp_fn.Find(YOUTUBE_URL) != -1 && tmp_fn.Find(_T("&list=")) != -1) || tmp_fn.Find(YOUTUBE_PL_URL) != -1) {
		return 1;
	}

	return 0;
}

CString PlayerYouTubePlaylist(CString fn)
{
	if (PlayerYouTubePlaylistCheck(fn)) {

		char* final	= NULL;

		HINTERNET f, s = InternetOpen(L"MPC-BE Youtube Downloader", 0, NULL, NULL, 0);
		if (s) {
			f = InternetOpenUrl(s, fn, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (f) {
				char *out			= NULL;
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

					if (strstr(out, MATCH_START)) {
						break;
					}
				} while (dwBytesRead);

				final = DNew char[dataSize + 1];
				memset(final, 0, dataSize + 1);
				memcpy(final, out, dataSize);
				delete [] out;

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);
		}

		if (!final) {
			return fn;
		}

		CString Playlist = _T(""), Video = _T(""), Title = _T("");
		int t_start = 0, t_stop = 0;
		char sep1[32], sep2[] = "title=\"";

		if (fn.Find(_T("&list=")) < 0) {
			strcpy(sep1, "<a href=\"/watch?v=");
		} else {
			strcpy(sep1, "\" href=\"/watch?v=");
		}

		while ((final = strstr(final, sep1)) != NULL) {

			final += strlen(sep1);

			t_stop = strpos(final, "&");
			if (t_stop > 0) {
				char* str = DNew char[t_stop + 1];
				memset(str, 0, t_stop + 1);
				memcpy(str, final, t_stop);

				Video = CString(str);

				delete [] str;
			}

			final += t_stop;

			t_start = strpos(final, sep2) + strlen(sep2);
			if (t_start > 0 && t_start < 64) {
				t_stop = strpos(final + t_start, "\"");
				if (t_stop > 0) {
					char* str = DNew char[t_stop + 1];
					memset(str, 0, t_stop + 1);
					memcpy(str, final + t_start, t_stop);

					if (str[0] == '[' && str[strlen(str) - 1] == ']') {
					} else {
						Title = PlayerYouTubeTitle(str);
					}

					delete [] str;
				}
			}

			final += (t_start + t_stop);

			if (!Video.IsEmpty() && !Title.IsEmpty()) {

				Playlist.Append(_T("#EXTINF:-1,"));
				Playlist.Append(Title);
				Playlist.Append(_T("\r\nhttp"));
				Playlist.Append(YOUTUBE_URL);
				Playlist.Append(_T("v="));
				Playlist.Append(Video);
				Playlist.Append(_T("\r\n"));

				Video = _T("");
				Title = _T("");
			}
		}

		TCHAR out_file[_MAX_PATH];

		if (GetTempPath(_MAX_PATH, out_file)) {

			CString out_tmp(out_file);
			out_tmp.Append(_T("mpc_youtube.m3u"));
			TCHAR *wchr = out_tmp.GetBuffer();
			wcsncpy(out_file, wchr, out_tmp.GetLength());
			out_tmp.ReleaseBuffer();

			CStdioFile fout;

			if (fout.Open(out_file, CFile::modeCreate|CFile::modeWrite|CFile::shareDenyWrite|CFile::typeBinary)) {

				CStringA ptr(Playlist);
				const char* pt = (LPCSTR)ptr;
				fout.Write(pt, strlen(pt));
				fout.Close();
			}
		}

		return out_file;
	}

	return fn;
}

CString PlayerYouTubeTitle(char* title)
{
	CString Title = UTF8To16(title);
	Title = Title.TrimLeft(_T(".")).TrimRight(_T("."));

	Title.Replace(_T(":"), _T(" -"));
	Title.Replace(_T("|"), _T("-"));
	Title.Replace(_T("—"), _T("-"));
	Title.Replace(_T("--"), _T("-"));
	Title.Replace(_T("  "), _T(" "));

	Title.Replace(_T("&quot;"), _T("\""));
	Title.Replace(_T("&amp;"), _T("&"));
	Title.Replace(_T("&#39;"), _T("'"));
	Title.Replace(_T("&#039;"), _T("'"));

	return Title;
}
