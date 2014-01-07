#include "stdafx.h"
#include <MMReg.h>
#include <moreuuids.h>

#include "APEFile.h"

/* The earliest and latest file formats supported */
#define APE_MIN_VERSION 3800
#define APE_MAX_VERSION 3990

#define MAC_FORMAT_FLAG_8_BIT                 1 // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2 // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4 // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8 // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16 // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32 // create the wave header on decompression (not stored)

#define APE_EXTRADATA_SIZE 6

typedef struct {
    /* Derived fields */
    uint32_t junklength;
    uint32_t firstframe;
    uint32_t totalsamples;
    int currentframe;

    /* Info from Descriptor Block */
    char magic[4];
    int16_t fileversion;
    int16_t padding1;
    uint32_t descriptorlength;
    uint32_t headerlength;
    uint32_t seektablelength;
    uint32_t wavheaderlength;
    uint32_t audiodatalength;
    uint32_t audiodatalength_high;
    uint32_t wavtaillength;
    uint8_t md5[16];

    /* Info from Header Block */
    uint16_t compressiontype;
    uint16_t formatflags;
    uint32_t blocksperframe;
    uint32_t finalframeblocks;
    uint32_t totalframes;
    uint16_t bps;
    uint16_t channels;
    uint32_t samplerate;

    /* Seektable */
    uint32_t* seektable;
    uint8_t*  bittable;
} APEContext;

//
// CAPEFile
//

CAPEFile::CAPEFile()
	: CAudioFile()
	, m_curentframe(0)
	, m_APETag(NULL)
{
	m_subtype = MEDIASUBTYPE_APE;
}

