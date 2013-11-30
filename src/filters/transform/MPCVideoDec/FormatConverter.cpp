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
#include <moreuuids.h>

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libswscale/swscale.h>
	#include <ffmpeg/libavutil/pixdesc.h>
	#include <ffmpeg/libavutil/intreadwrite.h>
}
#pragma warning(pop)


static const SW_OUT_FMT s_sw_formats[] = {
	// YUV formats are grouped according to luma bit depth and sorted in descending order of quality.
	//  name     biCompression  subtype                                         av_pix_fmt    chroma_w chroma_h
	// YUV 8 bit
	{_T("AYUV"),  FCC('AYUV'), &MEDIASUBTYPE_AYUV,  32, 4, 0, {1},     {1},     AV_PIX_FMT_YUV444P,     0, 0, 24}, // PixFmt_AYUV
	{_T("YUY2"),  FCC('YUY2'), &MEDIASUBTYPE_YUY2,  16, 2, 0, {1},     {1},     AV_PIX_FMT_YUYV422,     1, 0, 16}, // PixFmt_YUY2
	{_T("NV12"),  FCC('NV12'), &MEDIASUBTYPE_NV12,  12, 1, 2, {1,2},   {1,1},   AV_PIX_FMT_NV12,        1, 1, 12}, // PixFmt_NV12
	{_T("YV12"),  FCC('YV12'), &MEDIASUBTYPE_YV12,  12, 1, 3, {1,2,2}, {1,2,2}, AV_PIX_FMT_YUV420P,     1, 1, 12}, // PixFmt_YV12
	// YUV 10 bit
	{_T("Y410"),  FCC('Y410'), &MEDIASUBTYPE_Y410,  32, 4, 0, {1},     {1},     AV_PIX_FMT_YUV444P10LE, 0, 0, 30}, // PixFmt_Y410
	{_T("P210"),  FCC('P210'), &MEDIASUBTYPE_P210,  32, 2, 2, {1,1},   {1,1},   AV_PIX_FMT_YUV422P16LE, 1, 0, 20}, // PixFmt_P210
	{_T("P010"),  FCC('P010'), &MEDIASUBTYPE_P010,  24, 2, 2, {1,2},   {1,1},   AV_PIX_FMT_YUV420P16LE, 1, 1, 15}, // PixFmt_P010
	// YUV 16 bit
	{_T("Y416"),  FCC('Y416'), &MEDIASUBTYPE_Y416,  64, 8, 0, {1},     {1},     AV_PIX_FMT_YUV444P16LE, 0, 0, 48}, // PixFmt_Y416
	{_T("P216"),  FCC('P216'), &MEDIASUBTYPE_P216,  32, 2, 2, {1,1},   {1,1},   AV_PIX_FMT_YUV422P16LE, 1, 0, 32}, // PixFmt_P216
	{_T("P016"),  FCC('P016'), &MEDIASUBTYPE_P016,  24, 2, 2, {1,2},   {1,1},   AV_PIX_FMT_YUV420P16LE, 1, 1, 24}, // PixFmt_P016
	// RGB
	{_T("RGB32"), BI_RGB,      &MEDIASUBTYPE_RGB32, 32, 4, 0, {1},     {1},     AV_PIX_FMT_BGRA,        0, 0, 24}, // PixFmt_RGB32
	//
	// PS: AV_PIX_FMT_YUV444P not equal to AYUV, but is used as an intermediate format.
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

MPCPixFmtType GetPixFmtType(AVPixelFormat av_pix_fmt)
{
	const AVPixFmtDescriptor* pfdesc = av_pix_fmt_desc_get(av_pix_fmt);
	int lumabits = pfdesc->comp->depth_minus1 + 1;

	if (pfdesc->flags & (AV_PIX_FMT_FLAG_RGB|AV_PIX_FMT_FLAG_PAL)) {
		return PFType_RGB;
	}

	if (lumabits < 8 || lumabits > 16 || pfdesc->nb_components != 3 + (pfdesc->flags & AV_PIX_FMT_FLAG_ALPHA ? 1 : 0)) {
		return PFType_unspecified;
	}

	if ((pfdesc->flags & AV_PIX_FMT_FLAG_PLANAR ^ (AV_PIX_FMT_FLAG_BE + AV_PIX_FMT_FLAG_ALPHA)) == AV_PIX_FMT_FLAG_PLANAR) {

		if (pfdesc->log2_chroma_w == 1 && pfdesc->log2_chroma_h == 1) {
			return (lumabits == 8) ? PFType_YUV420 : PFType_YUV420bX;
		}

		if (pfdesc->log2_chroma_w == 1 && pfdesc->log2_chroma_h == 0) {
			return (lumabits == 8) ? PFType_YUV422 : PFType_YUV422bX;
		}

		if (pfdesc->log2_chroma_w == 0 && pfdesc->log2_chroma_h == 0) {
			return (lumabits == 8) ? PFType_YUV444 : PFType_YUV444bX;
		}
	}

	return PFType_unspecified;
}

// CFormatConverter

CFormatConverter::CFormatConverter()
	: m_pSwsContext(NULL)
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

	m_FProps.avpixfmt	= AV_PIX_FMT_NONE;
	m_FProps.width		= 0;
	m_FProps.height		= 0;
	m_FProps.pftype		= PFType_unspecified;
	m_FProps.colorspace	= AVCOL_SPC_UNSPECIFIED;
	m_FProps.colorrange	= AVCOL_RANGE_UNSPECIFIED;
}

