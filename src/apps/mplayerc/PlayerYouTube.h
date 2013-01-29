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

#define YOUTUBE_URL		_T("youtube.com/watch?")
#define YOUTU_BE_URL	_T("youtu.be/")

#define GET_VIDEO_URL	_T("http://www.youtube.com/get_video_info?video_id=%s")

#define MATCH_START		"url_encoded_fmt_stream_map="
#define MATCH_END		"\\u0026amp"

#define	MATCH_TITLE		_T("&title=")
#define	MATCH_AUTHOR	_T("&author=")
#define MATCH_URL		_T("url=http://")

typedef struct {
	const int		iTag;
	const LPCTSTR	Container;
	const LPCTSTR	Profile;
	const LPCTSTR	Resolution;
} YOUTUBE_PROFILES;

static const YOUTUBE_PROFILES youtubeProfiles[] = {
	{38,	_T("MP4"),	_T("High"),			_T("3072p")},
	{37,	_T("MP4"),	_T("High"),			_T("1080p")},
	{22,	_T("MP4"),	_T("High"),			_T("720p")},
	{18,	_T("MP4"),	_T("Baseline"),		_T("360p")},
	{35,	_T("FLV"),	_T("Main"),			_T("480p")},
	{34,	_T("FLV"),	_T("Main"),			_T("360p")},
	{6,		_T("FLV"),	_T("N/A"),			_T("270p")},
	{5,		_T("FLV"),	_T("N/A"),			_T("240p")},
	{36,	_T("3GP"),	_T("Simple"),		_T("240p")},
	{17,	_T("3GP"),	_T("Simple"),		_T("144p")},
	{13,	_T("3GP"),	_T("N/A"),			_T("N/A")},
	{46,	_T("WebM"),	_T("N/A"),			_T("1080p")},
	{45,	_T("WebM"),	_T("N/A"),			_T("720p")},
	{44,	_T("WebM"),	_T("N/A"),			_T("480p")},
	{43,	_T("WebM"),	_T("N/A"),			_T("360p")},
	{82,	_T("MP4"),	_T("3D"),			_T("360p")},
	{83,	_T("MP4"),	_T("3D"),			_T("240p")},
	{84,	_T("MP4"),	_T("3D"),			_T("720p")},
	{85,	_T("MP4"),	_T("3D"),			_T("520p")},
	{100,	_T("WebM"),	_T("3D"),			_T("360p")},
	{101,	_T("WebM"),	_T("3D"),			_T("360p")},
	{102,	_T("WebM"),	_T("3D"),			_T("720p")},
	{120,	_T("FLV"),	_T("Main@L3.1"),	_T("720p")}
};

static const YOUTUBE_PROFILES youtubeProfileEmpty = {0, _T(""), _T("N/A"), _T("")};

static YOUTUBE_PROFILES getProfile(int iTag) {
	for (int i = 0; i < _countof(youtubeProfiles); i++)
		if (iTag == youtubeProfiles[i].iTag) {
			return youtubeProfiles[i];
	}

	return youtubeProfileEmpty;
}

static DWORD strpos(char* h, char* n)
{
	char* p = strstr(h, n);

	if (p) {
		return p - h;
	}

	return 0;
}

CString PlayerYouTube(CString fn, CString* out_Title, CString* out_Author);
