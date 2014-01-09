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

#define NORMALIZATION_REGAIN_STEP      0.06 // +6%/s
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
	, m_fCustomChannelMapping(false)
	, m_rtAudioTimeShift(0)
	, m_rtNextStart(0)
	, m_rtNextStop(1)
	, m_fNormalize(false)
	, m_fNormalizeRecover(false)
	, m_normalizeFactor(4.0)
	, m_boost_mul(1.0f)
{
	memset(m_pSpeakerToChannelMap, 0, sizeof(m_pSpeakerToChannelMap));

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

template<class T, class U, int Umin, int Umax>
__forceinline void mix(DWORD mask, int ch, int bps, BYTE* src, BYTE* dst)
{
	U sum = 0;

	for (int i = 0, j = ch; i < j; i++) {
		if (mask & (1<<i)) {
			sum += *(T*)&src[bps*i];
		}
	}

	if (sum < Umin) {
		sum = Umin;
	} else if (sum > Umax) {
		sum = Umax;
	}

	*(T*)dst = (T)sum;
}

template<>
__forceinline void mix<int, INT64, INT24_MIN, INT24_MAX>(DWORD mask, int ch, int bps, BYTE* src, BYTE* dst)
{
	INT64 sum = 0;

	for (int i = 0, j = ch; i < j; i++) {
		if (mask & (1<<i)) {
			int tmp;
			memcpy((BYTE*)&tmp+1, &src[bps*i], 3);
			sum += tmp >> 8;
		}
	}

	sum = min(max(sum, INT24_MIN), INT24_MAX);

	memcpy(dst, (BYTE*)&sum, 3);
}

template<class T, class U, int Umin, int Umax>
__forceinline void mix4(DWORD mask, BYTE* src, BYTE* dst)
{
	U sum = 0;
	int bps = sizeof T;

	if (mask & (1<<0)) {
		sum += *(T*)&src[bps*0];
	}
	if (mask & (1<<1)) {
		sum += *(T*)&src[bps*1];
	}
	if (mask & (1<<2)) {
		sum += *(T*)&src[bps*2];
	}
	if (mask & (1<<3)) {
		sum += *(T*)&src[bps*3];
	}

	if (sum < Umin) {
		sum = Umin;
	} else if (sum > Umax) {
		sum = Umax;
	}

	*(T*)dst = (T)sum;
}

template<class T>
T clamp(double s, T smin, T smax)
{
	if (s < -1) {
		s = -1;
	} else if (s > 1) {
		s = 1;
	}
	T t = (T)(s * smax);
	if (t < smin) {
		t = smin;
	} else if (t > smax) {
		t = smax;
	}
	return t;
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

	if (m_fCustomChannelMapping) {
		size_t channelsCount = m_chs[wfe->nChannels-1].GetCount();
		if (channelsCount > 0 && wfeout->nChannels <= channelsCount) {
			for (int i = 0; i < wfeout->nChannels; i++) {
				DWORD mask = m_chs[wfe->nChannels-1][i].Channel;

				BYTE* src = pDataIn;
				BYTE* dst = &pDataOut[bps*i];

				int srcstep = bps*wfe->nChannels;
				int dststep = bps*wfeout->nChannels;
				int channels = min(AS_MAX_CHANNELS, wfe->nChannels);
				if (fPCM) {
					if (wfe->wBitsPerSample == 8) {
						for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
							mix<unsigned char, INT64, 0, UCHAR_MAX>(mask, channels, bps, src, dst);
						}
					} else if (wfe->wBitsPerSample == 16) {
						if (wfe->nChannels != 4 || wfeout->nChannels != 4) {
							for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
								mix<short, INT64, SHRT_MIN, SHRT_MAX>(mask, channels, bps, src, dst);
							}
						} else { // most popular channels count
							for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
								mix4<short, INT64, SHRT_MIN, SHRT_MAX>(mask, src, dst);
							}
						}
					} else if (wfe->wBitsPerSample == 24) {
						for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
							mix<int, INT64, INT24_MIN, INT24_MAX>(mask, channels, bps, src, dst);
						}
					} else if (wfe->wBitsPerSample == 32) {
						for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
							mix<int, __int64, INT_MIN, INT_MAX>(mask, channels, bps, src, dst);
						}
					}
				} else if (fFloat) {
					if (wfe->wBitsPerSample == 32) {
						for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
							mix<float, double, -1, 1>(mask, channels, bps, src, dst);
						}
					} else if (wfe->wBitsPerSample == 64) {
						for (int k = 0; k < len; k++, src += srcstep, dst += dststep) {
							mix<double, double, -1, 1>(mask, channels, bps, src, dst);
						}
					}
				}
			}
		} else {
			BYTE* pDataOut2 = NULL;
			HRESULT hr2;
			if (FAILED(hr2 = pOut->GetPointer(&pDataOut2)) || !pDataOut2) {
				return hr2;
			}
			memset(pDataOut2, 0, pOut->GetSize());
		}
	} else {
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
			} else if (m_fNormalizeRecover
					   && sample_max * m_normalizeFactor < NORMALIZATION_REGAIN_THRESHOLD) { // we don't regain if we are too close of the maximum
				m_normalizeFactor += NORMALIZATION_REGAIN_STEP * rtDur / 10000000; // the step is per second so we weight it with the duration
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

	if (m_fCustomChannelMapping) {
		m_chs[wfe->nChannels-1].RemoveAll();

		DWORD mask = DWORD((__int64(1)<<wfe->nChannels)-1);
		for (int i = 0; i < AS_MAX_CHANNELS; i++) {
			if (m_pSpeakerToChannelMap[wfe->nChannels-1][i]&mask) {
				ChMap cm = {1<<i, m_pSpeakerToChannelMap[wfe->nChannels-1][i]};
				m_chs[wfe->nChannels-1].Add(cm);
			}
		}

		if (m_chs[wfe->nChannels-1].GetCount() > 0) {
			mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.pbFormat;
			wfex->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
			wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			wfex->Samples.wValidBitsPerSample = wfe->wBitsPerSample;
			wfex->SubFormat =
				wfe->wFormatTag == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM :
				wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT :
				wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? ((WAVEFORMATEXTENSIBLE*)wfe)->SubFormat :
				KSDATAFORMAT_SUBTYPE_PCM; // can't happen

			wfex->dwChannelMask = 0;
			for (size_t i = 0; i < m_chs[wfe->nChannels-1].GetCount(); i++) {
				wfex->dwChannelMask |= m_chs[wfe->nChannels-1][i].Speaker;
			}

			wfex->Format.nChannels = (WORD)m_chs[wfe->nChannels-1].GetCount();
			wfex->Format.nBlockAlign = wfex->Format.nChannels*wfex->Format.wBitsPerSample>>3;
			wfex->Format.nAvgBytesPerSec = wfex->Format.nBlockAlign*wfex->Format.nSamplesPerSec;
		}
	}

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

STDMETHODIMP CAudioSwitcherFilter::GetInputSpeakerConfig(DWORD* pdwChannelMask)
{
	if (!pdwChannelMask) {
		return E_POINTER;
	}

	*pdwChannelMask = 0;

	CStreamSwitcherInputPin* pInPin = GetInputPin();
	if (!pInPin || !pInPin->IsConnected()) {
		return E_UNEXPECTED;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;

	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
		*pdwChannelMask = wfex->dwChannelMask;
	} else {
		*pdwChannelMask = 0/*wfe->nChannels == 1 ? 4 : wfe->nChannels == 2 ? 3 : 0*/;
	}

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetSpeakerConfig(bool* pfCustomChannelMapping, DWORD pSpeakerToChannelMap[AS_MAX_CHANNELS][AS_MAX_CHANNELS])
{
	if (pfCustomChannelMapping) {
		*pfCustomChannelMapping = m_fCustomChannelMapping;
	}
	memcpy(pSpeakerToChannelMap, m_pSpeakerToChannelMap, sizeof(m_pSpeakerToChannelMap));

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetSpeakerConfig(bool fCustomChannelMapping, DWORD pSpeakerToChannelMap[AS_MAX_CHANNELS][AS_MAX_CHANNELS])
{
	if (m_State == State_Stopped || m_fCustomChannelMapping != fCustomChannelMapping
			|| memcmp(m_pSpeakerToChannelMap, pSpeakerToChannelMap, sizeof(m_pSpeakerToChannelMap))) {
		PauseGraph;

		CStreamSwitcherInputPin* pInput = GetInputPin();

		SelectInput(NULL);

		m_fCustomChannelMapping = fCustomChannelMapping;
		memcpy(m_pSpeakerToChannelMap, pSpeakerToChannelMap, sizeof(m_pSpeakerToChannelMap));

		SelectInput(pInput);

		ResumeGraph;
	}

	return S_OK;
}

STDMETHODIMP_(int) CAudioSwitcherFilter::GetNumberOfInputChannels()
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	return pInPin ? ((WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat)->nChannels : 0;
}

STDMETHODIMP_(REFERENCE_TIME) CAudioSwitcherFilter::GetAudioTimeShift()
{
	return m_rtAudioTimeShift;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift)
{
	m_rtAudioTimeShift = rtAudioTimeShift;
	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetNormalizeBoost(bool& fNormalize, bool& fNormalizeRecover, float& boost_dB)
{
	fNormalize = m_fNormalize;
	fNormalizeRecover = m_fNormalizeRecover;
	boost_dB = 20*log10(m_boost_mul);
	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetNormalizeBoost(bool fNormalize, bool fNormalizeRecover, float boost_dB)
{
	if (m_fNormalize != fNormalize) {
		m_normalizeFactor = 4.0;
	}
	m_fNormalize = fNormalize;
	m_fNormalizeRecover = fNormalizeRecover;
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