CFormatConverter::~CFormatConverter()
{
	Cleanup();
}

bool CFormatConverter::Init()
{
	Cleanup();
	if (m_FProps.avpixfmt == AV_PIX_FMT_NONE) {
		// check the input data, which can cause a crash.
		TRACE(_T("FormatConverter: incorrect source format\n"));
		return false;
	}

	const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

	m_pSwsContext = sws_getCachedContext(
						NULL,
						m_FProps.width,
						m_FProps.height,
						m_FProps.avpixfmt,
						m_FProps.width,
						m_FProps.height,
						swof.av_pix_fmt,
						m_swsFlags | SWS_PRINT_INFO,
						NULL,
						NULL,
						NULL);

	if (m_pSwsContext == NULL) {
		TRACE(_T("FormatConverter: sws_getCachedContext failed\n"));
		return false;
	}

	return true;
}

void  CFormatConverter::UpdateDetails()
{
	if (m_pSwsContext) {
		int *inv_tbl = NULL, *tbl = NULL;
		int srcRange, dstRange, brightness, contrast, saturation;
		int ret = sws_getColorspaceDetails(m_pSwsContext, &inv_tbl, &srcRange, &tbl, &dstRange, &brightness, &contrast, &saturation);
		if (ret >= 0) {
			if (m_out_pixfmt == PixFmt_RGB32 && !(av_pix_fmt_desc_get(m_FProps.avpixfmt)->flags & (AV_PIX_FMT_FLAG_RGB|AV_PIX_FMT_FLAG_PAL))) {
				dstRange = m_dstRGBRange;
			}
			ret = sws_setColorspaceDetails(m_pSwsContext, sws_getCoefficients(m_colorspace), srcRange, tbl, dstRange, brightness, contrast, saturation);
		}
	}
}

HRESULT CFormatConverter::ConvertToAYUV(const uint8_t* const src[4], const int srcStride[4], uint8_t* dst[], int width, int height, int dstStride[])
{
  const BYTE *y = NULL;
  const BYTE *u = NULL;
  const BYTE *v = NULL;
  int line, i = 0;
  int sourceStride = 0;
  BYTE *pTmpBuffer = NULL;

  if (m_FProps.avpixfmt != AV_PIX_FMT_YUV444P) {
    uint8_t *tmp[4] = {NULL};
    int     tmpStride[4] = {0};
    int scaleStride = FFALIGN(width, 32);

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 3);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride);
    tmp[2] = tmp[1] + (height * scaleStride);
    tmp[3] = NULL;
    tmpStride[0] = scaleStride;
    tmpStride[1] = scaleStride;
    tmpStride[2] = scaleStride;
    tmpStride[3] = 0;

    sws_scale(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = tmp[0];
    u = tmp[1];
    v = tmp[2];
    sourceStride = scaleStride;
  } else {
    y = src[0];
    u = src[1];
    v = src[2];
    sourceStride = srcStride[0];
  }

