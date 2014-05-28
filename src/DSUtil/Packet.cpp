/*
 * (C) 2006-2014 see Authors.txt
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
#include "Packet.h"

//
// CPacketQueue
//

CPacketQueue::CPacketQueue() : m_size(0)
{
}

void CPacketQueue::Add(CAutoPtr<Packet> p)
{
	CAutoLock cAutoLock(this);

	if (p) {
		m_size += p->GetDataSize();

		if (p->bAppendable && !p->bDiscontinuity && !p->pmt
				&& p->rtStart == INVALID_TIME
				&& !IsEmpty() && GetTail()->rtStart != INVALID_TIME) {
			Packet* tail = GetTail();
			size_t oldsize = tail->GetCount();
			size_t newsize = tail->GetCount() + p->GetCount();
			tail->SetCount(newsize, max(1024, newsize)); // doubles the reserved buffer size
			memcpy(tail->GetData() + oldsize, p->GetData(), p->GetCount());
			//GetTail()->Append(*p); // too slow
			return;
		}
	}

	AddTail(p);
}

CAutoPtr<Packet> CPacketQueue::Remove()
{
	CAutoLock cAutoLock(this);
	ASSERT(__super::GetCount() > 0);
	CAutoPtr<Packet> p = RemoveHead();
	if (p) {
		m_size -= p->GetDataSize();
	}
	return p;
}

CAutoPtr<Packet> CPacketQueue::GetFirst()
{
	CAutoLock cAutoLock(this);
	ASSERT(__super::GetCount() > 0);
	return __super::GetHead();
}

CAutoPtr<Packet> CPacketQueue::GetLast()
{
	CAutoLock cAutoLock(this);
	ASSERT(__super::GetCount() > 0);
	return __super::GetTail();
}

void CPacketQueue::RemoveAll()
{
	CAutoLock cAutoLock(this);
	m_size = 0;
	__super::RemoveAll();
}

size_t CPacketQueue::GetCount()
{
	CAutoLock cAutoLock(this);
	return __super::GetCount();
}

size_t CPacketQueue::GetSize()
{
	CAutoLock cAutoLock(this);
	return m_size;
}
