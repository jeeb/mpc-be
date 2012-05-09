//-----------------------------------------------------------------------------
//
//	Monogram Multimedia AAC Decoder
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "aac_latm.h"

//-----------------------------------------------------------------------------
//
//	CLATMReader class
//
//-----------------------------------------------------------------------------

CLATMReader::CLATMReader()
{
	// zero internals
	memset(frameLengthTypes, 0, sizeof(frameLengthTypes));
	memset(latmBufferFullness, 0, sizeof(latmBufferFullness));
	memset(frameLength, 0, sizeof(frameLength));
	memset(muxSlotLengthBytes, 0, sizeof(muxSlotLengthBytes));
	memset(muxSlotLengthBytes, 0, sizeof(muxSlotLengthBytes));
	memset(numLayer, 0, sizeof(numLayer));
	memset(progCIndx, 0, sizeof(progCIndx));
	memset(layCIndx, 0, sizeof(layCIndx));
	memset(AuEndFlag, 0, sizeof(AuEndFlag));
}

CLATMReader::~CLATMReader()
{
	FlushAudioSpecificConfig();
}

int CLATMReader::StreamMuxConfig(Bitstream &b)
{
	streamCnt = 0;
	numSubFrames;

	progSIndex.RemoveAll();
	laySIndex.RemoveAll();
	FlushAudioSpecificConfig();

	audio_mux_version_A = 0;
	audio_mux_version = b.UGetBits(1);
	if (audio_mux_version == 1) {				// audioMuxVersion
		audio_mux_version_A = b.UGetBits(1);
	}

	b.NeedBits();
	if (audio_mux_version_A == 0) {

		if (audio_mux_version == 1) {
			taraFullness = b.LatmGetValue();
		}
		b.NeedBits();
		all_same_framing = b.UGetBits(1);
		numSubFrames = b.UGetBits(6);
		numProgram = b.UGetBits(4);

		for (int prog=0; prog<=numProgram; prog++) {
			
			b.NeedBits();
			int numLayer = b.UGetBits(3);
			int objTypes[8];
			this->numLayer[prog] = numLayer;

			for (int lay=0; lay<=numLayer; lay++) {
				CAACConfig	cfg;
				b.NeedBits();

				progSIndex.Add(prog);
				laySIndex.Add(lay);
				streamId[prog][lay]=streamCnt;
				streamCnt++;

				uint8 use_same_config;
				if (prog == 0 && lay == 0) {
					use_same_config = 0;
				} else {
					use_same_config = b.UGetBits(1);
				}

				if (!use_same_config) {
					if (audio_mux_version == 0) {
						// audio specific config.
						cfg.Read(b);
					} else {
						return -1;
#if 0
						int ascLen = b.LatmGetValue();

						ascLen -= cfg.Read(b);

						// fill bits
						ascLen -= 16;
						while (ascLen > 0) {
							b.NeedBits();
							b.DumpBits(16);
							ascLen -= 16;
						}
						ascLen += 16;
						b.NeedBits();
						b.DumpBits(ascLen);
#endif
					}

					// store config
					objTypes[lay] = cfg.audioObjectType;
					config.Add(new CAACConfig(cfg));
				} else {
					// same as before
					objTypes[lay] = objTypes[lay-1];
					config.Add(new CAACConfig(*(config[lay-1])));
				}


				// these are not needed... perhaps
				b.NeedBits();
				int frame_length_type = b.UGetBits(3);
				frameLengthTypes[streamId[prog][lay]] = frame_length_type;
				if (frame_length_type == 0) {
					latmBufferFullness[streamId[prog][lay]] = b.UGetBits(8);
					if (!all_same_framing) {
						if ((objTypes[lay] == 6 ||
							 objTypes[lay] == 20) &&
							(objTypes[lay-1] == 8 ||
							 objTypes[lay-1] == 24)) {
							b.NeedBits();
							int core_frame_offset = b.UGetBits(6);
						}
					}
				} else
				if (frame_length_type == 1) {
					frameLength[streamId[prog][lay]] = b.UGetBits(9);
				} else
				if (frame_length_type == 3 ||
					frame_length_type == 4 ||
					frame_length_type == 5) {
					int celp_table_index = b.UGetBits(6);
				} else
				if (frame_length_type == 6 ||
					frame_length_type == 7) {
					int hvxc_table_index = b.UGetBits(1);
				}

			}
		}

		// other data
		b.NeedBits();
		other_data_bits = 0;
		if (b.UGetBits(1)) {
			// other data present
			if (audio_mux_version == 1) {
				other_data_bits = b.LatmGetValue();
			} else {
				// other data not present
				other_data_bits = 0;
				int esc, tmp;
				do {
					other_data_bits <<= 8;
					b.NeedBits();
					esc = b.UGetBits(1);
					tmp = b.UGetBits(8);
					other_data_bits |= tmp;
				} while (esc);
			}
		}

		// CRC
		b.NeedBits();
		if (b.UGetBits(1)) {
			config_crc = b.UGetBits(8);
		}

	} else {
		// tbd
	}

	return 0;
}