#define YUV444_PACK_AYUV(offset) *idst++ = v[i+offset] | (u[i+offset] << 8) | (y[i+offset] << 16) | (0xff << 24);

  BYTE *out = dst[0];
  for (line = 0; line < height; ++line) {
    int32_t *idst = (int32_t *)out;
    for (i = 0; i < (width-7); i+=8) {
      YUV444_PACK_AYUV(0)
      YUV444_PACK_AYUV(1)
      YUV444_PACK_AYUV(2)
      YUV444_PACK_AYUV(3)
      YUV444_PACK_AYUV(4)
      YUV444_PACK_AYUV(5)
      YUV444_PACK_AYUV(6)
      YUV444_PACK_AYUV(7)
    }
    for (; i < width; ++i) {
      YUV444_PACK_AYUV(0)
    }
    y += sourceStride;
    u += sourceStride;
    v += sourceStride;
    out += dstStride[0];
  }

  av_freep(&pTmpBuffer);

  return S_OK;
}

HRESULT CFormatConverter::ConvertToPX1X(const uint8_t* const src[4], const int srcStride[4], uint8_t* dst[], int width, int height, int dstStride[], int chromaVertical)
{
  const BYTE *y = NULL;
  const BYTE *u = NULL;
  const BYTE *v = NULL;
  int line, i = 0;
  int sourceStride = 0;

  int shift = 0;

  BYTE *pTmpBuffer = NULL;

  const AVPixFmtDescriptor* in_pfdesc = av_pix_fmt_desc_get(m_FProps.avpixfmt);
  int InBpp = in_pfdesc->comp->depth_minus1 + 1;

  if ((m_FProps.pftype != PFType_YUV422bX && chromaVertical == 1) || (m_FProps.pftype != PFType_YUV420bX && chromaVertical == 2)) {
    uint8_t *tmp[4] = {NULL};
    int     tmpStride[4] = {0};
    int scaleStride = FFALIGN(width, 32) * 2;

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 2);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride);
    tmp[2] = tmp[1] + ((height / chromaVertical) * (scaleStride / 2));
    tmp[3] = NULL;
    tmpStride[0] = scaleStride;
    tmpStride[1] = scaleStride / 2;
    tmpStride[2] = scaleStride / 2;
    tmpStride[3] = 0;

    sws_scale(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = tmp[0];
    u = tmp[1];
    v = tmp[2];
    sourceStride = scaleStride;
  } else {
    y = src[0];
    u = src[1];
    v = src[2];
    sourceStride = srcStride[0];

    shift = (16 - InBpp);
  }

  // copy Y
  BYTE *pLineOut = dst[0];
  const BYTE *pLineIn = y;
  for (line = 0; line < height; ++line) {
    if (shift == 0) {
      memcpy(pLineOut, pLineIn, width * 2);
    } else {
      const int16_t *yc = (int16_t *)pLineIn;
      int16_t *idst = (int16_t *)pLineOut;
      for (i = 0; i < width; ++i) {
        int32_t yv = AV_RL16(yc+i);
        if (shift) yv <<= shift;
        *idst++ = yv;
      }
    }
    pLineOut += dstStride[0];
    pLineIn += sourceStride;
  }

  sourceStride >>= 2;

  // Merge U/V
  BYTE *out = dst[1];
  const int16_t *uc = (int16_t *)u;
  const int16_t *vc = (int16_t *)v;
  for (line = 0; line < height/chromaVertical; ++line) {
    int32_t *idst = (int32_t *)out;
    for (i = 0; i < width/2; ++i) {
      int32_t uv = AV_RL16(uc+i);
      int32_t vv = AV_RL16(vc+i);
      if (shift) {
        uv <<= shift;
        vv <<= shift;
      }
      *idst++ = uv | (vv << 16);
    }
    uc += sourceStride;
    vc += sourceStride;
    out += dstStride[1];
  }

  av_freep(&pTmpBuffer);

  return S_OK;
}

