/*
 * (C) 2011-2014 see Authors.txt
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
#include "CUE.h"
#include "text.h"
#include "../Subtitles/TextFile.h"

static CString GetCUECommand(CString& ln)
{
	CString c;
	ln.Trim();
	int i = ln.Find(' ');
	if (i < 0) {
		c = ln;
		ln.Empty();
	} else {
		c = ln.Left(i);
		ln.Delete(0, i+1);
		ln.TrimLeft();
	}
	return c;
}

void MakeCUETitle(CString &Title, CString title, CString performer, UINT trackNum)
{
	if (performer.GetLength() > 0 && title.GetLength() > 0) {
		Title.Format(_T("%02d. %s - %s"), trackNum, performer, title);
	} else if (performer.GetLength() > 0) {
		Title.Format(_T("%02d. %s"), trackNum, performer);
	} else if (title.GetLength() > 0) {
		Title.Format(_T("%02d. %s"), trackNum, title);
	}

	if (trackNum == UINT_MAX && Title.GetLength() > 0) {
		Title.Delete(0, Title.Find('.') + 2);
	}
}

bool ParseCUESheet(CString cueData, CAtlList<Chapters> &ChaptersList, CString& Title, CString& Performer)
{
	BOOL fAudioTrack;
	int track_no = -1, /*index, */index_cnt = 0;
	REFERENCE_TIME rt = _I64_MIN;
	CString TrackTitle;
	CString title, performer;

	Title.Empty();
	Performer.Empty();

	CAtlList<CString> cuelines;
	Explode(cueData, cuelines, '\n');

	if (cuelines.GetCount() <= 1) {
		return false;
	}

	while (cuelines.GetCount()) {
		CString cueLine	= cuelines.RemoveHead().Trim();
		CString cmd		= GetCUECommand(cueLine);

		if (cmd == _T("TRACK")) {
			if (rt != _I64_MIN && track_no != -1 && index_cnt) {
				MakeCUETitle(TrackTitle, title, performer, track_no);
				if (!TrackTitle.IsEmpty()) {
					ChaptersList.AddTail(Chapters(TrackTitle, rt));
				}
			}
			rt = _I64_MIN;
			index_cnt = 0;

			TCHAR type[256];
			swscanf_s(cueLine, _T("%d %s"), &track_no, type, _countof(type)-1);
			fAudioTrack = (wcscmp(type, _T("AUDIO")) == 0);
			TrackTitle.Format(_T("Track %02d"), track_no);
		} else if (cmd == _T("TITLE")) {
			cueLine.Trim(_T(" \""));
			title = cueLine;

			if (track_no == -1) {
				Title = title;
			}
		} else if (cmd == _T("PERFORMER")) {
			cueLine.Trim(_T(" \""));
			performer = cueLine;

			if (track_no == -1) {
				Performer = performer;
			}
		} else if (cmd == _T("INDEX")) {
			int idx, mm, ss, ff;
			swscanf_s(cueLine, _T("%d %d:%d:%d"), &idx, &mm, &ss, &ff);

			if (fAudioTrack) {
				index_cnt++;

				rt = MILLISECONDS_TO_100NS_UNITS((mm * 60 + ss) * 1000);
			}
		}
	}

	if (rt != _I64_MAX && track_no != -1 && index_cnt) {
		MakeCUETitle(TrackTitle, title, performer, track_no);
		if (!TrackTitle.IsEmpty()) {
			ChaptersList.AddTail(Chapters(TrackTitle, rt));
		}
	}

	if (ChaptersList.GetCount()) {
		return true;
	} else {
		return false;
	}
}

bool ParseCUESheetFile(CString fn, CAtlList<CUETrack> &CUETrackList, CString& Title, CString& Performer)
{
	CWebTextFile f;
	if (!f.Open(fn) || f.GetLength() > 32 * 1024) {
		return false;
	}

	if (f.GetEncoding() == CTextFile::ASCII) {
		f.SetEncoding(CTextFile::ANSI);
	}

	Title.Empty();
	Performer.Empty();

	CString cueLine;

	CAtlArray<CString> sFilesArray;
	CAtlArray<CString> sTrackArray;
	while (f.ReadString(cueLine)) {
		CString cmd = GetCUECommand(cueLine);
		if (cmd == L"TRACK") {
			sTrackArray.Add(cueLine);
		} else if (cmd == L"FILE" && cueLine.Find(L" WAVE") > 0) {
			cueLine.Replace(L" WAVE", L"");
			cueLine.Trim(L" \"");
			sFilesArray.Add(cueLine);
		}
	};

	if (sTrackArray.IsEmpty() || sFilesArray.IsEmpty()) {
		return false;
	}

	BOOL bMultiple = sTrackArray.GetCount() == sFilesArray.GetCount();

	CString sTitle, sPerformer, sFileName, sFileName2;
	REFERENCE_TIME rt = _I64_MIN;
	BOOL fAudioTrack = FALSE;
	UINT trackNum = 0;

	UINT idx = 0;
	f.Seek(0, CFile::SeekPosition::begin);
	while (f.ReadString(cueLine)) {
		CString cmd = GetCUECommand(cueLine);

		if (cmd == L"TRACK") {
			if (rt != _I64_MIN) {
				CString fName = bMultiple ? sFilesArray[idx++] : sFilesArray.GetCount() == 1 ? sFilesArray[0] : sFileName;
				CUETrackList.AddTail(CUETrack(fName, rt, trackNum, sTitle, sPerformer));
			}
			rt = _I64_MIN;
			sFileName = sFileName2;

			TCHAR type[256] = { 0 };
			trackNum = 0;
			fAudioTrack = FALSE;
			if (2 == swscanf_s(cueLine, L"%d %s", &trackNum, type, _countof(type) - 1)) {
				fAudioTrack = (wcscmp(type, L"AUDIO") == 0);
			}
		} else if (cmd == L"TITLE") {
			cueLine.Trim(L" \"");
			sTitle = cueLine;

			if (sFileName2.IsEmpty()) {
				Title = sTitle;
			}
		} else if (cmd == L"PERFORMER") {
			cueLine.Trim(L" \"");
			sPerformer = cueLine;

			if (sFileName2.IsEmpty()) {
				Performer = sPerformer;
			}
		} else if (cmd == L"FILE" && cueLine.Find(L" WAVE") > 0) {
			cueLine.Replace(L" WAVE", L"");
			cueLine.Trim(L" \"");
			if (sFileName.IsEmpty()) {
				sFileName = sFileName2 = cueLine;
			} else {
				sFileName2 = cueLine;
			}
		} else if (cmd == L"INDEX") {
			int idx, mm, ss, ff;
			if (4 == swscanf_s(cueLine, L"%d %d:%d:%d", &idx, &mm, &ss, &ff) && fAudioTrack) {
				rt = MILLISECONDS_TO_100NS_UNITS((mm * 60 + ss) * 1000);
			}
		}
	}

	if (rt != _I64_MIN) {
		CString fName = bMultiple ? sFilesArray[idx] : sFilesArray.GetCount() == 1 ? sFilesArray[0] : sFileName2;
		CUETrackList.AddTail(CUETrack(fName, rt, trackNum, sTitle, sPerformer));
	}

	return CUETrackList.GetCount() > 0;
}