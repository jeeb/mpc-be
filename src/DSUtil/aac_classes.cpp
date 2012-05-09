//-----------------------------------------------------------------------------
//
//	Monogram Multimedia AAC Decoder
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "aac_classes.h"

const int AAC_Sample_Rates[] = {
		96000, 88200, 64000, 48000,
		44100, 32000, 24000, 22050, 
		16000, 12000, 11025, 8000,
		7350, 0, 0, 0
};

//-----------------------------------------------------------------------------
//
//	CAACBuffer class
//
//-----------------------------------------------------------------------------

CAACBuffer::CAACBuffer(int size) :
	max_size(size),
	count(0)
{
	buf = (uint8*)malloc(max_size);
	memset(buf, 0, max_size);
}

CAACBuffer::~CAACBuffer()
{
	free(buf);
}

void CAACBuffer::Clear()
{
	count = 0;
}

void CAACBuffer::Flush(int bytes_to_flush)
{
	bytes_to_flush = min(count, bytes_to_flush);
	int left = (count - bytes_to_flush);

	if (left > 0) {
		memcpy(buf, buf+bytes_to_flush, left);
	}
	count = left;
}

void CAACBuffer::Write(uint8 *data, int len)
{
	// buffer overflow check... just ignore the data before
	if (count + len > max_size) {
		int to_flush = (count+len) - max_size;
		Flush(to_flush);
	}

	// append data
	memcpy(buf+count, data, len);
	count += len;
}


//-----------------------------------------------------------------------------
//
//	CAACConfig class
//
//-----------------------------------------------------------------------------
CAACConfig::CAACConfig() 
{
	Reset();
}

CAACConfig::~CAACConfig()
{
}

void CAACConfig::Reset()
{
	extrasize = 0;
	audioObjectType = 0;
	samplingFrequencyIndex = 0;
	samplingFrequency = 0;
	channelConfiguration = 0;
}

int CAACConfig::Read(WAVEFORMATEX *wfx)
{
	extrasize = wfx->cbSize;
	if (extrasize > sizeof(extra)) return -1;	// way too much

	// copy data
	uint8 *src = ((uint8*)wfx) + sizeof(WAVEFORMATEX);
	memcpy(extra, src, extrasize);

	// now parse it
	Bitstream b(extra);

	// this will modify the "extra" buffer. But since we're writing
	// the same data, it's ok.
	Read(b);	
	return 0;
}

void CAACConfig::Assign(CAACConfig *c)
{
	memcpy(extra, c->extra, c->extrasize);
	extrasize = c->extrasize;

	audioObjectType = c->audioObjectType;
	samplingFrequencyIndex = c->samplingFrequencyIndex;
	samplingFrequency = c->samplingFrequency;
	channelConfiguration = c->channelConfiguration;
}

int CAACConfig::Read(Bitstream &b)
{
	Bitstream o(extra);

	// returns the number of bits read
	int ret = 0;
	int sbr_present = -1;

	b.NeedBits();

	// object
	audioObjectType = b.UGetBits(5);				ret += 5;
	o.PutBits(audioObjectType, 5);
	if (audioObjectType == 31) {
		uint8 n = b.UGetBits(6);		o.PutBits(n, 6);
		audioObjectType = 32 + n;					ret += 6;
		b.NeedBits();
	}

	samplingFrequencyIndex = b.UGetBits(4);			ret += 4;
	samplingFrequency = AAC_Sample_Rates[samplingFrequencyIndex];
	o.PutBits(samplingFrequencyIndex, 4);
	if (samplingFrequencyIndex == 0x0f) {
		b.NeedBits24();
		uint32 f = b.UGetBits(24);		o.PutBits(f, 24);
		o.PutBits(f, 24);
		samplingFrequency = f;						ret += 24;
		b.NeedBits();
	}
	channelConfiguration = b.UGetBits(4);			ret += 4;
	o.PutBits(channelConfiguration, 4);

	if (audioObjectType == 5) {
		sbr_present = 1;

		// TODO: parsing !!!!!!!!!!!!!!!!
	}

	switch (audioObjectType) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 6:
	case 7:
	case 17:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
		ret += GASpecificConfig(b, o);
		break;
	}

	if (sbr_present == -1) {
		if (samplingFrequency <= 24000) {
			samplingFrequency *= 2;
		}			
	}

	// count the extradata
	o.Put_ByteAlign_Zero();
	extrasize = (ret + 7) >> 3;
	return ret;
}

int CAACConfig::GASpecificConfig(Bitstream &b, Bitstream &bo)
{
	b.NeedBits();
	int ret=0;

	int framelen_flag = b.UGetBits(1);		ret += 1;
	bo.PutBits(framelen_flag, 1);
	int dependsOnCoder = b.UGetBits(1);		ret += 1;
	bo.PutBits(dependsOnCoder, 1);
	int ext_flag;
	int delay;
	int layerNr;

	if (dependsOnCoder) {
		delay = b.UGetBits(14);				ret += 14;
		bo.PutBits(delay, 14);
		b.NeedBits();		
	}
	ext_flag = b.UGetBits(1);				ret += 1;
	bo.PutBits(ext_flag, 1);
	if (!channelConfiguration) {
		// program config element
		// TODO:
	}

	if (audioObjectType == 6 || audioObjectType == 20) {
		layerNr = b.UGetBits(3);			ret += 3;
		bo.PutBits(layerNr, 3);
	}
	b.NeedBits();
	if (ext_flag) {
		if (audioObjectType == 22) {
			b.DumpBits(5);					ret += 5;	// numOfSubFrame
			b.DumpBits(11);					ret += 11;	// layer_length
			b.NeedBits();

			bo.PutBits(0, 16);
		}
		if (audioObjectType == 17 ||
			audioObjectType == 19 ||
			audioObjectType == 20 ||
			audioObjectType == 23) {

			b.DumpBits(3);					ret += 3;	// stuff
			bo.PutBits(0, 3);
		}

		b.DumpBits(1);						ret += 1;	// extflag3
		bo.PutBits(0, 1);
	}
	return ret;
}
