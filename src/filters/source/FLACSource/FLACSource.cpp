/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include <MMReg.h>
#include <ks.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <uuids.h>
#include <moreuuids.h>
#include "FLACSource.h"
#include "../../../DSUtil/DSUtil.h"
#include <libflac/include/FLAC/stream_decoder.h>

#define _DECODER_   (FLAC__StreamDecoder*)m_pDecoder

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_FLAC_FRAMED}
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CFLACSource), FlacSourceName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CFLACSource>, NULL, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"),
		_T("0"), _T("0,4,,664C6143"));

	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"),
		_T("Source Filter"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".flac"),
		_T("Source Filter"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".flac"));

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../FilterApp.h"

CFilterApp theApp;

#endif


// Declaration for FLAC callbacks
static FLAC__StreamDecoderReadStatus	StreamDecoderRead(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus	StreamDecoderSeek(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus	StreamDecoderTell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus	StreamDecoderLength(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool						StreamDecoderEof(const FLAC__StreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus	StreamDecoderWrite(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void								StreamDecoderError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void								StreamDecoderMetadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);


//
// CFLACSource
//

CFLACSource::CFLACSource(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseSource<CFLACStream>(NAME("CFLACSource"), lpunk, phr, __uuidof(this))
{
}

CFLACSource::~CFLACSource()
{
}

STDMETHODIMP CFLACSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI2(IAMMediaContent)
		QI(IDSMResourceBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAMMediaContent

STDMETHODIMP CFLACSource::get_AuthorName(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	file_info_struct file_info = (static_cast<CFLACStream*>(m_paStreams[0]))->GetInfo();
	if (file_info.got_vorbis_comments) {
		*pbstrTitle = file_info.artist.AllocSysString();
		return S_OK;
	}
	return E_UNEXPECTED;
}

STDMETHODIMP CFLACSource::get_Title(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);
	
	file_info_struct file_info = (static_cast<CFLACStream*>(m_paStreams[0]))->GetInfo();
	if (file_info.got_vorbis_comments) {
		CString Title	= file_info.title;
		CString Year	= file_info.year;

		if (!Title.IsEmpty() && !Year.IsEmpty()) {
			Title += _T(" (") + Year + _T(")");
		}

		*pbstrTitle = Title.AllocSysString();
		return S_OK;
	}
	return E_UNEXPECTED;
}

STDMETHODIMP CFLACSource::get_Description(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);
	
	file_info_struct file_info = (static_cast<CFLACStream*>(m_paStreams[0]))->GetInfo();
	if (file_info.got_vorbis_comments) {
		*pbstrTitle = file_info.comment.AllocSysString();
		return S_OK;
	}
	return E_UNEXPECTED;
}

STDMETHODIMP CFLACSource::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));
	wcscpy_s(pInfo->achName, FlacSourceName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// IDSMResourceBag
STDMETHODIMP_(DWORD) CFLACSource::ResGetCount()
{
	return (static_cast<CFLACStream*>(m_paStreams[0]))->m_Cover.IsEmpty() ? 0 : 1;
}

STDMETHODIMP CFLACSource::ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag)
{
	if (ppData) {
		CheckPointer(pDataLen, E_POINTER);
	}

	if (iIndex) {
		return E_INVALIDARG;
	}

	CFLACStream* stream = (static_cast<CFLACStream*>(m_paStreams[0]));
	CheckPointer(stream, E_NOTIMPL);

	if (stream->m_Cover.IsEmpty()) {
		return E_NOTIMPL;
	}

	if (ppName) {
		CString str = _T("cover.jpg");
		*ppName = str.AllocSysString();
	}
	if (ppDesc) {
		CString str = _T("cover");
		*ppDesc = str.AllocSysString();
	}
	if (ppMime) {
		CString str = stream->m_CoverMime;
		*ppMime = str.AllocSysString();
	}
	if (ppData) {
		*pDataLen = (DWORD)stream->m_Cover.GetCount();
		memcpy(*ppData = (BYTE*)CoTaskMemAlloc(*pDataLen), stream->m_Cover.GetData(), *pDataLen);
	}
	if (pTag) {
		*pTag = 0;
	}

	return S_OK;
}

// CFLACStream

CFLACStream::CFLACStream(const WCHAR* wfn, CSource* pParent, HRESULT* phr)
	: CBaseStream(NAME("CFLACStream"), pParent, phr)
	, m_bIsEOF (false)
	, m_pDecoder (NULL)
{
	CAutoLock		cAutoLock(&m_cSharedState);
	CString			fn(wfn);
	CFileException	ex;
	HRESULT			hr = E_FAIL;

	file_info.got_vorbis_comments = false;

	do {
		if (!m_file.Open(fn, CFile::modeRead|CFile::shareDenyNone, &ex)) {
			hr	= AmHresultFromWin32 (ex.m_lOsError);
			break;
		}

		m_pDecoder = FLAC__stream_decoder_new();
		if (!m_pDecoder) {
			break;
		}

		FLAC__stream_decoder_set_metadata_respond(_DECODER_, FLAC__METADATA_TYPE_VORBIS_COMMENT);
		FLAC__stream_decoder_set_metadata_respond(_DECODER_, FLAC__METADATA_TYPE_PICTURE); 

		if (FLAC__STREAM_DECODER_INIT_STATUS_OK != FLAC__stream_decoder_init_stream (_DECODER_,
				StreamDecoderRead,
				StreamDecoderSeek,
				StreamDecoderTell,
				StreamDecoderLength,
				StreamDecoderEof,
				StreamDecoderWrite,
				StreamDecoderMetadata,
				StreamDecoderError,
				this)) {
			break;
		}

		if (!FLAC__stream_decoder_process_until_end_of_metadata (_DECODER_) ||
				!FLAC__stream_decoder_seek_absolute (_DECODER_, 0)) {
			break;
		}

		FLAC__stream_decoder_get_decode_position(_DECODER_, &m_llOffset);

		hr = S_OK;
	} while (false);

	if (phr) {
		*phr = hr;
	}
}

CFLACStream::~CFLACStream()
{
	if (m_pDecoder) {
		FLAC__stream_decoder_delete (_DECODER_);
		m_pDecoder = NULL;
	}
}

HRESULT CFLACStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_nMaxFrameSize;

	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}
	ASSERT(Actual.cBuffers == pProperties->cBuffers);

	return NOERROR;
}

