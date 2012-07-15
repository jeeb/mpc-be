/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
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

// ----------------------------------------------------------------------------
#ifndef WAVPACK_FRAME_H_
#define WAVPACK_FRAME_H_
// ---------------------------------------------------------------------------

typedef struct {
  uint32_t block_samples; // number of samples per block
  uint32_t array_flags[10]; // flags of main frame (up to 10 blocks)
} common_frame_data;

typedef struct {
    char* data;
    int pos; // position on the data buffer
    int len; // total number of used bytes
    int total_len; // total len including free space
    uint32_t nb_block;
} frame_buffer;

//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
    
// ----------------------------------------------------------------------------

frame_buffer* frame_buffer_new();
void frame_buffer_free(frame_buffer* fb);

int frame_reserve_space(frame_buffer* dst, int len);

int frame_append_data(frame_buffer* dst, char* src, int len);
int frame_append_data2(frame_buffer* dst, stream_reader *io, int len);

void frame_reset(frame_buffer* dst);

int reconstruct_wavpack_frame(
    frame_buffer *frame,
    common_frame_data *common_data,
    char *pSrc,
    uint32_t SrcLength,
    int is_main_frame,
    int several_blocks,
    int version);

int strip_wavpack_block(frame_buffer *frame,
                        WavpackHeader *wphfr,
                        stream_reader *io,
                        uint32_t block_data_size,
                        int is_main_frame,
                        int several_blocks);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif  

// ---------------------------------------------------------------------------
#endif //WAVPACK_FRAME_H_
// ---------------------------------------------------------------------------