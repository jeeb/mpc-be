/*
 * $Id: PlayerVimeo.h 2229 2013-03-08 13:27:11Z exodus8 $
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

CString PlayerVimeo(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(VIMEO_URL) != -1) {

		CString link;
		link.Append(_T("http://downloadvimeo.com/generate?url="));
		link.Append(fn);
		link.Append(_T("&random="));

		char* final	= NULL;

		for (size_t i = 0; i < 10; i++) {

			HINTERNET f, s = InternetOpen(L"MPC-BE Vimeo Downloader", 0, NULL, NULL, 0);
			if (s) {
				CString tmp_link = link;
				char tbuf[16];
				_itoa((int)time(0), tbuf, 10);
				tmp_link.Append(CString(tbuf));

				f = InternetOpenUrl(s, tmp_link, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
				if (f) {
					char *out			= NULL;
					DWORD dwBytesRead	= 0;
					DWORD dataSize		= 0;

					char buffer[4096];
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead)) {
						if (strpos(buffer, "</cmd>")) {

							char *tempData = DNew char[dataSize + dwBytesRead];
							memcpy(tempData, out, dataSize);
							memcpy(tempData + dataSize, buffer, dwBytesRead);
							delete[] out;
							out = tempData;
							dataSize += dwBytesRead;

							final = DNew char[dataSize + 1];
							memset(final, 0, dataSize + 1);
							memcpy(final, out, dataSize);
							delete [] out;
						}
					}

					InternetCloseHandle(f);
				}
				InternetCloseHandle(s);
			}

			if (final) {
				break;
			}

			Sleep(300);
		}

		if (!final) {
			return fn;
		}

		CString url;

		int t_start = strpos(final, "http://");
		if (t_start > 0) {
			int t_stop = strpos(final + t_start, "</cmd>");
			if (t_stop > 0) {
				char* tmp_url = DNew char[t_stop + 1];
				memset(tmp_url, 0, t_stop + 1);
				memcpy(tmp_url, final + t_start, t_stop);

				url = CString(tmp_url);
				url.Replace(_T("&amp;"), _T("&"));

				delete [] tmp_url;
			}
		}

		return url;
	}

	return fn;
}

CString PlayerVimeoTitle(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(VIMEO_URL) != -1) {

		char* final	= NULL;
		CString Title;

		HINTERNET f, s = InternetOpen(L"MPC-BE Vimeo Downloader", 0, NULL, NULL, 0);
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

					if (strpos(out, "</title>")) {
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

				Title.Replace(_T(" on Vimeo"), _T(""));
				Title.Replace(_T(":"), _T(" -"));
				Title.Replace(_T("|"), _T("-"));
				Title.Replace(_T("—"), _T("-"));
				Title.Replace(_T("--"), _T("-"));
				Title.Replace(_T("  "), _T(" "));

				Title.Replace(_T("&quot;"), _T("\""));
				Title.Replace(_T("&amp;"), _T("&"));
				Title.Replace(_T("&#39;"), _T("'"));
				Title.Replace(_T("&#039;"), _T("'"));

				delete [] title;
			}
		}

		return Title + _T(".mp4");
	}

	return fn;
}
