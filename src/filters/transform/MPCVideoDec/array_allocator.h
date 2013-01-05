/*
 * $Id$
 *
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

#ifndef _ARRAY_ALLOCATOR_H_
#define _ARRAY_ALLOCATOR_H_

#include "../../../DSUtil/PODtypes.h"

template <class T,size_t size> class array_allocator
{
private:
	T* p;
public:
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef T* pointer;
	typedef const T* const_pointer;

	typedef T& reference;
	typedef const T& const_reference;

	pointer address(reference r) const {
		return &r;
	}
	const_pointer address(const_reference r) const {
		return &r;
	}

	array_allocator() throw() {}
	template <class U,size_t sz> array_allocator(const array_allocator<U,sz>& ) throw() {}
	~array_allocator() throw() {}

	pointer allocate(size_type n, const void* = 0) {
		p = ((T  *)::operator new(size * sizeof (T)));
		return p;
	}
	void deallocate(pointer p, size_type) {
		delete p;
	}

	//Use placement new to engage the constructor
	void construct(pointer p, const T& val) {
		new((void*)p) T(val);
	}
	void destroy(pointer p) {
		((T*)p)->~T();
	}

	size_type max_size() const throw() {
		return size;
	}
	template<class U> struct rebind {
		typedef array_allocator<U,size> other;
	};
};

template<class T,size_t size> struct array_vector : std::vector<T, array_allocator<T,size> > {
};

#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300)
template<class T,size_t a> struct allocator_traits<array_allocator<T,a> > {
	enum {is_static=true};
};
#endif

#endif
