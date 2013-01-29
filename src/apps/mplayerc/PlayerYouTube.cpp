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

CString PlayerYouTube(CString fn, CString* out_Title, CString* out_Author)
{
	if (out_Title) {
		*out_Title = _T("");
	}
	if (out_Author) {
		*out_Author = _T("");
	}

	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(YOUTUBE_URL) != -1 || tmp_fn.Find(YOUTU_BE_URL) != -1) {
		CString vidId;
		if (tmp_fn.Find(YOUTUBE_URL) >= 0) {
			int v	= fn.Find(_T("?v=")) + 3;
			int end	= fn.Find(_T("&"), v);
			if (end == -1) {
				end = fn.GetLength();
			}
			vidId = fn.Mid(v, end - v);
		} else {
			int v = fn.ReverseFind('/') + 1;
			vidId = fn.Mid(v, fn.GetLength() - v);
		}

		CString newUrl;
		newUrl.Format(GET_VIDEO_URL, vidId);

		char* final	= NULL;
		HINTERNET f, s = InternetOpen(L"MPC-BE Youtube Downloader", 0, NULL, NULL, 0);
		if (s) {
			f = InternetOpenUrl(s, newUrl, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
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

		CString str = UTF8To16(UrlDecode(UrlDecode(CStringA(final))));
		delete [] final;

#ifdef _DEBUG
		LOG2FILE(_T("------"));
		LOG2FILE(_T("Youtube parser"));
#endif

		CString Title, Author;

		if (str.Find(_T(MATCH_START)) == -1) {
			// open original html page
			int match_start	= 0;
			int match_len	= 0;

			final = NULL;
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
				if (strpos(final, "http://www.youtube.com")) {
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

			char *tmp = DNew char[match_len + 1];
			memset(tmp, 0, match_len + 1);
			memcpy(tmp, final + match_start, match_len);

			str = UTF8To16(UrlDecode(UrlDecode(CStringA(tmp))));
			delete [] tmp;

#ifdef _DEBUG
			LOG2FILE(_T("Source = \'%s\'"), str);
#endif
		} else {

			// get name(title) for output filename
			int tpos = CString(str).MakeLower().Find(MATCH_TITLE);
			if (tpos >= 0) {
				tpos += CString(MATCH_TITLE).GetLength();
				int tposend = str.Find(_T("&"), tpos);
				if (tposend == -1) {
					tposend = str.GetLength();
				}
				Title = str.Mid(tpos, tposend - tpos);

				Title.Replace(_T("+"), _T(" "));
			}

			// get Author of clip
			int apos = CString(str).MakeLower().Find(MATCH_AUTHOR);
			if (apos >= 0) {
				apos += CString(MATCH_AUTHOR).GetLength();
				int aposend = str.Find(_T("&"), apos);
				if (aposend == -1) {
					aposend = str.GetLength();
				}
				Author = str.Mid(apos, aposend - apos);

				Author.Replace(_T("+"), _T(" "));
			}

#ifdef _DEBUG
			LOG2FILE(_T("Source = \'%s\'"), str);
#endif

			str.Delete(0, str.Find(_T(MATCH_START)) + CString(_T(MATCH_START)).GetLength());
		}

		if (Title.IsEmpty()) {
			Title = _T("video");
		}

#ifdef _DEBUG
		LOG2FILE(_T("	==> \'%s\'"), str);
#endif

		CAtlList<CString> sl;
		CString tmp;
		for (int i = 0; i < str.GetLength(); i++) {
			if ((i+1 < str.GetLength()) && str[i] == ',' && str[i+1] != '+') {
				if (tmp.Find(MATCH_URL) >= 0) {
					sl.AddTail(tmp);
#ifdef _DEBUG
					LOG2FILE(_T("		=== >\'%s\'"), tmp);
#endif
				}
				tmp.Empty();
			} else {
				tmp += str[i];
			}
		}
		if (!tmp.IsEmpty() && tmp.Find(MATCH_URL) >= 0) {
			sl.AddTail(tmp);
#ifdef _DEBUG
			LOG2FILE(_T("		=== >\'%s\'"), tmp);
#endif
		}

		CString tag; tag.Format(_T("itag=%d"), AfxGetAppSettings().iYoutubeTag);
		boolean match_itag = AfxGetAppSettings().iYoutubeTag != 0;

again:

		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			str = sl.GetNext(pos);
			str.Trim(_T("&,="));

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
					if (templateStr[i] == '&' || templateStr[i] == '?' || ((i+1 < templateStr.GetLength()) && templateStr[i] == ',' && templateStr[i+1] != '+')) {
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
					tmp = sl2.GetNext(pos);
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
			sparams.Replace(_T("%2C"), _T(";"));
			CAtlList<CString> slparams;
			Explode(sparams, slparams, ';');
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
			url.Append(_T("&title="));
			url.Append(CString(UrlEncode(UTF16To8(Title))));
			url.Append(ext);

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

		return fn;
	}

	return fn;
}
