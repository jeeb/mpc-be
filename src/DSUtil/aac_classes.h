//-----------------------------------------------------------------------------
//
//	Monogram Multimedia AAC Decoder
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

#include "bits.h"

class CLATMReader;

//-----------------------------------------------------------------------------
//
//	CAACBuffer class
//
//	Various demuxers seem to deliver data in different chunks/sizes. This
//	buffer accumulates data until a complete frame can be read.
//
//-----------------------------------------------------------------------------
class CAACBuffer
{
public:
	uint8	*buf;				// allocated buffer
	int		max_size;			// total size of buffer
	int		count;				// number of bytes written in buffer

public:
	CAACBuffer(int size=64*1024);
	virtual ~CAACBuffer();

	void Flush(int bytes_to_flush);
	void Clear();
	void Write(uint8 *data, int len);

	operator Bitstream() { return Bitstream(buf); }
};

//-----------------------------------------------------------------------------
//
//	CAACConfig class
//
//-----------------------------------------------------------------------------
class CAACConfig
{
public:
	BYTE	extra[64];			// should be way enough
	int		extrasize;

	int		audioObjectType;
	int		samplingFrequencyIndex;
	int		samplingFrequency;
	int		channelConfiguration;

	int GASpecificConfig(Bitstream &b, Bitstream &bo);

public:
	CAACConfig();
	CAACConfig(const CAACConfig &c) { Assign((CAACConfig*)&c); }
	CAACConfig &operator =(const CAACConfig &c) { Assign((CAACConfig*)&c); return *this; }
	virtual ~CAACConfig();

	// parse out the configuration and/or construct extradata
	int Read(WAVEFORMATEX *wfx);
	int Read(Bitstream &b);
	void Reset();

	// copy data
	void Assign(CAACConfig *c);
};