HRESULT CFLACStream::FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len)
{
	FLAC__uint64	llCurPos;
	FLAC__uint64	llNextPos;

	if (m_bDiscontinuity) {
		FLAC__stream_decoder_seek_absolute (_DECODER_, (m_rtPosition * m_i64TotalNumSamples) / m_rtDuration);
	}

	FLAC__stream_decoder_get_decode_position(_DECODER_, &llCurPos);

	FLAC__stream_decoder_skip_single_frame (_DECODER_);
	if (m_bIsEOF) {
		return S_FALSE;
	}
	FLAC__stream_decoder_get_decode_position(_DECODER_, &llNextPos);

	FLAC__uint64	llCurFile = m_file.GetPosition();
	len = (long)(llNextPos - llCurPos);
	ASSERT (len > 0);
	if (len <= 0) {
		return S_FALSE;
	}

	m_file.Seek (llCurPos, CFile::begin);
	m_file.Read (pOut, len);
	m_file.Seek (llCurFile, CFile::begin);

	m_AvgTimePerFrame = m_rtDuration * len / (m_llFileSize-m_llOffset);

	return S_OK;
}

HRESULT CFLACStream::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (iPosition == 0) {
		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= MEDIASUBTYPE_FLAC_FRAMED;
		pmt->formattype			= FORMAT_WaveFormatEx;
		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag			= WAVE_FORMAT_FLAC;
		wfe->nChannels			= m_nChannels;
		wfe->nSamplesPerSec		= m_nSamplesPerSec;
		wfe->nAvgBytesPerSec	= m_nAvgBytesPerSec;
		wfe->nBlockAlign		= 1;
		wfe->wBitsPerSample		= m_wBitsPerSample;
		wfe->cbSize				= 0;
	} else {
		return VFW_S_NO_MORE_ITEMS;
	}

	pmt->SetTemporalCompression(FALSE);

	return S_OK;
}

HRESULT CFLACStream::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->majortype		== MEDIATYPE_Audio
		&& pmt->subtype		== MEDIASUBTYPE_FLAC_FRAMED
		&& pmt->formattype	== FORMAT_WaveFormatEx
		&& ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_FLAC) {
		return S_OK;
	} else {
		return E_INVALIDARG;
	}
}

