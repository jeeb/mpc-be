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
#ifndef _WAVPACK_PARSER_H_
#define _WAVPACK_PARSER_H_
// ----------------------------------------------------------------------------

typedef struct {
    stream_reader* io;
    int wvparser_eof;
    int is_correction;

    WavpackHeader first_wphdr;
    WavpackHeader wphdr; // last header read

    // last block meta data
    int block_channel_count;
    int block_bits_per_sample;
    uint32_t block_sample_rate;
    uint32_t block_samples_per_block;

    // global metadata
    int channel_count;
    int bits_per_sample;
    uint32_t sample_rate;
    uint32_t samples_per_block;

    int several_blocks;

    frame_buffer* fb;
    uint32_t suggested_buffer_size;

} WavPack_parser;

// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
// ----------------------------------------------------------------------------


WavPack_parser* wavpack_parser_new(stream_reader* io, int is_correction);

unsigned long wavpack_parser_read_frame(
    WavPack_parser* wpp,
    unsigned char* dst,
    unsigned long* FrameIndex,
    unsigned long* FrameLen);

void wavpack_parser_seek(WavPack_parser* wpp, uint64 seek_pos_100ns);

int wavpack_parser_eof(WavPack_parser* wpp);

void wavpack_parser_free(WavPack_parser* wpp);


// ----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif  
// ----------------------------------------------------------------------------

#endif // _WAVPACK_PARSER_H_

// ----------------------------------------------------------------------------
