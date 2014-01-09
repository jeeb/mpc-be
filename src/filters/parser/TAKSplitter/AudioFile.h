#pragma once

#include "../BaseSplitter/BaseSplitter.h"
#include "../DSUtil/ApeTag.h"

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
	virtual ~CAudioFile();

	CAPETag*		m_APETag;

	__int64 GetStartPos() const { return m_startpos; }
	__int64 GetEndPos() const { return m_endpos; }
	REFERENCE_TIME GetDuration() const { return m_rtduration;}
	bool SetMediaType(CMediaType& mt);

	virtual HRESULT Open(CBaseSplitterFile* pFile) PURE;
	virtual REFERENCE_TIME Seek(REFERENCE_TIME rt) PURE;
	virtual int GetAudioFrame(Packet* packet) PURE;
};
