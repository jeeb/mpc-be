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

#include "stdafx.h"
#include "atl/atlrx.h"
#include "PlayerYouTube.h"
#include "PlayerVimeo.h"

#include "../../DSUtil/MPCSocket.h"
#include "../../DSUtil/Log.h"

#define MATCH_STREAM_MAP_START	"\"url_encoded_fmt_stream_map\": \""
#define MATCH_WIDTH_START		"meta property=\"og:video:width\" content=\""
#define MATCH_DASHMPD_START		"\"dashmpd\": \"http:\\/\\/www.youtube.com\\/api\\/manifest\\/dash\\/"
#define MATCH_END				"\""

#define MATCH_PLAYLIST_ITEM_START	"<li class=\"yt-uix-scroller-scroll-unit \""

const YOUTUBE_PROFILES* getProfile(int iTag) {
	for (int i = 0; i < _countof(youtubeProfiles); i++)
		if (iTag == youtubeProfiles[i].iTag) {
			return &youtubeProfiles[i];
	}

	return &youtubeProfileEmpty;
}

bool SelectBestProfile(int &itag_final, CString &ext_final, int itag_current, const YOUTUBE_PROFILES* sets)
{
	const YOUTUBE_PROFILES* current = getProfile(itag_current);

	if (current->iTag <= 0
			|| current->Container != sets->Container
			|| current->Resolution > sets->Resolution) {
		return false;
	}

	if (itag_final != 0) {
		const YOUTUBE_PROFILES* fin = getProfile(itag_final);
		if (current->Resolution < fin->Resolution) {
			return false;
		}
	}

	itag_final = current->iTag;
	ext_final = '.' + CString(current->Container).MakeLower();

	return true;
}

bool PlayerYouTubeCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(_T(YOUTUBE_MP_URL)) != -1 && (tmp_fn.Find(_T("watch?")) < 0 || tmp_fn.Find(_T("playlist?")) != -1 || tmp_fn.Find(_T("&list=")) != -1)) {
		return 0;
	}
	if (tmp_fn.Find(YOUTUBE_URL) != -1 || tmp_fn.Find(YOUTU_BE_URL) != -1) {
		return 1;
	}

	return 0;
}

bool PlayerYouTubePlaylistCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	tmp_fn.Replace(_T("https"), _T(""));
	tmp_fn.Replace(_T("http"), _T(""));

	if (tmp_fn == _T(YOUTUBE_MP_URL) || (tmp_fn.Find(_T(YOUTUBE_MP_URL)) != -1 && tmp_fn.Find(_T("channel/")) != -1)) {
		return 0;
	}
	if (tmp_fn.Find(YOUTUBE_PL_URL) != -1 || (tmp_fn.Find(YOUTUBE_URL) != -1 && tmp_fn.Find(_T("&list=")) != -1)) {
		return 1;
	}
	if (tmp_fn.Find(_T(YOUTUBE_MP_URL)) != -1 && tmp_fn.Find(_T("watch?")) < 0) {
		return 1;
	}

	return 0;
}

