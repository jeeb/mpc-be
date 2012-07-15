#pragma once

#include "../BaseSplitter/BaseSplitter.h"

class CMP4SplitterFile : public CBaseSplitterFileEx
{
	void* m_pAp4File;

	HRESULT Init();

public:
	CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr);
	virtual ~CMP4SplitterFile();

	void* GetMovie();
};
