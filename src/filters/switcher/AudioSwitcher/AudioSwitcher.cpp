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
#include <math.h>

#define NORMALIZATION_REGAIN_STEP      20
#define NORMALIZATION_REGAIN_THRESHOLD 0.75

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
	, m_rtNextStop(1)
	, m_fNormalize(false)
	, m_iRecoverStep(NORMALIZATION_REGAIN_STEP)
	, m_normalizeFactor(4.0)
	, m_boost_mul(1.0f)
{
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
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if (!pInPin || !pOutPin) {
		return __super::Transform(pIn, pOut);
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;
	WAVEFORMATEX* wfeout = (WAVEFORMATEX*)pOutPin->CurrentMediaType().pbFormat;
	WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;

	int bps = wfe->wBitsPerSample>>3;

	int len = pIn->GetActualDataLength() / (bps*wfe->nChannels);
	int lenout = (UINT64)len * wfeout->nSamplesPerSec / wfe->nSamplesPerSec;

	REFERENCE_TIME rtStart, rtStop;
	if (SUCCEEDED(pIn->GetTime(&rtStart, &rtStop))) {
		rtStart += m_rtAudioTimeShift;
		rtStop += m_rtAudioTimeShift;
		pOut->SetTime(&rtStart, &rtStop);

		m_rtNextStart = rtStart;
		m_rtNextStop = rtStop;
	} else {
		pOut->SetTime(&m_rtNextStart, &m_rtNextStop);
	}

	REFERENCE_TIME rtDur = 10000000i64*len/wfe->nSamplesPerSec;

	m_rtNextStart += rtDur;
	m_rtNextStop += rtDur;

	if (pIn->IsDiscontinuity() == S_OK) {
		m_normalizeFactor = 4.0;
	}

	WORD tag = wfe->wFormatTag;
	bool fPCM = tag == WAVE_FORMAT_PCM || (tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM);
	bool fFloat = tag == WAVE_FORMAT_IEEE_FLOAT || (tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
	if (!fPCM && !fFloat) {
		return __super::Transform(pIn, pOut);
	}

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	HRESULT hr;
	if (FAILED(hr = pIn->GetPointer(&pDataIn))) {
		return hr;
	}
	if (FAILED(hr = pOut->GetPointer(&pDataOut))) {
		return hr;
	}

	if (!pDataIn || !pDataOut || len < 0 || lenout < 0) {
		return S_FALSE;
	}
	// len = 0 doesn't mean it's failed, return S_OK otherwise might screw the sound
	if (len == 0) {
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	{
		HRESULT hr2;
		if (S_OK != (hr2 = __super::Transform(pIn, pOut))) {
			return hr2;
		}
	}

	if (m_fNormalize ||  m_boost_mul > 1.0f) {
		size_t samples = lenout * wfeout->nChannels;
		double sample_mul = 1.0;

		if (m_fNormalize) {
			double sample_max = 0.0;

			// calculate max peak
			if (fPCM) {
				int32_t maxpeak = 0;
				if (wfe->wBitsPerSample == 8) {
					for (size_t i = 0; i < samples; i++) {
						int32_t peak = abs((int8_t)(pDataOut[i] ^ 0x80));
						if (peak > maxpeak) {
							maxpeak = peak;
						}
					}
					sample_max = (double)maxpeak / INT8_MAX;
				} else if (wfe->wBitsPerSample == 16) {
					for (size_t i = 0; i < samples; i++) {
						int32_t peak = abs(((int16_t*)pDataOut)[i]);
						if (peak > maxpeak) {
							maxpeak = peak;
						}
					}
					sample_max = (double)maxpeak / INT16_MAX;
				} else if (wfe->wBitsPerSample == 24) {
					for (size_t i = 0; i < samples; i++) {
						int32_t peak = 0;
						BYTE* p = (BYTE*)(&peak);
						p[1] = pDataOut[i * 3];
						p[2] = pDataOut[i * 3 + 1];
						p[3] = pDataOut[i * 3 + 2];
						peak = abs(peak);
						if (peak > maxpeak) {
							maxpeak = peak;
						}
					}
					sample_max = (double)maxpeak / INT32_MAX;
				} else if (wfe->wBitsPerSample == 32) {
					for (size_t i = 0; i < samples; i++) {
						int32_t peak = abs(((int32_t*)pDataOut)[i]);
						if (peak > maxpeak) {
							maxpeak = peak;
						}
					}
					sample_max = (double)maxpeak / INT32_MAX;
				}
			} else if (fFloat) {
				if (wfe->wBitsPerSample == 32) {
					for (size_t i = 0; i < samples; i++) {
						double sample = (double)abs(((float*)pDataOut)[i]);
						if (sample > sample_max) {
							sample_max = sample;
						}
					}
				} else if (wfe->wBitsPerSample == 64) {
					for (size_t i = 0; i < samples; i++) {
						double sample = (double)abs(((double*)pDataOut)[i]);
						if (sample > sample_max) {
							sample_max = sample;
						}
					}
				}
			}

			double normFact = 1.0 / sample_max;
			if (m_normalizeFactor > normFact) {
				m_normalizeFactor = normFact;
			} else if (sample_max * m_normalizeFactor < NORMALIZATION_REGAIN_THRESHOLD) { // we don't regain if we are too close of the maximum
				m_normalizeFactor += (double)m_iRecoverStep * rtDur / (10000000 * 100); // the step is per second so we weight it with the duration
			}

			if (m_normalizeFactor > 4.0) {
				m_normalizeFactor = 4.0;
			}

			sample_mul = m_normalizeFactor;
		}

		if (m_boost_mul > 1.0f) {
			sample_mul *= m_boost_mul;
		}

		if (sample_mul != 1.0) {
			if (fPCM) {
				if (wfe->wBitsPerSample == 8) {
					gain_uint8(sample_mul, samples, (uint8_t*)pDataOut);
				} else if(wfe->wBitsPerSample == 16) {
					gain_int16(sample_mul, samples, (int16_t*)pDataOut);
				} else if(wfe->wBitsPerSample == 24) {
					gain_int24(sample_mul, samples, pDataOut);
				} else if(wfe->wBitsPerSample == 32) {
					gain_int32(sample_mul, samples, (int32_t*)pDataOut);
				}
			} else if (fFloat) {
				if (wfe->wBitsPerSample == 32) {
					gain_float(sample_mul, samples, (float*)pDataOut);
				} else if(wfe->wBitsPerSample == 64) {
					gain_double(sample_mul, samples, (double*)pDataOut);
				}
			}
		}
	}

	pOut->SetActualDataLength(lenout * bps * wfeout->nChannels);

	return S_OK;
}

CMediaType CAudioSwitcherFilter::CreateNewOutputMediaType(CMediaType mt, long& cbBuffer)
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if (!pInPin || !pOutPin || ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		return __super::CreateNewOutputMediaType(mt, cbBuffer);
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;

	CorrectWaveFormatEx(&mt);

	WAVEFORMATEX* wfeout = (WAVEFORMATEX*)mt.pbFormat;

	int bps		= wfe->wBitsPerSample>>3;
	int len		= cbBuffer / (bps*wfe->nChannels);
	int lenout	= (UINT64)len * wfeout->nSamplesPerSec / wfe->nSamplesPerSec;
	cbBuffer	= lenout*bps*wfeout->nChannels;

	//	mt.lSampleSize = (ULONG)max(mt.lSampleSize, wfe->nAvgBytesPerSec * rtLen / 10000000i64);
	//	mt.lSampleSize = (mt.lSampleSize + (wfe->nBlockAlign-1)) & ~(wfe->nBlockAlign-1);

	return mt;
}

void CAudioSwitcherFilter::OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut)
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::OnNewOutputMediaType()"));
	m_normalizeFactor = 4.0;
}

HRESULT CAudioSwitcherFilter::DeliverEndFlush()
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::DeliverEndFlush()"));
	m_normalizeFactor = 4.0;

	return __super::DeliverEndFlush();
}

HRESULT CAudioSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::DeliverNewSegment()"));
	m_normalizeFactor = 4.0;
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

STDMETHODIMP CAudioSwitcherFilter::GetNormalizeBoost(bool& fNormalize, int& iRecoverStep, float& boost_dB)
{
	fNormalize = m_fNormalize;
	iRecoverStep = m_iRecoverStep;
	boost_dB = 20*log10(m_boost_mul);
	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetNormalizeBoost(bool fNormalize, int iRecoverStep, float boost_dB)
{
	if (m_fNormalize != fNormalize) {
		m_normalizeFactor = 4.0;
	}
	m_fNormalize = fNormalize;
	m_iRecoverStep = min(max(10, iRecoverStep), 200);
	m_boost_mul = pow(10.0f, boost_dB/20);
	return S_OK;
}

// IAMStreamSelect

STDMETHODIMP CAudioSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	HRESULT hr = __super::Enable(lIndex, dwFlags);
	if (S_OK == hr) {
		m_normalizeFactor = 4.0;
	}
	return hr;
}
