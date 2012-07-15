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

//-----------------------------------------------------------------------------
#ifndef WAVPACK_BUFFER_DECODER_H_
#define WAVPACK_BUFFER_DECODER_H_
//-----------------------------------------------------------------------------

typedef struct {
    stream_reader sr;
    
    char* buffer;
    uint32_t position;
    uint32_t length;
} frame_stream_reader;

typedef struct {
    frame_stream_reader* fsr;
    frame_stream_reader* fsrc;
    WavpackContext *wpc;
    char wavpack_error_msg[512];
} wavpack_buffer_decoder;

//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
    
// ----------------------------------------------------------------------------

wavpack_buffer_decoder* wavpack_buffer_decoder_new();

int wavpack_buffer_decoder_load_frame(wavpack_buffer_decoder* wbd,
                                      char* data, int length,
                                      char* correction_data, int cd_length);

uint32_t wavpack_buffer_decoder_unpack(wavpack_buffer_decoder* wbd,
                                       int32_t* buffer, uint32_t samples);

void wavpack_buffer_decoder_free(wavpack_buffer_decoder* wbd);

void wavpack_buffer_format_samples(wavpack_buffer_decoder* wbd,
                                   uchar *dst, long *src, uint32_t samples);


// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif  

//-----------------------------------------------------------------------------
#endif // WAVPACK_BUFFER_DECODER_H_
//-----------------------------------------------------------------------------