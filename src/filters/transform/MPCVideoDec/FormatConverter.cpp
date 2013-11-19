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

#include "stdafx.h"
#include "FormatConverter.h"
#include "MPCVideoDec.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libswscale/swscale.h>
	#include <ffmpeg/libavutil/pixdesc.h>
}
#pragma warning(pop)


static const SW_OUT_FMT s_sw_formats[] = {
	// YUV formats are grouped according to luma bit depth and sorted in descending order of quality.
	//  name     biCompression  subtype                                         av_pix_fmt    chroma_w chroma_h
	// YUV 8 bit
	{_T("YUY2"),  FCC('YUY2'), &MEDIASUBTYPE_YUY2,  16, 2, 0, {1},     {1},     AV_PIX_FMT_YUYV422, 1, 0 }, // PixFmt_YUY2
	{_T("NV12"),  FCC('NV12'), &MEDIASUBTYPE_NV12,  12, 1, 2, {1,2},   {1,1},   AV_PIX_FMT_NV12,    1, 1 }, // PixFmt_NV12
	{_T("YV12"),  FCC('YV12'), &MEDIASUBTYPE_YV12,  12, 1, 3, {1,2,2}, {1,2,2}, AV_PIX_FMT_YUV420P, 1, 1 }, // PixFmt_YV12
	// YUV 10 bit
	// ...
	// RGB
	{_T("RGB32"), BI_RGB,      &MEDIASUBTYPE_RGB32, 32, 4, 0, {1},     {1},     AV_PIX_FMT_BGRA,    0, 0 }, // PixFmt_RGB32
};

const SW_OUT_FMT* GetSWOF(int pixfmt)
{
	if (pixfmt < 0 && pixfmt >= PixFmt_count) {
		return NULL;
	}
	return &s_sw_formats[pixfmt];
}

MPCPixelFormat GetPixFormat(GUID& subtype)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (*s_sw_formats[i].subtype == subtype) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

MPCPixelFormat GetPixFormat(AVPixelFormat av_pix_fmt)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (s_sw_formats[i].av_pix_fmt == av_pix_fmt) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

MPCPixelFormat GetPixFormat(DWORD biCompression)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (s_sw_formats[i].biCompression == biCompression) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

// CFormatConverter

CFormatConverter::CFormatConverter()
	: m_pSwsContext(NULL)
	, m_ActualContext(2)
	, m_width(0)
	, m_height(0)
	, m_in_avpixfmt(AV_PIX_FMT_NONE)
	, m_out_pixfmt(PixFmt_None)
	, m_swsFlags(SWS_BILINEAR | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INP)
	, m_colorspace(SWS_CS_DEFAULT)
	, m_dstRGBRange(0)
	, m_dstStride(0)
	, m_planeHeight(0)
	, m_nAlignedBufferSize(0)
	, m_pAlignedBuffer(NULL)
{
	ASSERT(PixFmt_count == _countof(s_sw_formats));
}

CFormatConverter::~CFormatConverter()
{
	Cleanup();
}

void CFormatConverter::Init(CMPCVideoDecFilter* pFilter)
{
	m_pFilter = pFilter;
}

static inline enum AVPixelFormat SelectFmt(enum AVPixelFormat pix_fmt)
{
	switch (pix_fmt) {
		case AV_PIX_FMT_YUVJ420P:
			return AV_PIX_FMT_YUV420P;
		case AV_PIX_FMT_YUVJ422P:
			return AV_PIX_FMT_YUV422P;
		case AV_PIX_FMT_YUVJ440P:
			return AV_PIX_FMT_YUV440P;
		case AV_PIX_FMT_YUVJ444P:
			return AV_PIX_FMT_YUV444P;
		default:
			return pix_fmt;
	}
}

