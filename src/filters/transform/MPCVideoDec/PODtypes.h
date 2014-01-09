/*
 * (C) 2003-2006 Gabest
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

#ifndef _PODTYPES_H_
#define _PODTYPES_H_

template<class T> struct isPOD {
	enum {is=false};
};
template<> struct isPOD<bool> {
	enum {is=true};
};
template<> struct isPOD<char> {
	enum {is=true};
};
template<> struct isPOD<signed char> {
	enum {is=true};
};
template<> struct isPOD<short int> {
	enum {is=true};
};
template<> struct isPOD<int> {
	enum {is=true};
};
template<> struct isPOD<long int> {
	enum {is=true};
};
template<> struct isPOD<__int64> {
	enum {is=true};
};
template<> struct isPOD<unsigned char> {
	enum {is=true};
};
template<> struct isPOD<unsigned short int> {
	enum {is=true};
};
template<> struct isPOD<unsigned int> {
	enum {is=true};
};
template<> struct isPOD<unsigned long int> {
	enum {is=true};
};
template<> struct isPOD<unsigned __int64> {
	enum {is=true};
};
template<> struct isPOD<float> {
	enum {is=true};
};
template<> struct isPOD<double> {
	enum {is=true};
};
template<> struct isPOD<long double> {
	enum {is=true};
};

#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300)
template<> struct isPOD<wchar_t> {
	enum {is=true};
};
template<class Tp> struct isPOD<Tp*> {
	enum {is=true};
};
#endif

template<class A> struct allocator_traits {
	enum {is_static=false};
};

#endif
