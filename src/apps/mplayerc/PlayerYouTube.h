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

#pragma once

#include <afxinet.h>

#define YOUTUBE_MP_URL	"://www.youtube.com/"
#define YOUTUBE_PL_URL	_T("://www.youtube.com/playlist?")
#define YOUTUBE_URL		_T("://www.youtube.com/watch?")
#define YOUTU_BE_URL	_T("://youtu.be/")
#define VIMEO_URL		_T("://vimeo.com/")

#define ENABLE_YOUTUBE_3D	0
#define ENABLE_YOUTUBE_DASH	0

enum ytype {
	y_unknown,
	y_mp4,
	y_webm,
	y_flv,
	y_3gp,
	y_3d_mp4,
	y_3d_webm,
	y_apple_live,
	y_dash_mp4_video,
	y_dash_mp4_audio,
	y_dash_webm_video,
	y_dash_webm_audio,
};

typedef struct {
	const int		iTag;
	const ytype     type;
	const int		quality;
	LPCTSTR	ext;
} YOUTUBE_PROFILES;

static const YOUTUBE_PROFILES youtubeProfiles[] = {
	{22,	y_mp4,				 720,	_T("mp4") },
	{18,	y_mp4,				 360,	_T("mp4") },
	{43,	y_webm,				 360,	_T("webm")},
	{5,		y_flv,				 240,	_T("flv") },
	{36,	y_3gp,				 240,	_T("3gp") },
	{17,	y_3gp,				 144,	_T("3gp") },
#if ENABLE_YOUTUBE_3D
	{84,	y_3d_mp4,			 720,	_T("mp4") },
	{83,	y_3d_mp4,			 480,	_T("mp4") },
	{82,	y_3d_mp4,			 360,	_T("mp4") },
	{100,	y_3d_webm,			 360,	_T("webm")},
#endif
#if ENABLE_YOUTUBE_DASH
	{137,	y_dash_mp4_video,	1080,	_T("mp4") },
	{136,	y_dash_mp4_video,	 720,	_T("mp4") },
	{135,	y_dash_mp4_video,	 480,	_T("mp4") },
	{134,	y_dash_mp4_video,	 360,	_T("mp4") },
	{133,	y_dash_mp4_video,	 240,	_T("mp4") },
	{160,	y_dash_mp4_video,	 144,	_T("mp4") },
	{140,	y_dash_mp4_audio,	 128,	_T("m4a") },
	{248,	y_dash_webm_video,	1080,	_T("webm")},
	{247,	y_dash_webm_video,	 720,	_T("webm")},
	{244,	y_dash_webm_video,	 480,	_T("webm")},
	{243,	y_dash_webm_video,	 360,	_T("webm")},
	{242,	y_dash_webm_video,	 240,	_T("webm")},
	{171,	y_dash_webm_audio,	  48,	_T("webm")},
#endif
};

static const YOUTUBE_PROFILES youtubeProfileEmpty = {0, y_unknown, 0};

static DWORD strpos(char* h, char* n)
{
	char* p = strstr(h, n);

	if (p) {
		return p - h;
	}

	return 0;
}

bool PlayerYouTubeCheck(CString fn);
bool PlayerYouTubePlaylistCheck(CString fn);
CString PlayerYouTube(CString fn, CString* out_Title, CString* out_Author);
CString PlayerYouTubePlaylist(CString fn, bool type);
CString PlayerYouTubePlaylistCreate();
void PlayerYouTubePlaylistDelete();
CString PlayerYouTubeGetTitle(CString fn);
CString PlayerYouTubeSearchTitle(char* final);
CString PlayerYouTubeReplaceTitle(char* title);
