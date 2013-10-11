/*
 * 
 * (C) 2013 see Authors.txt.
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
#include "AudioHelper.h"

#define INT8_PEAK       128
#define INT16_PEAK      32768
#define INT24_PEAK      8388608
#define INT32_PEAK      2147483648

#define F16MAX ( float(INT16_MAX) / INT16_PEAK)
#define D24MAX (double(INT24_MAX) / INT24_PEAK)
#define D32MAX (double(INT32_MAX) / INT32_PEAK)

#define round_f(x) ((x) > 0 ? (x) + 0.5f : (x) - 0.5f)
#define round_d(x) ((x) > 0 ? (x) + 0.5  : (x) - 0.5)

#define limit(a, x, b) if (x < a) { x = a; } else if (x > b) { x = b; }

HRESULT convert_to_int16(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, int16_t* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (int16_t)(*(int8_t*)pIn ^ 0x80) << 8;
				pIn += sizeof(uint8_t);
			}
			break;
		case SAMPLE_FMT_S16:
			memcpy(pOut, pIn, allsamples * sizeof(int16_t));
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = *(int16_t*)(pIn + 1); // read the high bits only
				pIn += 3;
			}
			break;
		case SAMPLE_FMT_S32:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = *(int16_t*)(pIn + 2); // read the high bits only
				pIn += sizeof(int32_t);
			}
			break;
		case SAMPLE_FMT_FLT:
			for (size_t i = 0; i < allsamples; ++i) {
				float f = *(float*)pIn;
				limit(-1, f, F16MAX);
				*pOut++ = (int16_t)round_f(f * INT16_PEAK);
				pIn += sizeof(float);
			}
			break;
		case SAMPLE_FMT_DBL:
			for (size_t i = 0; i < allsamples; ++i) {
				float f = (float)*(double*)pIn;
				limit(-1, f, F16MAX);
				*pOut++ = (int16_t)round_f(f * INT16_PEAK);
				pIn += sizeof(double);
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					int8_t b = ((int8_t*)pIn)[nSamples * ch + i];
					*pOut++ = (int16_t)(b ^ 0x80) << 8;
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = ((int16_t*)pIn)[nSamples * ch + i];
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = *(int16_t*)(pIn + (nSamples * ch + i) * sizeof(int32_t) + sizeof(int16_t)); // read the high bits only
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					float f = ((float*)pIn)[nSamples * ch + i];
					limit(-1, f, F16MAX);
					*pOut++ = (int16_t)round_f(f * INT16_PEAK);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					float f = (float)((double*)pIn)[nSamples * ch + i];
					limit(-1, f, F16MAX);
					*pOut++ = (int16_t)round_f(f * INT16_PEAK);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_int24(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, BYTE* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = 0;
				*pOut++ = 0;
				*pOut++ = (*pIn++) ^ 0x80;
			}
			break;
		case SAMPLE_FMT_S16:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = 0;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
			}
			break;
		case SAMPLE_FMT_S24:
			memcpy(pOut, pIn, allsamples * 3);
			break;
		case SAMPLE_FMT_S32:
			for (size_t i = 0; i < allsamples; ++i) {
				pIn++;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
			}
			break;
		case SAMPLE_FMT_FLT:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = (double)(*(float*)pIn);
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				*pOut++ = (BYTE)(u32 >> 8);
				*pOut++ = (BYTE)(u32 >> 16);
				*pOut++ = (BYTE)(u32 >> 24);
				pIn += sizeof(float);
			}
			break;
		case SAMPLE_FMT_DBL:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = *(double*)pIn;
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				*pOut++ = (BYTE)(u32 >> 8);
				*pOut++ = (BYTE)(u32 >> 16);
				*pOut++ = (BYTE)(u32 >> 24);
				pIn += sizeof(double);
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = 0;
					*pOut++ = 0;
					*pOut++ = pIn[nSamples * ch + i] ^ 0x80;
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					uint16_t u16 = ((uint16_t*)pIn)[nSamples * ch + i];
					*pOut++ = 0;
					*pOut++ = (BYTE)(u16);
					*pOut++ = (BYTE)(u16 >> 8);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					uint32_t u32 = ((uint32_t*)pIn)[nSamples * ch + i];
					*pOut++ = (BYTE)(u32 >> 8);
					*pOut++ = (BYTE)(u32 >> 16);
					*pOut++ = (BYTE)(u32 >> 24);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = (double)((float*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
					*pOut++ = (BYTE)(u32 >> 8);
					*pOut++ = (BYTE)(u32 >> 16);
					*pOut++ = (BYTE)(u32 >> 24);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = ((double*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
					*pOut++ = (BYTE)(u32 >> 8);
					*pOut++ = (BYTE)(u32 >> 16);
					*pOut++ = (BYTE)(u32 >> 24);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_int32(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, int32_t* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (int32_t)(*(int8_t*)pIn ^ 0x80) << 24;
				pIn += sizeof(uint8_t);
			}
			break;
		case SAMPLE_FMT_S16:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (int32_t)(*(int16_t*)pIn) << 16;
				pIn += sizeof(int16_t);
			}
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				pOut[i] = (uint32_t)pIn[3 * i]     << 8  |
				          (uint32_t)pIn[3 * i + 1] << 16 |
				          (uint32_t)pIn[3 * i + 2] << 24;
			}
			break;
		case SAMPLE_FMT_S32:
			memcpy(pOut, pIn, nSamples * nChannels * sizeof(int32_t));
			break;
		case SAMPLE_FMT_FLT:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = (double)(*(float*)pIn);
				limit(-1, d, D32MAX);
				*pOut++ = (int32_t)round_d(d * INT32_PEAK);
				pIn += sizeof(float);
			}
			break;
		case SAMPLE_FMT_DBL:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = *(double*)pIn;
				limit(-1, d, D32MAX);
				*pOut++ = (int32_t)round_d(d * INT32_PEAK);
				pIn += sizeof(double);
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					int8_t b = ((int8_t*)pIn)[nSamples * ch + i];
					*pOut++ = (int32_t)(b ^ 0x80) << 24;
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (int32_t)((int16_t*)pIn)[nSamples * ch + i] << 16;
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = ((int32_t*)pIn)[nSamples * ch + i];
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = (double)((float*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					*pOut++ = (int32_t)round_d(d * INT32_PEAK);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = ((double*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					*pOut++ = (int32_t)round_d(d * INT32_PEAK);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_float(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, float* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)(*(int8_t*)pIn ^ 0x80) / INT8_PEAK;
				pIn += sizeof(uint8_t);
			}
			break;
		case SAMPLE_FMT_S16:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)(*(int16_t*)pIn) / INT16_PEAK;
				pIn += sizeof(int16_t);
			}
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				int32_t i32 = (uint32_t)pIn[3 * i]     << 8  |
				              (uint32_t)pIn[3 * i + 1] << 16 |
				              (uint32_t)pIn[3 * i + 2] << 24;
				*pOut++ = (float)((double)i32 / INT32_PEAK);
				pIn += sizeof(int32_t);
			}
			break;
		case SAMPLE_FMT_S32:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)((double)(*(int32_t*)pIn) / INT32_PEAK);
				pIn += sizeof(int32_t);
			}
			break;
		case SAMPLE_FMT_FLT:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBL:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)*(double*)pIn;
				pIn += sizeof(double);
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (float)(((int8_t*)pIn)[nSamples * ch + i] ^ 0x80)/ INT8_PEAK;
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (float)((int16_t*)pIn)[nSamples * ch + i] / INT16_PEAK;
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (float)((double)((int32_t*)pIn)[nSamples * ch + i] / INT32_PEAK);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = ((float*)pIn)[nSamples * ch + i];
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (float)((double*)pIn)[nSamples * ch + i];
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_planar_float(SampleFormat sfmt, WORD nChannels, DWORD nSamples, BYTE* pIn, float* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (int ch = 0; ch < nChannels; ++ch) {
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = (float)(((int8_t*)pIn)[nChannels * i + ch] ^ 0x80)/ INT8_PEAK;
				}
			}
			break;
		case SAMPLE_FMT_S16:
			for (int ch = 0; ch < nChannels; ++ch) {
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = (float)((int16_t*)pIn)[nChannels * i + ch] / INT16_PEAK;
				}
			}
			break;
		case SAMPLE_FMT_S32:
			for (int ch = 0; ch < nChannels; ++ch) {
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = (float)((double)((int32_t*)pIn)[nChannels * i + ch] / INT32_PEAK);
				}
			}
			break;
		case SAMPLE_FMT_FLT:
			for (int ch = 0; ch < nChannels; ++ch) {
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = ((float*)pIn)[nChannels * i + ch];
				}
			}
			break;
		case SAMPLE_FMT_DBL:
			for (int ch = 0; ch < nChannels; ++ch) {
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = (float)((double*)pIn)[nChannels * i + ch];
				}
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)(*(int8_t*)pIn ^ 0x80) / INT8_PEAK;
				pIn += sizeof(uint8_t);
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)(*(int16_t*)pIn) / INT16_PEAK;
				pIn += sizeof(int16_t);
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)((double)(*(int32_t*)pIn) / INT32_PEAK);
				pIn += sizeof(int32_t);
			}
			break;
		case SAMPLE_FMT_FLTP:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = (float)*(double*)pIn;
				pIn += sizeof(double);
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}
