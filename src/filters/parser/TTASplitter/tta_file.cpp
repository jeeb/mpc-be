/*
 *
 * Adaptation for MPC-BE (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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
#include "tta_file.h"
#include "TTASplitter.h"

#ifdef _BIG_ENDIAN
#define	ENDSWAP_INT16(x)	(((((x)>>8)&0xFF)|(((x)&0xFF)<<8)))
#define	ENDSWAP_INT32(x)	(((((x)>>24)&0xFF)|(((x)>>8)&0xFF00)|(((x)&0xFF00)<<8)|(((x)&0xFF)<<24)))
#else
#define	ENDSWAP_INT16(x)	(x)
#define	ENDSWAP_INT32(x)	(x)
#endif

#define SWAP16(x) (\
	(((x)&(1<< 0))?(1<<15):0) | \
	(((x)&(1<< 1))?(1<<14):0) | \
	(((x)&(1<< 2))?(1<<13):0) | \
	(((x)&(1<< 3))?(1<<12):0) | \
	(((x)&(1<< 4))?(1<<11):0) | \
	(((x)&(1<< 5))?(1<<10):0) | \
	(((x)&(1<< 6))?(1<< 9):0) | \
	(((x)&(1<< 7))?(1<< 8):0) | \
	(((x)&(1<< 8))?(1<< 7):0) | \
	(((x)&(1<< 9))?(1<< 6):0) | \
	(((x)&(1<<10))?(1<< 5):0) | \
	(((x)&(1<<11))?(1<< 4):0) | \
	(((x)&(1<<12))?(1<< 3):0) | \
	(((x)&(1<<13))?(1<< 2):0) | \
	(((x)&(1<<14))?(1<< 1):0) | \
(((x)&(1<<15))?(1<< 0):0))

#define PREDICTOR1(x, k)	((long)((((uint64)x << k) - x) >> k))

#define ENC(x)  (((x)>0)?((x)<<1)-1:(-(x)<<1))
#define DEC(x)  (((x)&1)?(++(x)>>1):(-(x)>>1))

const unsigned long bit_mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

const unsigned long bit_shift[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000
};

const unsigned long *shift_16 = bit_shift + 4;

void bit_buffer_free(TTA_bit_buffer* bbuf)
{
	if(bbuf)
	{
		if(bbuf->bit_buffer)
		{
			bbuf->bitpos = NULL;
			bbuf->BIT_BUFFER_END = NULL;
			GlobalFree(bbuf->bit_buffer);
			bbuf->bit_buffer = NULL;
		}
		GlobalFree(bbuf);
	}
}

TTA_bit_buffer* bit_buffer_new(unsigned long size)
{
	TTA_bit_buffer* bbuf = (TTA_bit_buffer*)tta_alloc(sizeof(TTA_bit_buffer));
	if(!bbuf)
		return NULL;
	bbuf->size = size + 8;
	bbuf->bit_buffer = (unsigned char*)tta_alloc(size + 8);
	if(!bbuf->bit_buffer)
	{
		bit_buffer_free(bbuf);
		return NULL;
	}
	bbuf->BIT_BUFFER_END = bbuf->bit_buffer + bbuf->size;
	return bbuf;
}

void bit_buffer_init_write(TTA_bit_buffer* bbuf)
{
	bbuf->crc32 = 0xFFFFFFFFUL;
	bbuf->bit_count = bbuf->bit_cache = 0;
	bbuf->bitpos = bbuf->bit_buffer;
}

void bit_buffer_init_read(TTA_bit_buffer* bbuf)
{
	bbuf->crc32 = 0xFFFFFFFFUL;
	bbuf->bit_count = bbuf->bit_cache = 0;
	bbuf->bitpos = bbuf->BIT_BUFFER_END;
}

__inline void bit_buffer_put_binary(TTA_bit_buffer* bbuf, unsigned long value, unsigned long bits)
{
	while (bbuf->bit_count >= 8) {
		*bbuf->bitpos = (unsigned char) (bbuf->bit_cache & 0xFF);
		UPDATE_CRC32(*bbuf->bitpos, bbuf->crc32);
		bbuf->bit_cache >>= 8;
		bbuf->bit_count -= 8;
		bbuf->bitpos++;
	}

	bbuf->bit_cache |= (value & bit_mask[bits]) << bbuf->bit_count;
	bbuf->bit_count += bits;
}

__inline void bit_buffer_get_binary(TTA_bit_buffer* bbuf, unsigned long *value, unsigned long bits)
{
	while (bbuf->bit_count < bits) {
		UPDATE_CRC32(*bbuf->bitpos, bbuf->crc32);
		bbuf->bit_cache |= *bbuf->bitpos << bbuf->bit_count;
		bbuf->bit_count += 8;
		bbuf->bitpos++;
	}

	*value = bbuf->bit_cache & bit_mask[bits];
	bbuf->bit_cache >>= bits;
	bbuf->bit_count -= bits;
	bbuf->bit_cache &= bit_mask[bbuf->bit_count];
}

__inline void bit_buffer_put_unary(TTA_bit_buffer* bbuf, unsigned long value)
{
	do {
		while (bbuf->bit_count >= 8) {
			*bbuf->bitpos = (unsigned char) (bbuf->bit_cache & 0xFF);
			UPDATE_CRC32(*bbuf->bitpos, bbuf->crc32);
			bbuf->bit_cache >>= 8;
			bbuf->bit_count -= 8;
			bbuf->bitpos++;
		}

		if (value > 23) {
			bbuf->bit_cache |= bit_mask[23] << bbuf->bit_count;
			bbuf->bit_count += 23;
			value -= 23;
		} else {
			bbuf->bit_cache |= bit_mask[value] << bbuf->bit_count;
			bbuf->bit_count += value + 1;
			value = 0;
		}

	} while (value);
}

__inline void bit_buffer_get_unary(TTA_bit_buffer* bbuf, unsigned long *value)
{
	*value = 0;

	while (!(bbuf->bit_cache ^ bit_mask[bbuf->bit_count])) {
		*value += bbuf->bit_count;
		bbuf->bit_cache = *bbuf->bitpos++;
		UPDATE_CRC32(bbuf->bit_cache, bbuf->crc32);
		bbuf->bit_count = 8;
	}

	while (bbuf->bit_cache & 1) {
		(*value)++;
		bbuf->bit_cache >>= 1;
		bbuf->bit_count--;
	}

	bbuf->bit_cache >>= 1;
	bbuf->bit_count--;
}

int bit_buffer_done_write(TTA_bit_buffer* bbuf)
{
	unsigned long bytes_to_write;

	while (bbuf->bit_count) {
		*bbuf->bitpos = (unsigned char) (bbuf->bit_cache & 0xFF);
		UPDATE_CRC32(*bbuf->bitpos, bbuf->crc32);
		bbuf->bit_cache >>= 8;
		bbuf->bit_count = (bbuf->bit_count > 8) ? (bbuf->bit_count - 8) : 0;
		bbuf->bitpos++;
	}

	bbuf->crc32 ^= 0xFFFFFFFFUL;
	bbuf->crc32 = ENDSWAP_INT32(bbuf->crc32);
	memcpy(bbuf->bitpos, &bbuf->crc32, 4);
	bytes_to_write = bbuf->bitpos + sizeof(long) - bbuf->bit_buffer;

	bbuf->bitpos = bbuf->bit_buffer;
	bbuf->crc32 = 0xFFFFFFFFUL;

	return bytes_to_write;
}

int bit_buffer_done_read(TTA_bit_buffer* bbuf)
{
	unsigned long crc32, res;
	bbuf->crc32 ^= 0xFFFFFFFFUL;

	memcpy(&crc32, bbuf->bitpos, 4);
	crc32 = ENDSWAP_INT32(crc32);
	bbuf->bitpos += sizeof(long);
	res = (crc32 != bbuf->crc32);

	bbuf->bit_cache = bbuf->bit_count = 0;
	bbuf->crc32 = 0xFFFFFFFFUL;

	return res;
}

void convert_input_buffer(long *dst, unsigned char *src, long byte_size, unsigned long len)
{
	unsigned char *start = src;
	switch (byte_size) {
	case 1: for (; src < start + len; dst++)
				*dst = (signed long) *src++ - 0x80;
			break;
	case 2: for (; src < start + (len * 2); dst++) {
				*dst = (unsigned char) *src++;
				*dst |= (signed char) *src++ << 8;
			}
			break;
	case 3: for (; src < start + (len * 3); dst++) {
				*dst = (unsigned char) *src++;
				*dst |= (unsigned char) *src++ << 8;
				*dst |= (signed char) *src++ << 16;
			}
			break;
	case 4: for (; src < start + (len * 4); dst += 2) {
				*dst = (unsigned char) *src++;
				*dst |= (unsigned char) *src++ << 8;
				*dst |= (unsigned char) *src++ << 16;
				*dst |= (signed char) *src++ << 24;
			}
			break;
	}
}

long convert_output_buffer(unsigned char *dst, long *data, long byte_size, long num_chan, unsigned long len)
{
	long *src = data;
	
	switch (byte_size) {
	case 1: for (; src < data + (len * num_chan); src++)
				*dst++ = (unsigned char) (*src + 0x80);
		break;
	case 2: for (; src < data + (len * num_chan); src++) {
		*dst++ = (unsigned char) *src;
		*dst++ = (unsigned char) (*src >> 8);
			}
		break;
	case 3: for (; src < data + (len * num_chan); src++) {
		*dst++ = (unsigned char) *src;
		*dst++ = (unsigned char) (*src >> 8);
		*dst++ = (unsigned char) (*src >> 16);
			}
		break;
	case 4: for (; src < data + (len * num_chan * 2); src += 2) {
		*dst++ = (unsigned char) *src;
		*dst++ = (unsigned char) (*src >> 8);
		*dst++ = (unsigned char) (*src >> 16);
		*dst++ = (unsigned char) (*src >> 24);
			}
		break;
	}

	return byte_size * len * num_chan;
}

void rice_init(TTA_adapt *rice, unsigned long k0, unsigned long k1)
{
	rice->k0 = k0;
	rice->k1 = k1;
	rice->sum0 = shift_16[k0];
	rice->sum1 = shift_16[k1];
}

void channels_codec_init(TTA_channel_codec *ttacc, long nch, long byte_size)
{
	long *fset = flt_set[byte_size - 1];
	long i;
	
	for (i = 0; i < nch; i++) {
		filter_init(&ttacc[i].fst, fset[0], fset[1]);
		rice_init(&ttacc[i].rice, 10, 10);
		ttacc[i].last = 0;
	}
}

void compress_data(TTA_codec* ttacodec, long *data, unsigned long len)
{
	long *p, tmp, prev;
	unsigned long value, k, unary, binary;
	
	for (p = data, prev = 0; p < data + len * ttacodec->codec_num_chan; p++) {
		TTA_fltst *fst = &ttacodec->enc->fst;
		TTA_adapt *rice = &ttacodec->enc->rice;
		long *last = &ttacodec->enc->last;

		if (!ttacodec->is_float) {
			if (ttacodec->enc < ttacodec->tta + ttacodec->codec_num_chan - 1)
				*p = prev = *(p + 1) - *p;
			else
				*p -= prev / 2;
		} else if (!((p - data) & 1)) {
			unsigned long t = *p;
			unsigned long negative = (t & 0x80000000) ? -1 : 1;
			unsigned long data_hi = (t & 0x7FFF0000) >> 16;
			unsigned long data_lo = (t & 0x0000FFFF);
			
			*p = (data_hi || data_lo) ? (data_hi - 0x3F80) : 0;
			*(p + 1) = (SWAP16(data_lo) + 1) * negative;
		}

		tmp = *p;
		switch (ttacodec->byte_size) {
		case 1:	*p -= PREDICTOR1(*last, 4); break;	// bps 8
		case 2:	*p -= PREDICTOR1(*last, 5);	break;	// bps 16
		case 3: *p -= PREDICTOR1(*last, 5); break;	// bps 24
		case 4: *p -= *last; break;			// bps 32
		}
		*last = tmp;

		hybrid_filter(fst, p, 1);

		value = ENC(*p);

		k = rice->k0;

		rice->sum0 += value - (rice->sum0 >> 4);
		if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
			rice->k0--;
		else if (rice->sum0 > shift_16[rice->k0 + 1])
			rice->k0++;

		if (value >= bit_shift[k]) {
			value -= bit_shift[k];
			k = rice->k1;

			rice->sum1 += value - (rice->sum1 >> 4);
			if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
				rice->k1--;
			else if (rice->sum1 > shift_16[rice->k1 + 1])
				rice->k1++;

			unary = 1 + (value >> k);
		} else
			unary = 0;

		bit_buffer_put_unary(ttacodec->fbuf, unary);
		if (k) {
			binary = value & bit_mask[k];
			bit_buffer_put_binary(ttacodec->fbuf, binary, k);
		}

		if (ttacodec->enc < ttacodec->tta + ttacodec->codec_num_chan - 1)
			ttacodec->enc++;
		else
			ttacodec->enc = ttacodec->tta;
	}
}

unsigned long decompress_data(TTA_codec* ttacodec, long *data)
{
	long *p, value;
	unsigned long k, depth, unary, binary;
	unsigned long decoded_pcm_frames = 0;

	for (p = data; ; p++) {
		TTA_fltst *fst = &ttacodec->enc->fst;
		TTA_adapt *rice = &ttacodec->enc->rice;
		long *last = &ttacodec->enc->last;

		bit_buffer_get_unary(ttacodec->fbuf, &unary);

		switch (unary) {
		case 0: depth = 0; k = rice->k0; break;
		default:
			depth = 1; k = rice->k1;
			unary--;
		}

		if (k) {
			bit_buffer_get_binary(ttacodec->fbuf, &binary, k);
			value = (unary << k) + binary;
		} else value = unary;

		switch (depth) {
		case 1: 
			rice->sum1 += value - (rice->sum1 >> 4);
			if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
				rice->k1--;
			else if (rice->sum1 > shift_16[rice->k1 + 1])
				rice->k1++;
			value += bit_shift[rice->k0];
		default:
			rice->sum0 += value - (rice->sum0 >> 4);
			if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
				rice->k0--;
			else if (rice->sum0 > shift_16[rice->k0 + 1])
				rice->k0++;
		}

		*p = DEC(value);

		hybrid_filter(fst, p, 0);

		switch (ttacodec->byte_size) {
		case 1: *p += PREDICTOR1(*last, 4); break;	// bps 8
		case 2: *p += PREDICTOR1(*last, 5); break;	// bps 16
		case 3: *p += PREDICTOR1(*last, 5); break;	// bps 24
		case 4: *p += *last; break;			// bps 32
		} *last = *p;

		if (ttacodec->is_float && ((p - data) & 1)) {
			unsigned long negative = *p & 0x80000000;
			unsigned long data_hi = *(p - 1);
			unsigned long data_lo = abs(*p) - 1;
			
			data_hi += (data_hi || data_lo) ? 0x3F80 : 0;
			*(p - 1) = (data_hi << 16) | SWAP16(data_lo) | negative;
		}

		if (ttacodec->enc < ttacodec->tta + ttacodec->codec_num_chan - 1)
			ttacodec->enc++;
		else {
			if (!ttacodec->is_float && ttacodec->codec_num_chan > 1) {
				long *r = p - 1;
				*p += *(p - 1) / 2;
				while (r > p - ttacodec->codec_num_chan)
					*r = *(r + 1) - *r--;
			}
			ttacodec->enc = ttacodec->tta;
			decoded_pcm_frames++;

			if(ttacodec->fbuf->bitpos >= ttacodec->fbuf->BIT_BUFFER_END)
			{
				break;
			}
		}
	}

	return decoded_pcm_frames;
}

unsigned long compress_one_frame(TTA_codec* ttacodec, long *data, unsigned long len)
{
	channels_codec_init(ttacodec->tta, ttacodec->codec_num_chan, ttacodec->byte_size);

	compress_data(ttacodec, data, len);

	return bit_buffer_done_write(ttacodec->fbuf);
}

unsigned long decompress_one_frame(TTA_codec* ttacodec, long *data, unsigned long len)
{
	unsigned long decoded_pcm_frames = 0;

	channels_codec_init(ttacodec->tta, ttacodec->codec_num_chan, ttacodec->byte_size);

	memcpy(ttacodec->fbuf->bit_buffer, data, len);
	ttacodec->fbuf->bitpos = ttacodec->fbuf->bit_buffer;
	ttacodec->fbuf->BIT_BUFFER_END = ttacodec->fbuf->bit_buffer + len - sizeof(ttacodec->fbuf->crc32);

	decoded_pcm_frames = decompress_data(ttacodec, ttacodec->data);

	if (bit_buffer_done_read(ttacodec->fbuf)) {

		ZeroMemory(ttacodec->data, len);
		bit_buffer_init_read(ttacodec->fbuf);
		return 0;
	}

	return decoded_pcm_frames;
}

TTA_codec* tta_codec_new(long srate, short num_chan, short bps, short format)
{
	TTA_codec* ttacodec = (TTA_codec*)tta_alloc(sizeof(TTA_codec));

	if(!ttacodec)
		return NULL;

	ttacodec->framelen = (long)(TTA_FRAME_TIME * srate);
	ttacodec->srate = srate;
	ttacodec->num_chan = num_chan;
	ttacodec->bps = bps;
	ttacodec->byte_size = (bps + 7) / 8;
	ttacodec->is_float = (bps == 32) || (format == WAVE_FORMAT_IEEE_FLOAT);
	ttacodec->wavformat = format;
	ttacodec->codec_num_chan = (ttacodec->num_chan << ttacodec->is_float);

	ttacodec->fbuf = bit_buffer_new(ttacodec->framelen * ttacodec->byte_size * ttacodec->codec_num_chan * (ttacodec->is_float ? 2 : 1) + sizeof(long));
	if(!ttacodec->fbuf)
	{
		tta_codec_free(ttacodec);
		return NULL;
	}

	ttacodec->data_len = 0;
	ttacodec->data = (long *) tta_alloc(ttacodec->codec_num_chan * ttacodec->framelen * sizeof(long) * (ttacodec->is_float ? 2 : 1));
	if(!ttacodec->data)
	{
		tta_codec_free(ttacodec);
		return NULL;
	}

	ttacodec->enc = ttacodec->tta = (TTA_channel_codec*)tta_alloc(ttacodec->codec_num_chan * sizeof(TTA_channel_codec));
	if(!ttacodec->tta)
	{
		tta_codec_free(ttacodec);
		return NULL;
	}

	return ttacodec;
}

void tta_codec_free(TTA_codec* ttacodec)
{
	if(ttacodec)
	{
		bit_buffer_free(ttacodec->fbuf);
		ttacodec->fbuf = NULL;
		if (ttacodec->data) {
			GlobalFree(ttacodec->data);
			ttacodec->data = NULL;
		}
		if (ttacodec->tta) {
			GlobalFree(ttacodec->tta = ttacodec->enc);
			ttacodec->enc = NULL;
			ttacodec->tta = NULL;
		}
		GlobalFree(ttacodec);
	}
}

void tta_codec_fill_header(TTA_codec* ttaenc, TTA_header *hdr, unsigned long data_len)
{
	hdr->TTAid = ENDSWAP_INT32(TTA1_SIGN);
	hdr->AudioFormat = ENDSWAP_INT16(ttaenc->wavformat);
	hdr->NumChannels = ENDSWAP_INT16(ttaenc->num_chan);
	hdr->BitsPerSample = ENDSWAP_INT16(ttaenc->bps);
	hdr->SampleRate = ENDSWAP_INT32(ttaenc->srate);
	hdr->DataLength = ENDSWAP_INT32(data_len);
	hdr->CRC32 = crc32((unsigned char *) &hdr, sizeof(TTA_header) - sizeof(long));
	hdr->CRC32 = ENDSWAP_INT32(hdr->CRC32);
}

unsigned long tta_codec_get_frame_len(long srate)
{
	return (long)(TTA_FRAME_TIME * srate);
}

TTA_codec* tta_codec_encoder_new(long srate, short num_chan, short bps, short format)
{
	TTA_codec* ttaenc = tta_codec_new(srate, num_chan, bps, format);

	if(ttaenc) {
		bit_buffer_init_write(ttaenc->fbuf);
	}

	return ttaenc;
}

unsigned long tta_codec_encoder_compress(TTA_codec* ttaenc, unsigned char *src, unsigned long len, unsigned long *bytes_to_write)
{
	long needed_data = (ttaenc->framelen - ttaenc->data_len);
	long data_available = (len / ttaenc->num_chan / (ttaenc->bps / 8));
	long consume_data = min(needed_data, data_available);

	convert_input_buffer(ttaenc->data + (ttaenc->data_len * ttaenc->num_chan), src, ttaenc->byte_size, consume_data * ttaenc->num_chan);

	ttaenc->data_len += consume_data;

	if(ttaenc->data_len == ttaenc->framelen)
	{
		*bytes_to_write = compress_one_frame(ttaenc, ttaenc->data, ttaenc->framelen);
	}

	return (consume_data * ttaenc->num_chan * (ttaenc->bps / 8));
}

unsigned char* tta_codec_encoder_get_frame(TTA_codec* ttaenc, unsigned long *num_frames)
{
	*num_frames = ttaenc->data_len;
	ttaenc->data_len = 0;
	return ttaenc->fbuf->bit_buffer;
}

unsigned long tta_codec_encoder_finalize(TTA_codec* ttaenc)
{
	unsigned long bytes_to_write = 0;

	if(ttaenc->data_len > 0)
	{
		bytes_to_write = compress_one_frame(ttaenc, ttaenc->data, ttaenc->data_len);
	}

	return bytes_to_write;
}

TTA_codec* tta_codec_decoder_new(long srate, short num_chan, short bps)
{
	TTA_codec* ttadec = tta_codec_new(srate, num_chan, bps, 0);

	if(ttadec) {
		bit_buffer_init_read(ttadec->fbuf);
	}

	return ttadec;
}

unsigned long tta_codec_decoder_decompress(TTA_codec* ttadec, unsigned char *src, unsigned long len, unsigned char *dst, unsigned long dstlen)
{
	unsigned long decoded_pcm_frames = 0, decoded_pcm_frames_bytes = 0;

	decoded_pcm_frames = decompress_one_frame(ttadec, (long*)src, len);
	decoded_pcm_frames_bytes = decoded_pcm_frames * ttadec->byte_size * ttadec->num_chan;

	if(decoded_pcm_frames_bytes > dstlen)
		return 0;

	convert_output_buffer(dst, ttadec->data, ttadec->byte_size,
		ttadec->num_chan, decoded_pcm_frames);

	return (decoded_pcm_frames_bytes);
}

TTA_parser* tta_parser_new(TTA_io_callback* io)
{
	unsigned long i, checksum, st_size;

	TTA_parser* ttaparser = (TTA_parser*)tta_alloc(sizeof(TTA_parser));

	if(!ttaparser)
		return NULL;

	ttaparser->io = io;
	ttaparser->DataOffset = 0;

	if(!io->read(io, &ttaparser->id3v2, sizeof(TTA_id3v2_header)))
	{
		return NULL;
	}

	if (!memcmp(ttaparser->id3v2.id, "ID3", 3)) {	
		if ((ttaparser->id3v2.size[0] |
			 ttaparser->id3v2.size[1] |
			 ttaparser->id3v2.size[2] |
			 ttaparser->id3v2.size[3]) & 0x80) {
			return NULL;
		}

		ttaparser->ID3v2Len = (ttaparser->id3v2.size[0] << 21) |
			(ttaparser->id3v2.size[1] << 14) |
			(ttaparser->id3v2.size[2] << 7) |
			ttaparser->id3v2.size[3];
		ttaparser->ID3v2Len += 10;

		if (ttaparser->id3v2.flags & (1 << 4))
			ttaparser->ID3v2Len += 10;

		if(!io->seek(io, ttaparser->ID3v2Len, SEEK_CUR))
			return NULL;

		ttaparser->DataOffset = sizeof(TTA_id3v2_header) + ttaparser->ID3v2Len;
	} else {
		if(!io->seek(io, 0, SEEK_SET))
			return NULL;		
	}

	if(!io->read(io, &ttaparser->TTAHeader, sizeof(TTA_header)))
	{
		return NULL;
	}

	ttaparser->DataOffset += sizeof(TTA_header);

	if (ttaparser->TTAHeader.TTAid != TTA1_SIGN) {
		return NULL;
	}

	checksum = crc32((unsigned char *) &ttaparser->TTAHeader, sizeof(TTA_header) - sizeof(long));

	if (checksum != ttaparser->TTAHeader.CRC32) {
		return NULL;
	}

	ttaparser->FrameLen = tta_codec_get_frame_len(ttaparser->TTAHeader.SampleRate);
	ttaparser->LastFrameLen = ttaparser->TTAHeader.DataLength % ttaparser->FrameLen;
	ttaparser->FrameTotal = ttaparser->TTAHeader.DataLength / ttaparser->FrameLen + (ttaparser->LastFrameLen ? 1 : 0);
	st_size = (ttaparser->FrameTotal + 1); // +1 for crc32

	ttaparser->SeekTable = (unsigned long*)tta_alloc(st_size * sizeof(long));

	ttaparser->SeekOffsetTable = (unsigned long*)tta_alloc(st_size * sizeof(long));

	if(!io->read(io, ttaparser->SeekTable, st_size * sizeof(long)))
	{
		return NULL;
	}

	ttaparser->DataOffset += (st_size * sizeof(long));

	io->seek(io, 0, SEEK_SET);
	ttaparser->extradata_size = ttaparser->DataOffset;
	io->read(io, ttaparser->extradata, ttaparser->extradata_size);

	checksum = crc32((unsigned char *) ttaparser->SeekTable, ((st_size - 1) * sizeof(long)));

	if (checksum != ttaparser->SeekTable[st_size - 1])
	{
		return NULL;
	}

	ttaparser->SeekOffsetTable[0] = ttaparser->DataOffset;

	for(i = 1; i < (st_size-1); i++)
	{		
		ttaparser->SeekOffsetTable[i] = ttaparser->SeekOffsetTable[i-1] + ttaparser->SeekTable[i-1];
	}

	ttaparser->MaxFrameLenBytes = 0;
	for(i = 0; i < (st_size-1); i++)
	{		
		if(ttaparser->SeekTable[i] > ttaparser->MaxFrameLenBytes)
			ttaparser->MaxFrameLenBytes = ttaparser->SeekTable[i];
	}

	ttaparser->FrameIndex = 0;

	return ttaparser;
}

unsigned long tta_parser_read_frame(TTA_parser* ttaparser, unsigned char* dst, unsigned long* FrameIndex, unsigned long* FrameLen)
{
	unsigned long CurrentFrameLen, FrameLenBytes;

	if(tta_parser_eof(ttaparser))
		return 0;

	CurrentFrameLen = ttaparser->FrameLen;

	if(ttaparser->FrameIndex == (ttaparser->FrameTotal - 1))
		CurrentFrameLen = ttaparser->LastFrameLen;

	FrameLenBytes = ttaparser->SeekTable[ttaparser->FrameIndex];

	if(!ttaparser->io->read(ttaparser->io, dst, FrameLenBytes))
	{
		return 0;
	}

	*FrameIndex = ttaparser->FrameIndex;
	*FrameLen = CurrentFrameLen;

	ttaparser->FrameIndex++;

	return FrameLenBytes;
}

void tta_parser_seek(TTA_parser* ttaparser, uint64 seek_pos_100ns)
{
	ttaparser->FrameIndex = (long)(seek_pos_100ns / (TTA_FRAME_TIME * 10000000));

	if(tta_parser_eof(ttaparser))
		return;

	ttaparser->io->seek(ttaparser->io, ttaparser->SeekOffsetTable[ttaparser->FrameIndex], SEEK_SET);
}

int tta_parser_eof(TTA_parser* ttaparser)
{
	return (ttaparser->FrameIndex >= ttaparser->FrameTotal);
}

void tta_parser_free(TTA_parser** ttaparser)
{
	if(!(*ttaparser))
		return;

	if((*ttaparser)->SeekOffsetTable)
	{
		GlobalFree((*ttaparser)->SeekOffsetTable);
		(*ttaparser)->SeekOffsetTable = NULL;
	}

	if((*ttaparser)->SeekTable)
	{
		GlobalFree((*ttaparser)->SeekTable);
		(*ttaparser)->SeekTable = NULL;
	}

	GlobalFree(*ttaparser);

	*ttaparser = NULL;
}
