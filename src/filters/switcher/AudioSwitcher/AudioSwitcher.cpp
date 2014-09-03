/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "AudioSwitcher.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioTools.h"
#include "../../../AudioTools/AudioHelper.h"
#include <math.h>

#ifdef REGISTER_FILTER

#include <InitGuid.h>
#endif
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CAudioSwitcherFilter), AudioSwitcherName, MERIT_DO_NOT_USE, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CAudioSwitcherFilter>, NULL, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CAudioSwitcherFilter
//

CAudioSwitcherFilter::CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CStreamSwitcherFilter(lpunk, phr, __uuidof(this))
	, m_rtAudioTimeShift(0)
	, m_rtNextStart(0)
	, m_bAutoVolumeControl(false)
	, m_iNormLevel(75)
	, m_iNormRealeaseTime(8)
	, m_buffer(NULL)
	, m_buf_size(0)
	, m_fGain_dB(0.0f)
	, m_fGainFactor(1.0f)
{
	m_AudioNormalizer.SetParam(m_iNormLevel, true, m_iNormRealeaseTime);

	if (phr) {
		if (FAILED(*phr)) {
			return;
		} else {
			*phr = S_OK;
		}
	}
}

STDMETHODIMP CAudioSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAudioSwitcherFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAudioSwitcherFilter::CheckMediaType(const CMediaType* pmt)
{
	return (pmt->majortype == MEDIATYPE_Audio
			&& pmt->formattype == FORMAT_WaveFormatEx
			&& (((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 8
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 16
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 24
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 32)
			&& (((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_PCM
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_IEEE_FLOAT
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_EXTENSIBLE))
		   ? S_OK
		   : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CAudioSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	HRESULT hr;

	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if (!pInPin || !pOutPin) {
		return __super::Transform(pIn, pOut);
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;
	WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;

	SampleFormat sample_format = GetSampleFormat(wfe);
	if (sample_format == SAMPLE_FMT_NONE) {
		return __super::Transform(pIn, pOut);
	}

	const unsigned in_bytespersample = wfe->wBitsPerSample / 8;
	const unsigned in_samples        = pIn->GetActualDataLength() / (wfe->nChannels * in_bytespersample);
	const unsigned in_allsamples     = in_samples * wfe->nChannels;

	REFERENCE_TIME rtDur = 10000000i64 * in_samples / wfe->nSamplesPerSec;

	REFERENCE_TIME rtStart, rtStop;
	if (FAILED(pIn->GetTime(&rtStart, &rtStop))) {
		rtStart = m_rtNextStart;
		rtStop  = m_rtNextStart + rtDur;
	}
	m_rtNextStart = rtStop;

	//if (pIn->IsDiscontinuity() == S_OK) {
	//}

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;
	if (FAILED(hr = pIn->GetPointer(&pDataIn)) || FAILED(hr = pOut->GetPointer(&pDataOut))) {
		return hr;
	}

	if (!pDataIn || !pDataOut || in_samples < 0) {
		return S_FALSE;
	}

	// in_samples = 0 doesn't mean it's failed, return S_OK otherwise might screw the sound
	if (in_samples == 0) {
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	if (m_bAutoVolumeControl) {
		int out_samples = 0;
		long out_size = 0;

		if (sample_format == SAMPLE_FMT_FLT) {
			if (S_OK != (hr = __super::Transform(pIn, pOut))) {
				return hr;
			}
			out_samples = m_AudioNormalizer.MSteadyHQ32((float*)pDataOut, in_samples, wfe->nChannels);
			out_size = out_samples * wfe->nChannels * get_bytes_per_sample(sample_format);
		} else {
			if (in_allsamples > m_buf_size) {
				if (m_buffer) {
					delete[] m_buffer;
				}
				m_buf_size = in_allsamples;
				m_buffer = DNew float[m_buf_size];
			}

			convert_to_float(sample_format, wfe->nChannels, in_samples, pDataIn, m_buffer);
			out_samples = m_AudioNormalizer.MSteadyHQ32(m_buffer, in_samples, wfe->nChannels);
			out_size = out_samples * wfe->nChannels * get_bytes_per_sample(sample_format);

			if (out_size > pOut->GetSize()) {
				ASSERT(0);
				return E_FAIL;
			}

			convert_float_to(sample_format, wfe->nChannels, in_samples, m_buffer, pDataOut);
		}

		pOut->SetActualDataLength(out_size);
	} else {
		// copy input data to output and SetActualDataLength for output
		if (S_OK != (hr = __super::Transform(pIn, pOut))) {
			return hr;
		}

		if (m_fGainFactor != 1.0f) {
			size_t out_allsamples = pOut->GetActualDataLength() / get_bytes_per_sample(sample_format);

			switch (sample_format) {
			case SAMPLE_FMT_U8:
				gain_uint8(m_fGainFactor, out_allsamples, (uint8_t*)pDataOut);
				break;
			case SAMPLE_FMT_S16:
				gain_int16(m_fGainFactor, out_allsamples, (int16_t*)pDataOut);
				break;
			case SAMPLE_FMT_S24:
				gain_int24(m_fGainFactor, out_allsamples, pDataOut);
				break;
			case SAMPLE_FMT_S32:
				gain_int32(m_fGainFactor, out_allsamples, (int32_t*)pDataOut);
				break;
			case SAMPLE_FMT_FLT:
				gain_float(m_fGainFactor, out_allsamples, (float*)pDataOut);
				break;
			case SAMPLE_FMT_DBL:
				gain_double(m_fGainFactor, out_allsamples, (double*)pDataOut);
				break;
			}
		}
	}

	rtStart += m_rtAudioTimeShift;
	rtStop  += m_rtAudioTimeShift;
	pOut->SetTime(&rtStart, &rtStop);

	return S_OK;
}

void CAudioSwitcherFilter::TransformMediaType(CMediaType& mt)
{
	CorrectWaveFormatEx(mt); // fix incorrect WAVEFORMATEX structure

	// TODO: add change mediatype after transform (channel mixer and other).
}

HRESULT CAudioSwitcherFilter::DeliverEndFlush()
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::DeliverEndFlush()"));

	return __super::DeliverEndFlush();
}

HRESULT CAudioSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::DeliverNewSegment()"));

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

// IAudioSwitcherFilter

STDMETHODIMP_(REFERENCE_TIME) CAudioSwitcherFilter::GetAudioTimeShift()
{
	return m_rtAudioTimeShift;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift)
{
	m_rtAudioTimeShift = rtAudioTimeShift;
	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetAutoVolumeControl(bool& bAutoVolumeControl, bool& bNormBoost, int& iNormLevel, int& iNormRealeaseTime)
{
	bAutoVolumeControl	= m_bAutoVolumeControl;
	bNormBoost			= m_bNormBoost;
	iNormLevel			= m_iNormLevel;
	iNormRealeaseTime	= m_iNormRealeaseTime;

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetAutoVolumeControl(bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime)
{
	m_bAutoVolumeControl	= bAutoVolumeControl;
	m_bNormBoost			= bNormBoost;
	m_iNormLevel				= min(max(0, iNormLevel), 100);
	m_iNormRealeaseTime		= min(max(5, iNormRealeaseTime), 10);

	m_AudioNormalizer.SetParam(m_iNormLevel, bNormBoost, m_iNormRealeaseTime);

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioGain(float fGain_dB)
{
	m_fGain_dB = min(max(-3.0f, fGain_dB), 10.0f);
	m_fGainFactor = pow(10.0f, m_fGain_dB/20.0f);

	return S_OK;
}

// IAMStreamSelect

STDMETHODIMP CAudioSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	HRESULT hr = __super::Enable(lIndex, dwFlags);

	return hr;
}
