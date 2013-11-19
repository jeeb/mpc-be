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

#include "IMPCVideoDec.h"

#pragma warning(push)
#pragma warning(disable: 4005)
#include <stdint.h>
#pragma warning(pop)

typedef struct {
	const LPCTSTR				name;
	const DWORD					biCompression;
	const GUID*					subtype;
	const int					bpp;
	const int					codedbytes;
	const int					planes;
	const int					planeHeight[4];
	const int					planeWidth[4];
	const enum AVPixelFormat	av_pix_fmt;
	const uint8_t				chroma_w;
	const uint8_t				chroma_h;
} SW_OUT_FMT;

const SW_OUT_FMT* GetSWOF(int pixfmt);

struct AVFrame;
struct SwsContext;

class CMPCVideoDecFilter;

class CFormatConverter
{
protected:
	SwsContext*			m_pSwsContext;
	int					m_ActualContext; // 0 -actual, 1 - need update options, 2 - nead reinit

	int					m_width;
	int					m_height;

	enum AVPixelFormat	m_in_avpixfmt;
	MPCPixelFormat		m_out_pixfmt;

	int					m_swsFlags;
	int					m_colorspace;
	int					m_dstRGBRange;

	int					m_dstStride;
	int					m_planeHeight;

	size_t				m_nAlignedBufferSize;
	uint8_t*			m_pAlignedBuffer;

	CMPCVideoDecFilter*	m_pFilter;

	bool Init();

public:
	CFormatConverter();
	~CFormatConverter();

	void Init(CMPCVideoDecFilter* pFilter);

	void UpdateOutput(MPCPixelFormat out_pixfmt, int dstStride, int planeHeight);
	void UpdateOutput2(DWORD biCompression, LONG biWidth, LONG biHeight);
	void SetOptions(int preset, int standard, int rgblevels);

	MPCPixelFormat GetOutPixFormat() { return m_out_pixfmt; }

	int  Converting(BYTE* dst, AVFrame* pFrame);

	void Cleanup();
};