HRESULT CAPEFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	
	int version = 0;
	m_pFile->ByteRead((BYTE*)&version, 2);
	if (version < APE_MIN_VERSION || version > APE_MAX_VERSION) {
		return E_FAIL;
	}

	APEContext ape;
	memset(&ape, 0, sizeof(APEContext));
	ape.fileversion = version;

	if (ape.fileversion >= 3980) {
		m_pFile->ByteRead((BYTE*)&ape.padding1            , 2);
		m_pFile->ByteRead((BYTE*)&ape.descriptorlength    , 4);
		m_pFile->ByteRead((BYTE*)&ape.headerlength        , 4);
		m_pFile->ByteRead((BYTE*)&ape.seektablelength     , 4);
		m_pFile->ByteRead((BYTE*)&ape.wavheaderlength     , 4);
		m_pFile->ByteRead((BYTE*)&ape.audiodatalength     , 4);
		m_pFile->ByteRead((BYTE*)&ape.audiodatalength_high, 4);
		m_pFile->ByteRead((BYTE*)&ape.wavtaillength       , 4);
		m_pFile->ByteRead((BYTE*)&ape.md5                 , 16);

		// Skip any unknown bytes at the end of the descriptor. This is for future compatibility
		if (ape.descriptorlength > 52) {
			m_pFile->Seek(m_pFile->GetPos() + ape.descriptorlength - 52);
		}

		// Read header data
		m_pFile->ByteRead((BYTE*)&ape.compressiontype , 2);
		m_pFile->ByteRead((BYTE*)&ape.formatflags     , 2);
		m_pFile->ByteRead((BYTE*)&ape.blocksperframe  , 4);
		m_pFile->ByteRead((BYTE*)&ape.finalframeblocks, 4);
		m_pFile->ByteRead((BYTE*)&ape.totalframes     , 4);
		m_pFile->ByteRead((BYTE*)&ape.bps             , 2);
		m_pFile->ByteRead((BYTE*)&ape.channels        , 2);
		m_pFile->ByteRead((BYTE*)&ape.samplerate      , 4);
	} else {
		ape.descriptorlength = 0;
		ape.headerlength = 32;

		m_pFile->ByteRead((BYTE*)&ape.compressiontype , 2);
		m_pFile->ByteRead((BYTE*)&ape.formatflags     , 2);
		m_pFile->ByteRead((BYTE*)&ape.channels        , 2);
		m_pFile->ByteRead((BYTE*)&ape.samplerate      , 4);
		m_pFile->ByteRead((BYTE*)&ape.wavheaderlength , 4);
		m_pFile->ByteRead((BYTE*)&ape.wavtaillength   , 4);
		m_pFile->ByteRead((BYTE*)&ape.totalframes     , 4);
		m_pFile->ByteRead((BYTE*)&ape.finalframeblocks, 4);

		if (ape.formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL) {
			m_pFile->Seek(m_pFile->GetPos() + 4);
			ape.headerlength += 4;
		}

		if (ape.formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS) {
			m_pFile->ByteRead((BYTE*)&ape.seektablelength, 4);
			ape.headerlength += 4;
			ape.seektablelength *= sizeof(int32_t);
		} else {
			ape.seektablelength = ape.totalframes * sizeof(int32_t);
		}

		if (ape.formatflags & MAC_FORMAT_FLAG_8_BIT) {
			ape.bps = 8;
		} else if (ape.formatflags & MAC_FORMAT_FLAG_24_BIT) {
			ape.bps = 24;
		} else {
			ape.bps = 16;
		}

		if (ape.fileversion >= 3950) {
			ape.blocksperframe = 73728 * 4;
		} else if (ape.fileversion >= 3900 || (ape.fileversion >= 3800  && ape.compressiontype >= 4000)) {
			ape.blocksperframe = 73728;
		} else {
			ape.blocksperframe = 9216;
		}

		// Skip any stored wav header
		if (!(ape.formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER)) {
			m_pFile->Seek(m_pFile->GetPos() + ape.wavheaderlength);
		}
	}

	if(!ape.totalframes){
		TRACE(L"No frames in the file!\n");
		return E_FAIL;
	}
	if(ape.totalframes > UINT_MAX / sizeof(APEFrame)){
		TRACE(L"Too many frames: %d\n", ape.totalframes);
		return E_FAIL;
	}
	if (ape.seektablelength / sizeof(*ape.seektable) < ape.totalframes) {
		TRACE(L"Number of seek entries is less than number of frames: %zu vs. %d\n", ape.seektablelength / sizeof(*ape.seektable), ape.totalframes);
		return E_FAIL;
	}
	if (!m_frames.SetCount(ape.totalframes)) {
		return E_OUTOFMEMORY;
	}
	ape.firstframe = ape.junklength + ape.descriptorlength + ape.headerlength + ape.seektablelength + ape.wavheaderlength;
	if (ape.fileversion < 3810)
		ape.firstframe += ape.totalframes;
	ape.currentframe = 0;

	ape.totalsamples = ape.finalframeblocks;
	if (ape.totalframes > 1) {
		ape.totalsamples += ape.blocksperframe * (ape.totalframes - 1);
	}

	if (ape.seektablelength > 0) {
		ape.seektable = (uint32_t*)malloc(ape.seektablelength);
		if (!ape.seektable) {
			return E_OUTOFMEMORY;
		}
		memset(ape.seektable, 0, ape.seektablelength);
		for (size_t i = 0; i < ape.seektablelength / sizeof(uint32_t) && m_pFile->GetAvailable() >= 4; i++) {
			m_pFile->ByteRead((BYTE*)&ape.seektable[i], 4);
		}
		if (ape.fileversion < 3810) {
			ape.bittable = (uint8_t*)malloc(ape.totalframes);
			if (!ape.bittable) {
				return E_OUTOFMEMORY;
			}
			memset(ape.bittable, 0, ape.totalframes);
			for (size_t i = 0; i < ape.totalframes && m_pFile->GetAvailable(); i++) {
				m_pFile->ByteRead((BYTE*)&ape.bittable[i], 1);
			}
		}
		if (!m_pFile->GetAvailable()) {
			TRACE(L"File truncated\n");
		}
	}

	m_frames[0].pos     = ape.firstframe;
	m_frames[0].nblocks = ape.blocksperframe;
	m_frames[0].skip    = 0;
	for (size_t i = 1; i < ape.totalframes; i++) {
		m_frames[i].pos      = ape.seektable[i] + ape.junklength;
		m_frames[i].nblocks  = ape.blocksperframe;
		m_frames[i - 1].size = m_frames[i].pos - m_frames[i - 1].pos;
		m_frames[i].skip     = (m_frames[i].pos - m_frames[0].pos) & 3;
	}
	m_frames[ape.totalframes - 1].nblocks = ape.finalframeblocks;
	/* calculate final packet size from total file size, if available */
	__int64 file_size = m_pFile->GetLength(); //?
	int final_size = 0;
	if (file_size > 0) {
		int final_size = file_size - m_frames[ape.totalframes - 1].pos - ape.wavtaillength;
		final_size -= final_size & 3;
	}
	if (file_size <= 0 || final_size <= 0) {
		final_size = ape.finalframeblocks * 8;
	}
	m_frames[ape.totalframes - 1].size = final_size;

	for (size_t i = 0; i < ape.totalframes; i++) {
		if(m_frames[i].skip){
			m_frames[i].pos  -= m_frames[i].skip;
			m_frames[i].size += m_frames[i].skip;
		}
		m_frames[i].size = (m_frames[i].size + 3) & ~3;
	}
	if (ape.fileversion < 3810) {
		for (size_t i = 0; i < ape.totalframes; i++) {
			if (i < ape.totalframes - 1 && ape.bittable[i + 1]) {
				m_frames[i].size += 4;
			}
			m_frames[i].skip <<= 3;
			m_frames[i].skip  += ape.bittable[i];
		}
	}

	int total_blocks = (ape.totalframes == 0) ? 0 : ((ape.totalframes - 1) * ape.blocksperframe) + ape.finalframeblocks;

	int64_t pts = 0;
	for (size_t i = 0; i < ape.totalframes; i++) {
		m_frames[i].pts = pts;
		pts += ape.blocksperframe;
	}

	m_samplerate	= ape.samplerate;
	m_bitdepth		= ape.bps;
	m_channels		= ape.channels;
	if (m_channels == 1) {
		m_layout = SPEAKER_FRONT_CENTER;
	} else if (m_channels == 2) {
		m_layout = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	}
	m_rtduration	= 10000000i64 * total_blocks / m_samplerate;

	m_extrasize = APE_EXTRADATA_SIZE;
	m_extradata = (BYTE*)malloc(m_extrasize);
	memcpy(m_extradata + 0, &ape.fileversion, 2);
	memcpy(m_extradata + 2, &ape.compressiontype, 2);
	memcpy(m_extradata + 4, &ape.formatflags, 2);

	m_startpos = m_pFile->GetPos();
	m_endpos = m_pFile->GetLength();

	m_pFile->Seek(m_startpos);

	return S_OK;
}