CString PlayerYouTube(CString fn, CString* out_Title, CString* out_Author)
{
	if (out_Title) {
		(*out_Title).Empty();
	}
	if (out_Author) {
		(*out_Author).Empty();
	}

	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(VIMEO_URL) != -1) {
		if (out_Title) {
			*out_Title = PlayerYouTubeGetTitle(fn);
		}
		return PlayerVimeo(fn);
	}

	if (PlayerYouTubeCheck(fn)) {

#ifdef _DEBUG
		LOG2FILE(_T("------"));
		LOG2FILE(_T("Youtube parser"));
#endif

		CString Author;

		char* data = NULL;
		DWORD dataSize = 0;

		int stream_map_start = 0;
		int stream_map_len = 0;

		int video_width_start = 0;
		int video_width_len = 0;

		int nMaxWidth = 0;

		AppSettings& sApp = AfxGetAppSettings();
		const YOUTUBE_PROFILES* youtubeSets = getProfile(sApp.iYoutubeTag);
		if (youtubeSets->iTag == 0) {
			youtubeSets = getProfile(22);
		}

#if 0
		BOOL bIsFullHD = FALSE;
		if (sApp.iYoutubeTag == 37) {
			// Full HD resolution, format .MP4
			match_itag	= FALSE;
			bIsFullHD	= TRUE;
		}
#endif

		HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
		if (s) {
			CString link = fn;
			if (link.Find(YOUTU_BE_URL) != -1) {
				link.Replace(YOUTU_BE_URL, YOUTUBE_URL);
				link.Replace(_T("watch?"), _T("watch?v="));
			}
			f = InternetOpenUrl(s, link, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (f) {
				char buffer[4096];
				DWORD dwBytesRead = 0;

				do {
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
						break;
					}

					data = (char*)realloc(data, dataSize + dwBytesRead + 1);
					memcpy(data + dataSize, buffer, dwBytesRead);
					dataSize += dwBytesRead;
					data[dataSize] = 0;

					// url_encoded_fmt_stream_map
					if (!stream_map_start && (stream_map_start = strpos(data, MATCH_STREAM_MAP_START)) != 0) {
						stream_map_start += strlen(MATCH_STREAM_MAP_START);
					}
					if (stream_map_start && !stream_map_len) {
						stream_map_len = strpos(data + stream_map_start, MATCH_END);
					}

					// <meta property="og:video:width" content="....">
					if (!video_width_start && (video_width_start = strpos(data, MATCH_WIDTH_START)) != 0) {
						video_width_start += strlen(MATCH_WIDTH_START);
					}
					if (video_width_start && !video_width_len) {
						video_width_len = strpos(data + video_width_start, MATCH_END);
					}

#if 0
					// detect MAX resolution for this video
					if (bIsFullHD && video_width_len && !nMaxWidth) {
						char* tmp = DNew char[video_width_len + 1];
						memcpy(tmp, data + video_width_start, video_width_len);
						tmp[video_width_len] = 0;

						if (sscanf_s(tmp, "%d", &nMaxWidth) != 1) {
							nMaxWidth = -1;
						}
						delete[] tmp;
					}
#endif

					// optimization - to not download the entire page
					if (stream_map_len) {
						if (nMaxWidth != 1920) {
							break;
						}
					}
				} while (dwBytesRead);

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);
		}

		if (!data || !f || !s) {
			return fn;
		}

		if (!stream_map_len) {
			if (strstr(data, YOUTUBE_MP_URL)) {
				// This is looks like Youtube page, but this page doesn't contains necessary information about video, so may be you have to register on google.com to view it.
				fn.Empty();
			}
			free(data);
			return fn;
		}

		CString Title = PlayerYouTubeSearchTitle(data);

#if 0
		DWORD dashmpd_start	= strpos(data, MATCH_DASHMPD_START);
		if (bIsFullHD && dashmpd_start && nMaxWidth == 1920) {
			DWORD dashmpd_len = strpos(data + dashmpd_start + strlen(MATCH_DASHMPD_START), MATCH_END);
			if (dashmpd_len) {
				dashmpd_start	+= strlen(MATCH_DASHMPD_START);
				char* dashmpd	= DNew char[dashmpd_len + 1];
				memset(dashmpd, 0, dashmpd_len + 1);
				memcpy(dashmpd, data + dashmpd_start, dashmpd_len);

				CString str_dashmpd = UTF8ToString(UrlDecode(UrlDecode(CStringA(dashmpd))));
				str_dashmpd.Replace(L"\\/", L"&");
				int fpos = 0;

				CString str_dashmpd_clear;
				for (int i = 0; i < str_dashmpd.GetLength(); i++) {
					TCHAR c = str_dashmpd[i];
					if (c == '&') {
						fpos++;
						if (fpos % 2) {
							c = '=';
						}
					}

					str_dashmpd_clear.AppendChar(c);
				}
				delete [] dashmpd;

				CString s_url = L"http://www.youtube.com/videoplayback?";
				s_url.AppendFormat(L"%s&ratebypass=yes&itag=%d", str_dashmpd_clear, sApp.iYoutubeTag);

				BOOL bValidateUrl = FALSE;
				CMPCSocket socket;
				if (socket.Create()) {
					socket.SetTimeOut(3000);
					if (socket.Connect(s_url, TRUE)) {
						bValidateUrl = TRUE;
					}

					socket.Close();
				}

				if (bValidateUrl) {

					if (out_Title) {
						CString ext = L".mp4";
						Title.Replace(ext, _T(""));
						*out_Title = Title + ext;
					}
					if (out_Author) {
						*out_Author = Author;
					}

#ifdef _DEBUG
					LOG2FILE(_T("final url = \'%s\'"), s_url);
					LOG2FILE(_T("------"));
#endif
					free(data);
					return s_url;
				}
			}
		}
#endif

		char *tmp = DNew char[stream_map_len + 1];
		memcpy(tmp, data + stream_map_start, stream_map_len);
		tmp[stream_map_len] = 0;
		free(data);

		CStringA strA = CStringA(tmp);
		delete[] tmp;
		strA.Replace("\\u0026", "&");
		//str.Replace(_T("\\u0022"), _T("\""));
		//str.Replace(_T("\\u0027"), _T("'"));
		//str.Replace(_T("\\u003c"), _T("<"));
		//str.Replace(_T("\\u003e"), _T(">"));

		CString final_url;
		CString final_ext;
		int final_itag = 0;


		//str = UTF8ToString(UrlDecode(UrlDecode(CStringA(tmp))));

		CAtlList<CStringA> linesA;
		Explode(strA, linesA, ',');

		POSITION posLine = linesA.GetHeadPosition();
		while (posLine) {
			CStringA &lineA = linesA.GetNext(posLine);

			int itag = 0;
			CStringA url;
			CString ext;
			int resolution = 0;

			CAtlList<CStringA> paramsA;
			Explode(lineA, paramsA, '&');

			POSITION posParam = paramsA.GetHeadPosition();
			while (posParam) {
				CStringA &paramA = paramsA.GetNext(posParam);

				int k = paramA.Find('=');
				if (k >= 0) {
					CStringA paramHeader = paramA.Left(k);
					CStringA paramValue = paramA.Mid(k + 1);

					// "quality", "fallback_host", "url", "itag", "type"
					 if (paramHeader == "url") {
						url = UrlDecode(UrlDecode(paramValue));
					} else if (paramHeader == "itag") {
						if (sscanf_s(paramValue, "%d", &itag) != 1) {
							itag = 0;
						}
					}
				}
			}

			if (itag) {
				if (SelectBestProfile(final_itag, final_ext, itag, youtubeSets)) {
					final_url = url;
				}
			}
		}

		if (out_Title) {
			Title.Replace(final_ext, _T(""));
			*out_Title = Title + final_ext;
		}
		if (out_Author) {
			*out_Author = Author;
		}

		if (final_url.GetLength() > 0) {
			return final_url;
		}
	}

	return fn;
}