#define YUV444_PACKED_LOOP_HEAD(width, height, y, u, v, out) \
  for (int line = 0; line < height; ++line) { \
    int32_t *idst = (int32_t *)out; \
    for(int i = 0; i < width; ++i) { \
      int32_t yv, uv, vv;

#define YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out) \
  YUV444_PACKED_LOOP_HEAD(width, height, y, u, v, out) \
    yv = AV_RL16(y+i); uv = AV_RL16(u+i); vv = AV_RL16(v+i);

#define YUV444_PACKED_LOOP_END(y, u, v, out, srcStride, dstStride) \
    } \
    y += srcStride; \
    u += srcStride; \
    v += srcStride; \
    out += dstStride; \
  }

HRESULT CFormatConverter::ConvertToY410(const uint8_t* const src[4], const int srcStride[4], uint8_t* dst[], int width, int height, int dstStride[])
{
  const int16_t *y = NULL;
  const int16_t *u = NULL;
  const int16_t *v = NULL;
  int sourceStride = 0;
  bool b9Bit = false;

  BYTE *pTmpBuffer = NULL;

  const AVPixFmtDescriptor* in_pfdesc = av_pix_fmt_desc_get(m_FProps.avpixfmt);
  int InBpp = in_pfdesc->comp->depth_minus1 + 1;

  if (m_FProps.pftype != PFType_YUV444bX || InBpp > 10) {
    uint8_t *tmp[4] = {NULL};
    int     tmpStride[4] = {0};
    int scaleStride = FFALIGN(width, 32);

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 6);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride * 2);
    tmp[2] = tmp[1] + (height * scaleStride * 2);
    tmp[3] = NULL;
    tmpStride[0] = scaleStride * 2;
    tmpStride[1] = scaleStride * 2;
    tmpStride[2] = scaleStride * 2;
    tmpStride[3] = 0;

    sws_scale(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = (int16_t *)tmp[0];
    u = (int16_t *)tmp[1];
    v = (int16_t *)tmp[2];
    sourceStride = scaleStride;
  } else {
    y = (int16_t *)src[0];
    u = (int16_t *)src[1];
    v = (int16_t *)src[2];
    sourceStride = srcStride[0] / 2;

    b9Bit = (InBpp == 9);
  }

#define YUV444_Y410_PACK \
  *idst++ = (uv & 0x3FF) | ((yv & 0x3FF) << 10) | ((vv & 0x3FF) << 20) | (3 << 30);

  BYTE *out = dst[0];
  YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out)
    if (b9Bit) {
      yv <<= 1;
      uv <<= 1;
      vv <<= 1;
    }
    YUV444_Y410_PACK
  YUV444_PACKED_LOOP_END(y, u, v, out, sourceStride, dstStride[0])

  av_freep(&pTmpBuffer);

  return S_OK;
}

HRESULT CFormatConverter::ConvertToY416(const uint8_t* const src[4], const int srcStride[4], uint8_t* dst[], int width, int height, int dstStride[])
{
  const int16_t *y = NULL;
  const int16_t *u = NULL;
  const int16_t *v = NULL;
  int sourceStride = 0;

  BYTE *pTmpBuffer = NULL;

  const AVPixFmtDescriptor* in_pfdesc = av_pix_fmt_desc_get(m_FProps.avpixfmt);
  int InBpp = in_pfdesc->comp->depth_minus1 + 1;

  if (m_FProps.pftype != PFType_YUV444bX || InBpp != 16) {
    uint8_t *tmp[4] = {NULL};
    int     tmpStride[4] = {0};
    int scaleStride = FFALIGN(width, 32);

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 6);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride * 2);
    tmp[2] = tmp[1] + (height * scaleStride * 2);
    tmp[3] = NULL;
    tmpStride[0] = scaleStride * 2;
    tmpStride[1] = scaleStride * 2;
    tmpStride[2] = scaleStride * 2;
    tmpStride[3] = 0;

    sws_scale(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = (int16_t *)tmp[0];
    u = (int16_t *)tmp[1];
    v = (int16_t *)tmp[2];
    sourceStride = scaleStride;
  } else {
    y = (int16_t *)src[0];
    u = (int16_t *)src[1];
    v = (int16_t *)src[2];
    sourceStride = srcStride[0] / 2;
  }