int CLATMReader::ReadConfig(BYTE *buf, int size)
{
	// ISO/IEC 14496-3 Table 1.28 - Syntax of AudioMuxElement()
	Bitstream	b(buf);
	b.NeedBits();

	if (b.UGetBits(11) != 0x2b7) return -1;		// not LATM
	b.NeedBits();

	int muxlength = b.UGetBits(13);

	uint8	use_same_mux = b.UGetBits(1);
	if (!use_same_mux) {
		int ret = StreamMuxConfig(b);
		if (ret < 0) return ret;
	}

	// we don't parse anything else here...
	return 0;
}

int CLATMReader::ReadPacket(BYTE *buf, int size, BYTE *payload, int *payloadsize)
{
	// ISO/IEC 14496-3 Table 1.28 - Syntax of AudioMuxElement()
	Bitstream	b(buf);
	b.NeedBits();

	if (b.UGetBits(11) != 0x2b7) return -1;		// not LATM
	b.NeedBits();
	int muxlength = b.UGetBits(13);

	if (3+muxlength > size) return 0;			// not enough data

	uint8	use_same_mux = b.UGetBits(1);
	if (!use_same_mux) {
		int ret = StreamMuxConfig(b);
		if (ret < 0) return ret;
	}

	if (audio_mux_version_A == 0) {
		for (int i=0; i <= numSubFrames; i++) {
			PayloadLengthInfo(b);

			// copy data
			for (int j=0; j<muxSlotLengthBytes[0]; j++) {
				b.NeedBits();
				*payload++ = b.UGetBits(8);
			}
			*payloadsize = muxSlotLengthBytes[0];
		}

	} else {
		// TBD
	}

	// we don't parse anything else here...
	return (3+muxlength);
}

void CLATMReader::FlushAudioSpecificConfig()
{
	for (int i=0; i<config.GetCount(); i++) {
		delete config[i];
	}
	config.RemoveAll();
}

int CLATMReader::PayloadLengthInfo(Bitstream &b)
{
	uint8 tmp;
	if (all_same_framing) {
		for (int prog=0; prog<=numProgram; prog++) {
			for (int lay=0; lay<=numLayer[prog]; lay++) {
				if (frameLengthTypes[streamId[prog][lay]] == 0) {
					muxSlotLengthBytes[streamId[prog][lay]] = 0;
					do {
						b.NeedBits();
						tmp = b.UGetBits(8);
						muxSlotLengthBytes[streamId[prog][lay]] += tmp;
					} while (tmp == 255);
				} else {
					if (frameLengthTypes[streamId[prog][lay]] == 5 ||
						frameLengthTypes[streamId[prog][lay]] == 7 ||
						frameLengthTypes[streamId[prog][lay]] == 3) {
						b.NeedBits();
						muxSlotLengthCoded[streamId[prog][lay]] = b.UGetBits(2);
					}
				}
			}
		}
	} else {
		b.NeedBits();
		numChunk = b.UGetBits(4);
		for (int chunkCnt=0; chunkCnt <= numChunk; chunkCnt++) {
			b.NeedBits();
			uint8 streamIndx = b.UGetBits(4);
			int prog = progCIndx[chunkCnt] = progSIndex[streamIndx];
			int lay  = layCIndx[chunkCnt]  = laySIndex[streamIndx];
			if (frameLengthTypes[streamId[prog][lay]] == 0) {
				muxSlotLengthBytes[streamId[prog][lay]] = 0;
				do {
					b.NeedBits();
					tmp = b.UGetBits(8);
					muxSlotLengthBytes[streamId[prog][lay]] += tmp;
				} while (tmp == 255);
				AuEndFlag[streamId[prog][lay]] = b.UGetBits(1);
			} else {
				if (frameLengthTypes[streamId[prog][lay]] == 5 ||
					frameLengthTypes[streamId[prog][lay]] == 7 ||
					frameLengthTypes[streamId[prog][lay]] == 3) {
					b.NeedBits();
					muxSlotLengthCoded[streamId[prog][lay]] = b.UGetBits(2);
				}
			}
		}
	}
	return 0;
}

