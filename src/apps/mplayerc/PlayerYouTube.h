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

#define YOUTUBE_URL			_T("youtube.com/watch?")
#define YOUTUBE_FULL_URL	_T("www.youtube.com/watch?v=")
#define YOUTU_BE_URL		_T("youtu.be/")
#define YOUTU_BE_FULL_URL	_T("www.youtu.be/")

#define MATCH_START			"url_encoded_fmt_stream_map"
#define MATCH_END			"\\u0026amp"

#define URL_DELIMETER		_T("url=")

typedef struct {
	const int		iTag;
	const LPCTSTR	Container;
	const LPCTSTR	Profile;
} YOUTUBE_PROFILES;

static const YOUTUBE_PROFILES youtubeProfiles[] = {
	{5,		_T("FLV"),	_T("N/A")},
	{6,		_T("FLV"),	_T("N/A")},
	{13,	_T("3GP"),	_T("N/A")},
	{17,	_T("3GP"),	_T("Simple")},
	{18,	_T("MP4"),	_T("Baseline")},
	{22,	_T("MP4"),	_T("High")},
	{34,	_T("FLV"),	_T("Main")},
	{35,	_T("FLV"),	_T("Main")},
	{36,	_T("3GP"),	_T("Simple")},
	{37,	_T("MP4"),	_T("High")},
	{38,	_T("MP4"),	_T("High")},
	{43,	_T("WebM"),	_T("N/A")},
	{44,	_T("WebM"),	_T("N/A")},
	{45,	_T("WebM"),	_T("N/A")},
	{46,	_T("WebM"),	_T("N/A")},
	{82,	_T("MP4"),	_T("3D")},
	{83,	_T("MP4"),	_T("3D")},
	{84,	_T("MP4"),	_T("3D")},
	{85,	_T("MP4"),	_T("3D")},
	{100,	_T("WebM"),	_T("3D")},
	{101,	_T("WebM"),	_T("3D")},
	{102,	_T("WebM"),	_T("3D")},
	{120,	_T("FLV"),	_T("Main@L3.1")}
};

static const YOUTUBE_PROFILES youtubeProfileEmpty = {0, _T(""), _T("N/A")};

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

static void Explode(const CString str, const CString sep, CAtlList<CString>& arr)
{
	arr.RemoveAll();

	CString local(str);

	int pos = local.Find(sep);
	while (pos != -1) {
		CString sss = local.Left(pos);
		arr.AddTail(sss);
		local.Delete(0, pos + sep.GetLength());
		pos = local.Find(sep);
	}

	if (!local.IsEmpty()) {
		arr.AddTail(local);
	}
}

CString PlayerYouTube(CString fn, CString* out_title);
