/*
 * (C) 2009-2013 see Authors.txt
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

#include "avcodec.h"
#include "h264.h"

static const enum AVPixelFormat h264_hwaccel_pixfmt_list_420[] = {
#if CONFIG_H264_DXVA2_HWACCEL
    AV_PIX_FMT_DXVA2_VLD,
#endif
#if CONFIG_H264_VAAPI_HWACCEL
    AV_PIX_FMT_VAAPI_VLD,
#endif
#if CONFIG_H264_VDA_HWACCEL
    AV_PIX_FMT_VDA_VLD,
#endif
#if CONFIG_H264_VDPAU_HWACCEL
    AV_PIX_FMT_VDPAU,
#endif
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_NONE
};

enum AVPixelFormat get_pixel_format(H264Context *h, int force_callback)
{
    switch (h->sps.bit_depth_luma) {
    case 9:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                return AV_PIX_FMT_GBRP9;
            } else
                return AV_PIX_FMT_YUV444P9;
        } else if (CHROMA422(h))
            return AV_PIX_FMT_YUV422P9;
        else
            return AV_PIX_FMT_YUV420P9;
        break;
    case 10:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                return AV_PIX_FMT_GBRP10;
            } else
                return AV_PIX_FMT_YUV444P10;
        } else if (CHROMA422(h))
            return AV_PIX_FMT_YUV422P10;
        else
            return AV_PIX_FMT_YUV420P10;
        break;
    case 12:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                return AV_PIX_FMT_GBRP12;
            } else
                return AV_PIX_FMT_YUV444P12;
        } else if (CHROMA422(h))
            return AV_PIX_FMT_YUV422P12;
        else
            return AV_PIX_FMT_YUV420P12;
        break;
    case 14:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                return AV_PIX_FMT_GBRP14;
            } else
                return AV_PIX_FMT_YUV444P14;
        } else if (CHROMA422(h))
            return AV_PIX_FMT_YUV422P14;
        else
            return AV_PIX_FMT_YUV420P14;
        break;
    case 8:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                av_log(h->avctx, AV_LOG_DEBUG, "Detected GBR colorspace.\n");
                return AV_PIX_FMT_GBR24P;
            } else if (h->avctx->colorspace == AVCOL_SPC_YCGCO) {
                av_log(h->avctx, AV_LOG_WARNING, "Detected unsupported YCgCo colorspace.\n");
            }
            return AV_PIX_FMT_YUV444P;
        } else if (CHROMA422(h)) {
            return AV_PIX_FMT_YUV422P;
        } else {
            int i;
            const enum AVPixelFormat * fmt = h->avctx->codec->pix_fmts ?
                                        h->avctx->codec->pix_fmts :
                                        h264_hwaccel_pixfmt_list_420;

            for (i=0; fmt[i] != AV_PIX_FMT_NONE; i++)
                if (fmt[i] == h->avctx->pix_fmt && !force_callback)
                    return fmt[i];
            return ff_thread_get_format(h->avctx, fmt);
        }
        break;
    default:
        av_log(h->avctx, AV_LOG_ERROR,
               "Unsupported bit depth %d\n", h->sps.bit_depth_luma);
        return AVERROR_INVALIDDATA;
    }
}
