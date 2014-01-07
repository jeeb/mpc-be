#include "stdafx.h"
#include "AudioFile.h"
//
// CAudioFile
//

CAudioFile::CAudioFile()
	: m_pFile(NULL)
	, m_startpos(0)
	, m_endpos(0)
	, m_samplerate(0)
	, m_bitdepth(0)
	, m_channels(0)
	, m_layout(0)
	, m_rtduration(0)
	, m_subtype(GUID_NULL)
	, m_extradata(NULL)
	, m_extrasize(0)
{
}

CAudioFile::~CAudioFile()
{
	m_pFile = NULL;
}

HRESULT CAudioFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	return S_OK;
}

bool CAudioFile::SetMediaType(CMediaType& mt)
{
	mt.majortype		= MEDIATYPE_Audio;
	mt.formattype		= FORMAT_WaveFormatEx;
	mt.subtype			= m_subtype;
	mt.SetSampleSize(256000);

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + m_extrasize);
	wfe->wFormatTag			= 0;
	wfe->nChannels			= m_channels;
	wfe->nSamplesPerSec		= m_samplerate;
	wfe->wBitsPerSample		= m_bitdepth;
	wfe->nBlockAlign		= m_channels * m_bitdepth / 8;
	wfe->nAvgBytesPerSec	= wfe->nSamplesPerSec * wfe->nBlockAlign;
	wfe->cbSize				= m_extrasize;
	memcpy(wfe + 1, m_extradata, m_extrasize);

	return true;
}

REFERENCE_TIME CAudioFile::Seek(REFERENCE_TIME rt)
{
	m_pFile->Seek(m_startpos);
	return 0;
}

int CAudioFile::GetAudioFrame(Packet* packet)
{
	return 0;
}
