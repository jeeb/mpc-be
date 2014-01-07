#pragma once

#include "AudioFile.h"

#define ID_TAK 'tBaK'

class CTAKFile : public CAudioFile
{
	__int64		m_samples;
	int			m_framelen;
	int			m_totalframes;

	bool ParseTAKStreamInfo(BYTE* buf, int size);

public:
	CTAKFile();

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(Packet* packet);
};