#define YUV444_Y416_PACK \
  *idst++ = 0xFFFF | (vv << 16); \
  *idst++ = yv | (uv << 16);

  BYTE *out = dst[0];
  YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out)
    YUV444_Y416_PACK
  YUV444_PACKED_LOOP_END(y, u, v, out, sourceStride, dstStride[0])

  av_freep(&pTmpBuffer);

  return S_OK;
}

void CFormatConverter::UpdateOutput(MPCPixelFormat out_pixfmt, int dstStride, int planeHeight)
{
	if (out_pixfmt != m_out_pixfmt) {
		m_out_pixfmt = out_pixfmt;
		Cleanup();
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
		m_autocolorspace = false;
		m_colorspace = SWS_CS_ITU601;
		break;
	case 1  : // HD(BT.709)
		m_autocolorspace = false;
		m_colorspace = SWS_CS_ITU709;
		break;
	case 2  : // Auto
	default :
		m_autocolorspace = true;
		m_colorspace = m_FProps.width > 768 ? SWS_CS_ITU709 : SWS_CS_ITU601;
		break;
	}

	m_dstRGBRange = rgblevels == 1 ? 0 : 1;

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
		Cleanup();
	} else {
		UpdateDetails();
	}
}

int CFormatConverter::Converting(BYTE* dst, AVFrame* pFrame)
{
	if (!m_pSwsContext || pFrame->format != m_FProps.avpixfmt || pFrame->width != m_FProps.width || pFrame->height != m_FProps.height) {
		// update the basic properties
		m_FProps.avpixfmt	= (AVPixelFormat)pFrame->format;
		m_FProps.width		= pFrame->width;
		m_FProps.height		= pFrame->height;
		if (!Init()) {
			TRACE(_T("FormatConverter: Init() failed\n"));
			return 0;
		}
		// update the additional properties (updated only when changing basic properties)
		m_FProps.pftype		= GetPixFmtType((AVPixelFormat)pFrame->format);
		m_FProps.colorspace	= pFrame->colorspace;
		m_FProps.colorrange	= pFrame->color_range;
		UpdateDetails();
	}

	const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

	// From LAVVideo...
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

	switch (m_out_pixfmt) {
	case PixFmt_AYUV:
		ConvertToAYUV(pFrame->data, pFrame->linesize, dstArray, m_FProps.width, m_FProps.height, dstStrideArray);
		break;
	case PixFmt_P010:
	case PixFmt_P016:
		ConvertToPX1X(pFrame->data, pFrame->linesize, dstArray, m_FProps.width, m_FProps.height, dstStrideArray, 2);
		break;
	case PixFmt_Y410:
		ConvertToY410(pFrame->data, pFrame->linesize, dstArray, m_FProps.width, m_FProps.height, dstStrideArray);
		break;
	case PixFmt_P210:
	case PixFmt_P216:
		ConvertToPX1X(pFrame->data, pFrame->linesize, dstArray, m_FProps.width, m_FProps.height, dstStrideArray, 1);
		break;
	case PixFmt_Y416:
		ConvertToY416(pFrame->data, pFrame->linesize, dstArray, m_FProps.width, m_FProps.height, dstStrideArray);
		break;
	default:
		int ret = sws_scale(m_pSwsContext, pFrame->data, pFrame->linesize, 0, m_FProps.height, dstArray, dstStrideArray);
	}

	if (out != dst) {
		int line = 0;

		// Copy first plane
		const int widthBytes = m_FProps.width * swof.codedbytes;
		const int srcStrideBytes = outStride * swof.codedbytes;
		const int dstStrideBytes = m_dstStride * swof.codedbytes;
		for (line = 0; line < m_FProps.height; ++line) {
			memcpy(dst, out, widthBytes);
			out += srcStrideBytes;
			dst += dstStrideBytes;
		}
		dst += (m_planeHeight - m_FProps.height) * dstStrideBytes;
		
		for (int plane = 1; plane < swof.planes; ++plane) {
			const int planeWidth        = widthBytes      / swof.planeWidth[plane];
			const int activePlaneHeight = m_FProps.height / swof.planeHeight[plane];
			const int totalPlaneHeight  = m_planeHeight   / swof.planeHeight[plane];
			const int srcPlaneStride    = srcStrideBytes  / swof.planeWidth[plane];
			const int dstPlaneStride    = dstStrideBytes  / swof.planeWidth[plane];
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
