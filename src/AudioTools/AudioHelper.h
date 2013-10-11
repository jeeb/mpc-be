/*
 * 
 * (C) 2013 see Authors.txt
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

#pragma warning(push)
#pragma warning(disable: 4005)
#include <stdint.h>
#pragma warning(pop)
#include "SampleFormat.h"

#ifdef _MSC_VER
#define bswap_16(x) _byteswap_ushort((unsigned short)(x))
#define bswap_32(x) _byteswap_ulong ((unsigned long)(x))
#define bswap_64(x) _byteswap_uint64((unsigned __int64)(x))
#else
#define bswap_16(x) ((uint16_t)(x) >> 8 | (uint16_t)(x) << 8)
#define bswap_32(x) ((uint32_t)(x) >> 24              | \
                    ((uint32_t)(x) & 0x00ff0000) >> 8 | \
                    ((uint32_t)(x) & 0x0000ff00) << 8 | \
                     (uint32_t)(x) << 24)
#define bswap_64(x) ((uint64_t)(x) >> 56                       | \
                    ((uint64_t)(x) & 0x00FF000000000000) >> 40 | \
                    ((uint64_t)(x) & 0x0000FF0000000000) >> 24 | \
                    ((uint64_t)(x) & 0x000000FF00000000) >>  8 | \
                    ((uint64_t)(x) & 0x00000000FF000000) <<  8 | \
                    ((uint64_t)(x) & 0x0000000000FF0000) << 24 | \
                    ((uint64_t)(x) & 0x000000000000FF00) << 40 | \
                     (uint64_t)(x) << 56)
#endif

HRESULT convert_to_int16(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, int16_t* pOut);
HRESULT convert_to_int24(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, BYTE* pOut);
HRESULT convert_to_int32(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, int32_t* pOut);
HRESULT convert_to_float(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, float* pOut);

HRESULT convert_to_planar_float(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, float* pOut);

inline void convert_int24_to_int32(size_t allsamples, BYTE* pIn, int32_t* pOut)
{
    for (size_t i = 0; i < allsamples; ++i) {
        pOut[i] = (uint32_t)pIn[3 * i]     << 8  |
                  (uint32_t)pIn[3 * i + 1] << 16 |
                  (uint32_t)pIn[3 * i + 2] << 24;
    }
}

//inline void convert_int24_to_int32(size_t allsamples, BYTE* pIn, int32_t* pOut)
//{
//    for (size_t i = 0; i < allsamples; ++i) {
//        BYTE* p = (BYTE*)&pOut[i];
//        p[0] = 0;
//        p[1] = *pIn++;
//        p[2] = *pIn++;
//        p[3] = *pIn++;
//    }
//}
//need perfomance tests

inline void convert_int32_to_int24(size_t allsamples, int32_t* pIn, BYTE* pOut)
{
    for (size_t i = 0; i < allsamples; ++i) {
        BYTE* p = (BYTE*)&pIn[i];
        *pOut++ = p[1];
        *pOut++ = p[2];
        *pOut++ = p[3];
    }
}

inline void convert_int24_to_float(size_t allsamples, BYTE* pIn, float* pOut)
{
    for (size_t i = 0; i < allsamples; ++i) {
        int32_t i32 = (uint32_t)pIn[3 * i]     << 8  |
                      (uint32_t)pIn[3 * i + 1] << 16 |
                      (uint32_t)pIn[3 * i + 2] << 24;
        pOut[i] = (float)((double)i32 / INT32_MAX);
    }
}

inline void convert_float_to_int24(size_t allsamples, float* pIn, BYTE* pOut)
{
    for (size_t i = 0; i < allsamples; ++i) {
        double d = (double)pIn[i] * INT32_MAX;
        if (d < INT32_MIN) { d = INT32_MIN; } else if (d > INT32_MAX) { d = INT32_MAX; }
        int32_t i32 = (int32_t)d;
        BYTE* p = (BYTE*)i32;
        *pOut++ = p[1];
        *pOut++ = p[2];
        *pOut++ = p[3];
    }
}
