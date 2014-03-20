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

typedef struct {
	const int		iTag;
	const LPCTSTR	Container;
	const int		Resolution;

} YOUTUBE_PROFILES;

static const YOUTUBE_PROFILES youtubeProfiles[] = {
//	{38,	_T("MP4"),	3072}, // High
//	{37,	_T("MP4"),	1080}, // High
	{22,	_T("MP4"),	720 }, // High
	{18,	_T("MP4"),	360 }, // Baseline
	{35,	_T("FLV"),	480 }, // Main
	{34,	_T("FLV"),	360 }, // Main
	{6,		_T("FLV"),	270 }, // N/A
	{5,		_T("FLV"),	240 }, // N/A
	{36,	_T("3GP"),	240 }, // Simple
	{17,	_T("3GP"),	144 }, // Simple
//	{13,	_T("3GP"),	144 }, // N/A
//	{46,	_T("WebM"),	1080}, // N/A
	{45,	_T("WebM"),	720 }, // N/A
	{44,	_T("WebM"),	480 }, // N/A
	{43,	_T("WebM"),	360 }, // N/A
//	{82,	_T("MP4"),	360 }, // 3D
//	{83,	_T("MP4"),	240 }, // 3D
//	{84,	_T("MP4"),	720 }, // 3D
//	{85,	_T("MP4"),	520 }, // 3D
//	{100,	_T("WebM"),	360 }, // 3D
//	{101,	_T("WebM"),	360 }, // 3D
//	{102,	_T("WebM"),	720 }, // 3D
//	{120,	_T("FLV"),	720 }, // Main@L3.1
};

static const YOUTUBE_PROFILES youtubeProfileEmpty = {0, _T(""), 0};

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
