/*
 * 
 *      Copyright (C) 2010-2013 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Adaptation for MPC-BE (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 *
 */

#include "stdafx.h"
#include "H264RandomAccess.h"
#include "../../DSUtil/H264Nalu.h"
#include "../../DSUtil/GolombBuffer.h"

extern "C" {
	#include "ffmpeg/libavcodec/avcodec.h"
}

/**
 * SEI message types
 */
typedef enum {
	SEI_BUFFERING_PERIOD             =  0, ///< buffering period (H.264, D.1.1)
	SEI_TYPE_PIC_TIMING              =  1, ///< picture timing
	SEI_TYPE_USER_DATA_UNREGISTERED  =  5, ///< unregistered user data
	SEI_TYPE_RECOVERY_POINT          =  6  ///< recovery point (frame # to decoder sync)
} SEI_Type;

CH264RandomAccess::CH264RandomAccess()
{
	flush(0);
}

CH264RandomAccess::~CH264RandomAccess()
{
}

void CH264RandomAccess::flush(int threadCount)
{
	m_RecoveryMode			= 1;
	m_RecoveryFrameCount	= 0;
	m_ThreadDelay			= threadCount;
}

BOOL CH264RandomAccess::searchRecoveryPoint(const BYTE *buf, size_t buf_size)
{
	if (m_RecoveryMode == 1) {
		int recoveryPoint = parseForRecoveryPoint(buf, buf_size, &m_RecoveryFrameCount);
		switch (recoveryPoint) {
			case 3: // IDRS
				m_RecoveryMode = 0;
				return TRUE;
			case 2: // SEI recovery
				m_RecoveryMode = 2;
				return TRUE;
			case 1: // I Frame
				m_RecoveryMode = 2;
				m_RecoveryFrameCount = 0;
				return TRUE;
			default:
				return FALSE;
			}
		} else {
			return TRUE;
	}
}

int CH264RandomAccess::decode_sei_recovery_point(CGolombBuffer *pGB)
{
	while(pGB->RemainingSize() > 2) {
		int size, type;
    
		type = 0;
		do {
			type += pGB->BitRead(8, true);
		} while (pGB->BitRead(8) == 255);

		size = 0;
		do {
			size += pGB->BitRead(8, true);
		} while (pGB->BitRead(8) == 255);

		if (type == SEI_TYPE_RECOVERY_POINT) {
			int recovery_count = (int)pGB->UExpGolombRead();
			return recovery_count;
		}
	}
	return -1;
}

void CH264RandomAccess::judgeFrameUsability(AVFrame *pFrame, int *got_picture_ptr)
{
	if ((m_ThreadDelay > 1 && --m_ThreadDelay > 0) || m_RecoveryMode == 0 || pFrame->h264_max_frame_num == 0) {
		return;
	}

	if (m_RecoveryMode == 1 || m_RecoveryMode == 2) {
		m_RecoveryFrameCount = (m_RecoveryFrameCount + pFrame->h264_frame_num_decoded) % pFrame->h264_max_frame_num;
		m_RecoveryMode = 3;
	}

	if (m_RecoveryMode == 3 && m_RecoveryFrameCount <= pFrame->h264_frame_num_decoded) {
		m_RecoveryPOC = pFrame->h264_poc_decoded;
		m_RecoveryMode = 4;
	}

	if (m_RecoveryMode == 4 && pFrame->h264_poc_outputed >= m_RecoveryPOC) {
		m_RecoveryMode = 0;
	}

	if (m_RecoveryMode != 0) {
		*got_picture_ptr = 0;
	}
}

int CH264RandomAccess::parseForRecoveryPoint(const BYTE *buf, size_t buf_size, int *recoveryFrameCount)
{
	int found = 0;

	CH264Nalu nal;
	nal.SetBuffer((BYTE*)buf, buf_size, m_AVCNALSize);
	while(nal.ReadNext()) {
		BYTE *pData	= nal.GetDataBuffer() + 1;
		size_t len	= nal.GetDataLength() - 1;
		CGolombBuffer gb(pData, len);

		switch (nal.GetType()) {
			case NALU_TYPE_IDR:
				TRACE(_T("CH264RandomAccess::parseForRecoveryPoint() : Found IDR slice\n"));
				found = 3;
				goto end;
			case NALU_TYPE_SEI:
				if (pData[0] == SEI_TYPE_RECOVERY_POINT) {
					int ret = decode_sei_recovery_point(&gb);
					if (ret >= 0) {
						TRACE(_T("CH264RandomAccess::parseForRecoveryPoint() : Found SEI recovery point (count: %d)\n"), ret);
						*recoveryFrameCount = ret;
						if (found < 2) {
							found = 2;
						}
					}
				}
				break;
			case NALU_TYPE_AUD:
				{
					int primary_pic_type = gb.BitRead(3);
					if (!found && (primary_pic_type == 0 || primary_pic_type == 3)) { // I Frame
						TRACE(_T("CH264RandomAccess::parseForRecoveryPoint() : Found I frame\n"));
						found = 1;
					}
				}
				break;
			case NALU_TYPE_SLICE:
			case NALU_TYPE_DPA:
				if (!found) {
					gb.UExpGolombRead();
					int slice_type = (int)gb.UExpGolombRead();
					if (slice_type == 2 || slice_type == 4 || slice_type == 7 || slice_type == 9) { // I/SI slice
						TRACE(_T("CH264RandomAccess::parseForRecoveryPoint() : Detected I/SI slice\n"));
						found = 1;
					} else {
						found = 0;
						goto end;
					}
				}
				break;
		}
	}
	end:
	return found;
}
