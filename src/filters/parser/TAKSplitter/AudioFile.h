#pragma once

#include "../BaseSplitter/BaseSplitter.h"

#pragma warning(disable: 4005 4244)
extern "C" {
	#include <stdint.h>
}

class CAudioFile
{
protected:
	CBaseSplitterFile* m_pFile;

	__int64			m_startpos;
	__int64			m_endpos;

	int				m_samplerate;
	int				m_bitdepth;
	int				m_channels;
	DWORD			m_layout;

	//__int64			m_totalsamples;
	REFERENCE_TIME	m_rtduration;


	GUID			m_subtype;
	BYTE*			m_extradata;
	int				m_extrasize;

public:
	CAudioFile();
	~CAudioFile();

	__int64 GetStartPos() { return m_startpos; }
	__int64 GetEndPos() { return m_endpos; }
	REFERENCE_TIME	GetDuration() { return m_rtduration;}

	HRESULT Open(CBaseSplitterFile* pFile);
	bool SetMediaType(CMediaType& mt);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	//int GetAudioFrame(BYTE* buf, REFERENCE_TIME& rt, int& samples);
};
