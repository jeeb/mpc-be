/*
 * Copyright (C) 2013 Sergey "Exodus8" (rusguy6@gmail.com)
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

		char* final = NULL;
		CString url;

		for (size_t i = 0; i < 10; i++) {

			HINTERNET f, s = InternetOpen(L"MPC-BE Vimeo Downloader", 0, NULL, NULL, 0);
			if (s) {
				CString tmp_link = link;
				tmp_link.AppendFormat(_T("%d"), (int)time(0));

				f = InternetOpenUrl(s, tmp_link, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
				if (f) {
					char *out			= NULL;
					DWORD dwBytesRead	= 0;
					DWORD dataSize		= 0;

					char buffer[4096];
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead)) {
						if (strstr(buffer, "</cmd>")) {

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

		int t_start = strpos(final, "http");
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

		delete [] final;

		return url;
	}

	return fn;
}
