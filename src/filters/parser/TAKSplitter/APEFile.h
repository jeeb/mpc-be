#pragma once

#include "AudioFile.h"

#define ID_APE 'MAC '

class CAPEFile : public CAudioFile
{
	typedef struct {
		int64_t pos;
		int nblocks;
		int size;
		int skip;
		int64_t pts;
	} APEFrame;

	size_t				m_curentframe;
	CAtlArray<APEFrame>	m_frames;

public:
	CAPEFile();

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(Packet* packet);
};
