/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
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

#include <atlbase.h>
#include <atlcoll.h>
#include <winddk/ntddcdvd.h>

#include "../../../DSUtil/DSUtil.h"

class CDVDSession
{
protected:
	HANDLE m_hDrive;

	DVD_SESSION_ID m_session;
	bool BeginSession();
	void EndSession();

	BYTE m_SessionKey[5];
	bool Authenticate();

	BYTE m_DiscKey[6], m_TitleKey[6];
	bool GetDiscKey();
	bool GetTitleKey(int lba, BYTE* pKey);

public:
	CDVDSession();
	virtual ~CDVDSession();

	bool Open(LPCTSTR path);
	void Close();

	operator HANDLE() const {
		return m_hDrive;
	}
	operator DVD_SESSION_ID() const {
		return m_session;
	}

	bool SendKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData);
	bool ReadKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData, int lba = 0);
};

class CLBAFile : private CFile
{
public:
	CLBAFile();
	virtual ~CLBAFile();

	bool IsOpen();

	bool Open(LPCTSTR path);
	void Close();

	int GetLength();
	int GetPosition();
	int Seek(int lba);
	bool Read(BYTE* buff);
};

class CVobFile : public CDVDSession
{
	// all files
	typedef struct {
		CString fn;
		int size;
	} file_t;
	CAtlArray<file_t> m_files;
	int m_iFile;
	int m_pos, m_size, m_offset;

	// currently opened file
	CLBAFile m_file;

	// attribs
	bool m_fDVD, m_fHasDiscKey, m_fHasTitleKey;

	CAtlMap<DWORD, CString> m_pStream_Lang;

	UINT m_ChaptersCount;
	CAtlMap<BYTE, LONGLONG> m_pChapters;

	REFERENCE_TIME	m_rtDuration;
	AV_Rational		m_Aspect;

public:
	CVobFile();
	virtual ~CVobFile();

	bool IsDVD();
	bool HasDiscKey(BYTE* key);
	bool HasTitleKey(BYTE* key);

	bool Open(CString fn, CAtlList<CString>& files /* out */); // vts ifo
	bool Open(CAtlList<CString>& files, int offset = -1); // vts vobs, video vob offset in lba
	void Close();

	int GetLength();
	int GetPosition();
	int Seek(int pos);
	bool Read(BYTE* buff);

	BSTR GetTrackName(UINT aTrackIdx);

	UINT			GetChaptersCount() { return m_ChaptersCount; }
	LONGLONG		GetChapterOffset(UINT ChapterNumber);

	REFERENCE_TIME	GetDuration() { return m_rtDuration; }

	AV_Rational		GetAspect() { return m_Aspect; }

private:
	CFile		m_ifoFile;
	DWORD		ReadDword();
	SHORT		ReadShort();
	BYTE		ReadByte();
	void		ReadBuffer(BYTE* pBuff, DWORD nLen);
};
