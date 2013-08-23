/*
 * $Id$
 *
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include "ApeTag.h"
#include "DSUtil.h"

#define APE_TAG_FOOTER_BYTES			32
#define APE_TAG_VERSION					2000

#define APE_TAG_FLAG_IS_HEADER			(1 << 29)
#define APE_TAG_FLAG_IS_BINARY			(1 << 1)

//
// ApeTagItem class
//

CApeTagItem::CApeTagItem()
	: m_key(_T(""))
	, m_value(_T(""))
	, m_type(APE_TYPE_STRING)
	, m_buf(NULL)
	, m_len(0)
{
}

CApeTagItem::~CApeTagItem()
{
	if (m_buf) {
		delete [] m_buf;
		m_buf = NULL;
	}
}

bool CApeTagItem::Load(CGolombBuffer &gb){
	if ((gb.GetSize() - gb.GetPos()) < 8) {
		return false;
	}

	DWORD tag_size	= gb.ReadDwordLE();	/* field size */
	DWORD flags		= gb.ReadDwordLE();	/* field flags */

	CString key;
	BYTE b = gb.ReadByte();
	while (b) {
		key += b;
		b = gb.ReadByte();
	}

	if (flags & APE_TAG_FLAG_IS_BINARY) {
		m_type = APE_TYPE_BINARY;

		key = "";
		b = gb.ReadByte();
		tag_size--;
		while (b && tag_size) {
			key += b;
			b = gb.ReadByte();
			tag_size--;
		}

		if (tag_size) {
			m_key	= key;
			m_len	= tag_size;
			m_buf	= DNew BYTE[tag_size];
			gb.ReadBuffer(m_buf, tag_size);
		}
	} else {

		BYTE* value = DNew BYTE[tag_size + 1];
		memset(value, 0, tag_size + 1);
		gb.ReadBuffer(value, tag_size);
		m_value	= UTF8ToString((LPCSTR)value);
		m_key	= CString(key);
		delete [] value;
	}

	return true;
}

//
// ApeTag class
//

CAPETag::CAPETag()
	: m_TagSize(0)
	, m_TagFields(0)
{
}

CAPETag::~CAPETag()
{
	Clear();
}

void CAPETag::Clear()
{
	CApeTagItem* item;
	while (TagItems.GetCount() > 0) {
		item = TagItems.RemoveHead();
		if (item) {
			delete item;
		}
	}

	m_TagSize = m_TagFields = 0;
}

bool CAPETag::ReadFooter(BYTE *buf, size_t len)
{
	m_TagSize = m_TagFields = 0;

	if (len < APE_TAG_FOOTER_BYTES) {
		return false;
	}

	if (memcmp(buf, "APETAGEX", 8) || memcmp(buf+24, "\0\0\0\0\0\0\0\0", 8)) {
		return false;
	}

	CGolombBuffer gb((BYTE*)buf + 8, APE_TAG_FOOTER_BYTES - 8);
	DWORD ver = gb.ReadDwordLE();
	if (ver != APE_TAG_VERSION) {
		return false;
	}

	DWORD tag_size	= gb.ReadDwordLE();
	DWORD fields	= gb.ReadDwordLE();
	DWORD flags		= gb.ReadDwordLE();

	if ((fields > 65536) || (flags & APE_TAG_FLAG_IS_HEADER)) {
		return false;
	}

	m_TagSize	= tag_size;
	m_TagFields	= fields;

	return true;
}

bool CAPETag::ReadTags(BYTE *buf, size_t len)
{
	if (!m_TagSize || m_TagSize != len) {
		return false;
	}

	CGolombBuffer gb(buf, len);
	for (size_t i = 0; i < m_TagFields; i++) {
		CApeTagItem *item = DNew CApeTagItem();
		if (!item->Load(gb)) {
			delete item;
			Clear();
			return false;
		}

		TagItems.AddTail(item);
	}

	return true;
}

CApeTagItem* CAPETag::Find(CString key)
{
	CString key_lc = key;
	key_lc.MakeLower();


	POSITION pos = TagItems.GetHeadPosition();

	while (pos) {
		CApeTagItem* item = TagItems.GetAt(pos);
		CString TagKey = item->GetKey();
		TagKey.MakeLower();
		if (TagKey == key_lc) {
			return item;
		}

		TagItems.GetNext(pos);
	}

	return NULL;
}