REFERENCE_TIME CAPEFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_curentframe = 0;
		m_pFile->Seek(m_frames[0].pos);
		return 0;
	}

	int64_t pts = rt * m_samplerate / 10000000;

	size_t i;
	for (i = 1; i < m_frames.GetCount(); i++) {
		if (m_frames[i].pts > pts) {
			break;
		}
	}
	m_curentframe = i - 1;

	m_pFile->Seek(m_frames[m_curentframe].pos);
	rt = m_frames[m_curentframe].pts * 10000000 / m_samplerate;

	return rt;
}

int CAPEFile::GetAudioFrame(Packet* packet)
{
	if (m_curentframe >= m_frames.GetCount()) {
		return 0;
	}
	int size = m_frames[m_curentframe].size + 8;
	packet->SetCount(size);

	memcpy(packet->GetData(), &m_frames[m_curentframe].nblocks, 4);
	memcpy(packet->GetData() + 4, &m_frames[m_curentframe].skip, 4);

	m_pFile->Seek(m_frames[m_curentframe].pos);
	m_pFile->ByteRead(packet->GetData() + 8, size - 8);

	packet->rtStart = m_frames[m_curentframe].pts * 10000000 / m_samplerate;
	packet->rtStop  = (m_frames[m_curentframe].pts + m_frames[m_curentframe].nblocks) * 10000000 / m_samplerate;

	m_curentframe++;

	return size;
}