static bool ParseVorbisComment(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry, CString* TagValue){
	CString TagValues[2] = {0, 0};
	*TagValue = _T("");

	CString vorvis_data(entry->entry);
	
	int tPos = 0, count = 0;
	CString vorvis_data_field = vorvis_data.Tokenize(_T("="), tPos);
	while (tPos != -1 && (count <= 2)) {
		TagValues[count] = vorvis_data_field;
		vorvis_data_field = vorvis_data.Tokenize(_T("="), tPos);
		count++;
	}

	if (count == 2) {
		CString TagId(field_name);
		if(!TagId.CompareNoCase(TagValues[0])) {
			*TagValue = TagValues[1];
			return true;
		}
	}

	return false;
}

void CFLACStream::UpdateFromMetadata (void* pBuffer)
{
	const FLAC__StreamMetadata* pMetadata = (const FLAC__StreamMetadata*) pBuffer;

	if (pMetadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		m_nMaxFrameSize			= pMetadata->data.stream_info.max_framesize;
		m_nSamplesPerSec		= pMetadata->data.stream_info.sample_rate;
		m_nChannels				= pMetadata->data.stream_info.channels;
		m_wBitsPerSample		= pMetadata->data.stream_info.bits_per_sample;
		m_i64TotalNumSamples	= pMetadata->data.stream_info.total_samples;
		m_nAvgBytesPerSec		= (m_nChannels * (m_wBitsPerSample >> 3)) * m_nSamplesPerSec;

		// === Init members from base classes
		GetFileSizeEx (m_file.m_hFile, (LARGE_INTEGER*)&m_llFileSize);
		m_rtDuration			= (m_i64TotalNumSamples * UNITS) / m_nSamplesPerSec;
		m_rtStop				= m_rtDuration;
		m_AvgTimePerFrame		= (m_nMaxFrameSize + pMetadata->data.stream_info.min_framesize) * m_rtDuration / 2 / m_llFileSize;
	} else if (pMetadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		const FLAC__StreamMetadata_VorbisComment *vc = &pMetadata->data.vorbis_comment;
		
		file_info.got_vorbis_comments = (vc->num_comments > 0);
		for(unsigned i = 0; i < vc->num_comments; i++) {
			CString TagValue = _T("");
			if (ParseVorbisComment("artist", &vc->comments[i], &TagValue)) {
				file_info.artist	= TagValue;
			} else  if (ParseVorbisComment("title", &vc->comments[i], &TagValue)) {
				file_info.title		= TagValue;
			} else  if (ParseVorbisComment("description", &vc->comments[i], &TagValue)) {
				file_info.comment	= TagValue;
			} else  if (ParseVorbisComment("comment", &vc->comments[i], &TagValue)) {
				file_info.comment	= TagValue;
			} else  if (ParseVorbisComment("date", &vc->comments[i], &TagValue)) {
				file_info.year		= TagValue.GetAllocLength() > 4 ? TagValue.Right(4) : TagValue;
			}
		}
	} else if (pMetadata->type == FLAC__METADATA_TYPE_PICTURE) {
		if (!m_Cover.GetCount() && pMetadata->data.picture.data_length) {
			m_CoverMime = pMetadata->data.picture.mime_type;
			m_Cover.SetCount(pMetadata->data.picture.data_length);
			memcpy(m_Cover.GetData(), pMetadata->data.picture.data, pMetadata->data.picture.data_length);
		}
	}
}

file_info_struct CFLACStream::GetInfo()
{
	return file_info;
}

FLAC__StreamDecoderReadStatus StreamDecoderRead(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);
	UINT			nRead = pThis->GetFile()->Read(buffer, (UINT)*bytes);

	pThis->m_bIsEOF	= (nRead != *bytes);
	*bytes			= nRead;

	return (*bytes == 0) ?  FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM : FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus StreamDecoderSeek(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*> (client_data);

	pThis->m_bIsEOF	= false;
	pThis->GetFile()->Seek (absolute_byte_offset, CFile::begin);
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus StreamDecoderTell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*> (client_data);
	*absolute_byte_offset = pThis->GetFile()->GetPosition();
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus StreamDecoderLength(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*> (client_data);
	CFile*			pFile = pThis->GetFile();

	if (pFile == NULL) {
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
	} else {
		*stream_length = pFile->GetLength();
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool StreamDecoderEof(const FLAC__StreamDecoder *decoder, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*> (client_data);

	return pThis->m_bIsEOF;
}

FLAC__StreamDecoderWriteStatus StreamDecoderWrite(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*> (client_data);
	UNREFERENCED_PARAMETER(pThis);

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void StreamDecoderError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
}

void StreamDecoderMetadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*> (client_data);

	if (pThis) {
		pThis->UpdateFromMetadata ((void*)metadata);
	}
}
