/*
 * $Id: CGdiPlusBitmap.h 466 2012-06-02 12:53:19Z exodus8 $
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
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

class CGdiPlusBitmap
{
public:
	Gdiplus::Bitmap* m_pBitmap;

public:
	CGdiPlusBitmap() { m_pBitmap = NULL; }
	CGdiPlusBitmap(LPCWSTR pFile) {
		m_pBitmap = NULL;
		Load(pFile);
	}
	virtual ~CGdiPlusBitmap() { Empty(); }

	void Empty() {
		delete m_pBitmap;
		m_pBitmap = NULL;
	}

	bool Load(LPCWSTR pFile) {
		Empty();
		m_pBitmap = Gdiplus::Bitmap::FromFile(pFile);
		return m_pBitmap->GetLastStatus() == Gdiplus::Ok;
	}

	operator Gdiplus::Bitmap*() const {
		return m_pBitmap;
	}
};

class CGdiPlusBitmapResource : public CGdiPlusBitmap
{
protected:
	HGLOBAL m_hBuffer;

public:
	CGdiPlusBitmapResource() {
		m_hBuffer = NULL;
	}
	CGdiPlusBitmapResource(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL) {
		m_hBuffer = NULL;
		Load(pName, pType, hInst);
	}
	CGdiPlusBitmapResource(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL) {
		m_hBuffer = NULL;
		Load(id, pType, hInst);
	}
	CGdiPlusBitmapResource(UINT id, UINT type, HMODULE hInst = NULL) {
		m_hBuffer = NULL;
		Load(id, type, hInst);
	}
	virtual ~CGdiPlusBitmapResource() { Empty(); }

	void Empty();

	bool Load(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL);
	bool Load(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL) {
		return Load(MAKEINTRESOURCE(id), pType, hInst);
	}
	bool Load(UINT id, UINT type, HMODULE hInst = NULL) {
		return Load(MAKEINTRESOURCE(id), MAKEINTRESOURCE(type), hInst);
	}
};

inline void CGdiPlusBitmapResource::Empty()
{
	CGdiPlusBitmap::Empty();

	if (m_hBuffer) {
		::GlobalUnlock(m_hBuffer);
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}
}

inline bool CGdiPlusBitmapResource::Load(LPCTSTR pName, LPCTSTR pType, HMODULE hInst)
{
	Empty();

	HRSRC hResource = ::FindResource(hInst, pName, pType);

	if (!hResource) {
		return false;
	}

	DWORD imageSize = ::SizeofResource(hInst, hResource);

	if (!imageSize) {
		return false;
	}

	const void* pResourceData = ::LockResource(::LoadResource(hInst, hResource));

	if (!pResourceData) {
		return false;
	}

	m_hBuffer  = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);

	if (m_hBuffer) {
		void* pBuffer = ::GlobalLock(m_hBuffer);

		if (pBuffer) {
			CopyMemory(pBuffer, pResourceData, imageSize);

			IStream* pStream = NULL;

			if (::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK) {
				m_pBitmap = Gdiplus::Bitmap::FromStream(pStream);
				pStream->Release();

				if (m_pBitmap) {
					if (m_pBitmap->GetLastStatus() == Gdiplus::Ok) {
						return true;
					}

					delete m_pBitmap;
					m_pBitmap = NULL;
				}
			}

			::GlobalUnlock(m_hBuffer);
		}

		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}

	return false;
}
