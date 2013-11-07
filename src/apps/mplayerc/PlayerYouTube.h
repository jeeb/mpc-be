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
	const LPCTSTR	Profile;
	const LPCTSTR	Resolution;
	const bool		Visible;
} YOUTUBE_PROFILES;

static const YOUTUBE_PROFILES youtubeProfiles[] = {
	{38,	_T("MP4"),	_T("High"),			_T("3072p"),	true},
	{37,	_T("MP4"),	_T("High"),			_T("1080p"),	true},
	{22,	_T("MP4"),	_T("High"),			_T("720p"),		true},
	{18,	_T("MP4"),	_T("Baseline"),		_T("360p"),		true},
	{35,	_T("FLV"),	_T("Main"),			_T("480p"),		true},
	{34,	_T("FLV"),	_T("Main"),			_T("360p"),		true},
	{6,		_T("FLV"),	_T("N/A"),			_T("270p"),		true},
	{5,		_T("FLV"),	_T("N/A"),			_T("240p"),		true},
	{36,	_T("3GP"),	_T("Simple"),		_T("240p"),		true},
	{17,	_T("3GP"),	_T("Simple"),		_T("144p"),		true},
	{13,	_T("3GP"),	_T("N/A"),			_T("N/A"),		true},
	{46,	_T("WebM"),	_T("N/A"),			_T("1080p"),	true},
	{45,	_T("WebM"),	_T("N/A"),			_T("720p"),		true},
	{44,	_T("WebM"),	_T("N/A"),			_T("480p"),		true},
	{43,	_T("WebM"),	_T("N/A"),			_T("360p"),		true},
	{82,	_T("MP4"),	_T("3D"),			_T("360p"),		false},
	{83,	_T("MP4"),	_T("3D"),			_T("240p"),		false},
	{84,	_T("MP4"),	_T("3D"),			_T("720p"),		false},
	{85,	_T("MP4"),	_T("3D"),			_T("520p"),		false},
	{100,	_T("WebM"),	_T("3D"),			_T("360p"),		false},
	{101,	_T("WebM"),	_T("3D"),			_T("360p"),		false},
	{102,	_T("WebM"),	_T("3D"),			_T("720p"),		false},
	{120,	_T("FLV"),	_T("Main@L3.1"),	_T("720p"),		false}
};

static const YOUTUBE_PROFILES youtubeProfileEmpty = {0, _T(""), _T("N/A"), _T(""), false};

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

bool PlayerYouTubeCheck(CString fn);
bool PlayerYouTubePlaylistCheck(CString fn);
CString PlayerYouTube(CString fn, CString* out_Title, CString* out_Author);
CString PlayerYouTubePlaylist(CString fn, bool type);
CString PlayerYouTubePlaylistCreate();
void PlayerYouTubePlaylistDelete();
CString PlayerYouTubeGetTitle(CString fn);
CString PlayerYouTubeSearchTitle(char* final);
CString PlayerYouTubeReplaceTitle(char* title);