bool CFormatConverter::Init()
{
	if (!m_pSwsContext || m_ActualContext == 2) {
		Cleanup();

		const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

		m_pSwsContext = sws_getCachedContext(
							NULL,
							m_width,
							m_height,
							SelectFmt(m_in_avpixfmt),
							m_width,
							m_height,
							swof.av_pix_fmt,
							m_swsFlags | SWS_PRINT_INFO,
							NULL,
							NULL,
							NULL);

		if (m_pSwsContext == NULL) {
			TRACE(_T("FormatConverter: sws_getCachedContext failed\n"));
			return false;
		}

		m_ActualContext = 1;
	}

	if (m_ActualContext == 1) {
		int *inv_tbl = NULL, *tbl = NULL;
		int srcRange, dstRange, brightness, contrast, saturation;
		int ret = sws_getColorspaceDetails(m_pSwsContext, &inv_tbl, &srcRange, &tbl, &dstRange, &brightness, &contrast, &saturation);
		if (ret >= 0) {
			if (!(av_pix_fmt_desc_get(m_in_avpixfmt)->flags & AV_PIX_FMT_FLAG_RGB) && m_out_pixfmt == PixFmt_RGB32) {
				dstRange = m_dstRGBRange;
			}
			ret = sws_setColorspaceDetails(m_pSwsContext, sws_getCoefficients(m_colorspace), srcRange, tbl, dstRange, brightness, contrast, saturation);
		}

		if (m_pFilter && m_pFilter->GetDialogHWND()) {
			HWND hwnd = m_pFilter->GetDialogHWND();

			if (m_pFilter->GetColorSpaceConversion() == 1) {
				EnableWindow(GetDlgItem(hwnd, IDC_PP_SWPRESET),       TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PP_SWSTANDARD),     TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_PP_SWOUTPUTLEVELS), TRUE);
			} else {
				EnableWindow(GetDlgItem(hwnd, IDC_PP_SWPRESET),       FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PP_SWSTANDARD),     FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_PP_SWOUTPUTLEVELS), FALSE);
			}
		}
	}

	m_ActualContext = 0;
	return true;
}

void CFormatConverter::UpdateOutput(MPCPixelFormat out_pixfmt, int dstStride, int planeHeight)
{
	if (out_pixfmt != m_out_pixfmt) {
		m_out_pixfmt	= out_pixfmt;
		m_ActualContext = 2;
	}

	m_dstStride   = dstStride;
	m_planeHeight = planeHeight;
}

void CFormatConverter::UpdateOutput2(DWORD biCompression, LONG biWidth, LONG biHeight)
{
	UpdateOutput(GetPixFormat(biCompression), biWidth, abs(biHeight));
}

void CFormatConverter::SetOptions(int preset, int standard, int rgblevels)
{
	switch (standard) {
	case 0  : // SD(BT.601)
		m_colorspace = SWS_CS_ITU601;
		break;
	case 1  : // HD(BT.709)
		m_colorspace = SWS_CS_ITU709;
		break;
	case 2  : // Auto
		m_colorspace = m_width > 768 ? SWS_CS_ITU709 : SWS_CS_ITU601;
		break;
	default :
		m_colorspace = SWS_CS_DEFAULT;
	}

	m_dstRGBRange = rgblevels == 1 ? 0 : 1;

	if (m_ActualContext == 0) {
		m_ActualContext = 1;
	}

	int swsFlags = 0;
	switch (preset) {
	case 0  : // "Fastest"
		swsFlags = SWS_FAST_BILINEAR | SWS_ACCURATE_RND;
		// SWS_FAST_BILINEAR or SWS_POINT disable dither and enable low-quality yv12_to_yuy2 conversion.
		// any interpolation type has no effect.
		break;
	case 1  : // "Fast"
		swsFlags = SWS_BILINEAR | SWS_ACCURATE_RND;
		break;
	case 2  :// "Normal"
	default :
		swsFlags = SWS_BILINEAR | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INP;
		break;
	case 3  : // "Full"
		swsFlags = SWS_BILINEAR | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INP | SWS_FULL_CHR_H_INT;
		break;
	}
	if (swsFlags != m_swsFlags) {
		m_swsFlags = swsFlags;
		m_ActualContext = 2;
	}
}