CString PlayerYouTubePlaylist(CString fn, bool type)
{
	if (PlayerYouTubePlaylistCheck(fn)) {

		char* data = NULL;
		DWORD dataSize = 0;

		HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
		if (s) {
			f = InternetOpenUrl(s, fn, NULL, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (f) {
				char buffer[4096];
				DWORD dwBytesRead = 0;

				do {
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
						break;
					}

					data = (char*)realloc(data, dataSize + dwBytesRead + 1);
					memcpy(data + dataSize, buffer, dwBytesRead);
					dataSize += dwBytesRead;
					data[dataSize] = 0;

					if (/*strstr(data, "id=\"player\"") || */strstr(data, "id=\"footer\"")) {
						break;
					}
				} while (dwBytesRead);

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);
		}

		if (!data || !f || !s) {
			return fn;
		}

		CString Playlist = L"#EXTM3U\n\n";

		char* block = data;
		while ((block = strstr(block, MATCH_PLAYLIST_ITEM_START)) != NULL) {
			block += strlen(MATCH_PLAYLIST_ITEM_START);

			int block_len = strpos(block, ">");
			if (block_len) {
				char* tmp = DNew char[block_len + 1];
				memcpy(tmp, block, block_len);
				tmp[block_len] = 0;
				CString item = UTF8ToString(CStringA(tmp));
				delete[] tmp;

				CString data_video_id;
				int data_index = 0;
				CString data_video_username;
				CString data_video_title;

				CAtlRegExp<> re;
				CAtlREMatchContext<> mc;
				REParseError pe = re.Parse(L" {[a-z-]+}=\"{[^\"]+}\"");

				LPCTSTR szEnd = item.GetBuffer();
				while (re.Match(szEnd, &mc)) {
					LPCTSTR szStart;
					mc.GetMatch(0, &szStart, &szEnd);
					CString propHeader = CString(szStart, int(szEnd - szStart));

					mc.GetMatch(1, &szStart, &szEnd);
					CString propValue = CString(szStart, int(szEnd - szStart));

					// data-video-id, data-video-clip-end, data-index, data-video-username, data-video-title, data-video-clip-start.
					if (propHeader == L"data-video-id") {
						data_video_id = propValue;
					} else if (propHeader == L"data-index") {
						if (swscanf_s(propValue, L"%d", &data_index) != 1) {
							data_index = 0;
						}
					} else if (propHeader == L"data-video-username") {
						data_video_username = propValue;
					} else if (propHeader == L"data-video-title") {
						data_video_title = propValue;
					}
				}

				if (data_video_id.GetLength() > 0) {
					Playlist.AppendFormat(L"#EXTINF:-1,%s\n", data_video_title);
					Playlist.AppendFormat(L"http://www.youtube.com/watch?v=%s\n\n", data_video_id);
				}
			}
		}

		if (Playlist.GetLength() > 9) {
			if (type) {
				return Playlist;
			}

			CStdioFile fout;
			CString file = PlayerYouTubePlaylistCreate();

			if (fout.Open(file, CFile::modeCreate|CFile::modeWrite|CFile::shareDenyWrite|CFile::typeBinary)) {
				CStringA ptr(Playlist);
				const char* pt = (LPCSTR)ptr;
				fout.Write(pt, strlen(pt));
				fout.Close();

				return file;
			}
		}
	}

	return fn;
}

