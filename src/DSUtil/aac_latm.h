//-----------------------------------------------------------------------------
//
//	Monogram Multimedia AAC Decoder
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

#include "aac_classes.h"
#include "bits.h"

//-----------------------------------------------------------------------------
//
//	CLATMReader class
//
//-----------------------------------------------------------------------------
class CLATMReader
{
public:

	// internals
	int						streamId[16][8];		

	// config data
	CArray<int>				progSIndex;
	CArray<int>				laySIndex;
	CArray<CAACConfig*>		config;
	uint8					frameLengthTypes[128];
	uint8					latmBufferFullness[128];
	uint16					frameLength[128];
	uint16					muxSlotLengthBytes[128];
	uint8					muxSlotLengthCoded[128];
	uint8					numLayer[16];
	uint8					progCIndx[128];
	uint8					layCIndx[128];
	uint8					AuEndFlag[128];

	uint8			audio_mux_version;
	uint8			audio_mux_version_A;
	uint8			all_same_framing;
	int				taraFullness;
	uint8			config_crc;
	int64			other_data_bits;
	int				numProgram;
	int				numSubFrames;
	int				streamCnt;
	int				numChunk;

	void FlushAudioSpecificConfig();
public:
	CLATMReader();
	virtual ~CLATMReader();

	// only reads Config information
	int	ReadConfig(BYTE *buf, int size);

	// Extracts LATM payload
	int ReadPacket(BYTE *buf, int size, BYTE *payload, int *payloadsize);

	int StreamMuxConfig(Bitstream &b);
	int PayloadLengthInfo(Bitstream &b);

};