int CFormatConverter::Converting(BYTE* dst, AVFrame* pFrame)
{
	if (pFrame->format != m_in_avpixfmt || pFrame->width != m_width || pFrame->height != m_height) {
		m_in_avpixfmt	= (AVPixelFormat)pFrame->format;
		m_width			= pFrame->width;
		m_height		= pFrame->height;
		m_ActualContext	= 2;
	}

	if ((!m_pSwsContext || m_ActualContext) && !Init()) {
		TRACE(_T("FormatConverter: Init() failed\n"));
		return 0;
	}

	const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

	// From LAVVideo ...
	uint8_t *out = dst;
	int outStride = m_dstStride;
	// Check if we have proper pixel alignment and the dst memory is actually aligned
	if (FFALIGN(m_dstStride, 16) != m_dstStride || ((uintptr_t)dst % 16u)) {
		outStride = FFALIGN(outStride, 16);
		size_t requiredSize = (outStride * m_planeHeight * swof.bpp) << 3;
		if (requiredSize > m_nAlignedBufferSize) {
			av_freep(&m_pAlignedBuffer);
			m_nAlignedBufferSize = requiredSize;
			m_pAlignedBuffer = (uint8_t*)av_malloc(m_nAlignedBufferSize + FF_INPUT_BUFFER_PADDING_SIZE);
		}
		out = m_pAlignedBuffer;
	}

	uint8_t*	dstArray[4]			= {NULL};
	int			dstStrideArray[4]	= {0};
	int			byteStride			= outStride * swof.codedbytes;

	dstArray[0] = out;
	dstStrideArray[0] = byteStride;
	for (int i = 1; i < swof.planes; ++i) {
		dstArray[i] = dstArray[i-1] + dstStrideArray[i-1] * (m_planeHeight / swof.planeWidth[i-1]);
		dstStrideArray[i] = byteStride / swof.planeWidth[i];
	}

	if (m_out_pixfmt == PixFmt_YV12) {
		std::swap(dstArray[1], dstArray[2]);
	}

	int ret = sws_scale(m_pSwsContext, pFrame->data, pFrame->linesize, 0, m_height, dstArray, dstStrideArray);

	if (out != dst) {
		int line = 0;

		// Copy first plane
		const int widthBytes = m_width * swof.codedbytes;
		const int srcStrideBytes = outStride * swof.codedbytes;
		const int dstStrideBytes = m_dstStride * swof.codedbytes;
		for (line = 0; line < m_height; ++line) {
			memcpy(dst, out, widthBytes);
			out += srcStrideBytes;
			dst += dstStrideBytes;
		}
		dst += (m_planeHeight - m_height) * dstStrideBytes;
		
		for (int plane = 1; plane < swof.planes; ++plane) {
			const int planeWidth        = widthBytes     / swof.planeWidth[plane];
			const int activePlaneHeight = m_height       / swof.planeHeight[plane];
			const int totalPlaneHeight  = m_planeHeight  / swof.planeHeight[plane];
			const int srcPlaneStride    = srcStrideBytes / swof.planeWidth[plane];
			const int dstPlaneStride    = dstStrideBytes / swof.planeWidth[plane];
			for (line = 0; line < activePlaneHeight; ++line) {
				memcpy(dst, out, planeWidth);
				out += srcPlaneStride;
				dst += dstPlaneStride;
			}
			dst+= (totalPlaneHeight - activePlaneHeight) * dstPlaneStride;
		}
	}

	return 0;
}

void CFormatConverter::Cleanup()
{
	if (m_pSwsContext) {
		sws_freeContext(m_pSwsContext);
		m_pSwsContext	= NULL;
	}
	if (m_pAlignedBuffer) {
		av_freep(&m_pAlignedBuffer);
	}
}