CString PlayerYouTubePlaylistCreate()
{
	TCHAR lpszTempPath[_MAX_PATH] = { 0 };

	CString tmpPlaylist;
	if (GetTempPath(_MAX_PATH, lpszTempPath)) {
		tmpPlaylist.AppendFormat(L"%smpc_youtube.m3u", lpszTempPath);
	}

	return tmpPlaylist;
}

void PlayerYouTubePlaylistDelete()
{
	CString file = PlayerYouTubePlaylistCreate();

	if (::PathFileExists(file)) {
		::DeleteFile(file);
	}
}

CString PlayerYouTubeGetTitle(CString fn)
{
	char* final = NULL;

	HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
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

				if (strstr(out, "</title>")) {
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

	if (!final || !f || !s) {
		return fn;
	}

	delete [] final;

	return PlayerYouTubeSearchTitle(final) + _T(".mp4");
}

CString PlayerYouTubeSearchTitle(char* final)
{
	CString Title;

	int t_start = strpos(final, "<title>");
	if (t_start > 0) {
		t_start += 7;
		int t_stop = strpos(final + t_start, "</title>");
		if (t_stop > 0) {
			char* title = DNew char[t_stop + 1];
			memset(title, 0, t_stop + 1);
			memcpy(title, final + t_start, t_stop);

			Title = PlayerYouTubeReplaceTitle(title);

			delete [] title;
		}
	}

	if (Title.IsEmpty()) {
		Title = _T("video");
	}

	return Title;
}

CString PlayerYouTubeReplaceTitle(char* title)
{
	CString Title = UTF8ToString(title);
	Title = Title.TrimLeft(_T(".")).TrimRight(_T("."));

	Title.Replace(_T(":"), _T(" -"));
	Title.Replace(_T("/"), _T("-"));
	Title.Replace(_T("|"), _T("-"));
	Title.Replace(_T("—"), _T("-"));
	Title.Replace(_T("--"), _T("-"));
	Title.Replace(_T("  "), _T(" "));
	Title.Replace(_T("\""), _T(""));
	Title.Replace(_T("* "), _T(""));
	Title.Replace(_T("*"), _T(""));

	Title.Replace(_T("&quot;"), _T(""));
	Title.Replace(_T("&amp;"), _T("&"));
	Title.Replace(_T("&#39;"), _T("'"));
	Title.Replace(_T("&#039;"), _T("'"));

	Title.Replace(_T(" - YouTube"), _T(""));
	Title.Replace(_T(" on Vimeo"), _T(""));

	return Title;
}
