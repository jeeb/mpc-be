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

#pragma once

#include "StreamSwitcher.h"
#include "AudioNormalizer.h"

#define AudioSwitcherName L"MPC AudioSwitcher"

interface __declspec(uuid("CEDB2890-53AE-4231-91A3-B0AAFCD1DBDE"))
IAudioSwitcherFilter :
public IUnknown {
	STDMETHOD_(REFERENCE_TIME, GetAudioTimeShift) () PURE;
	STDMETHOD(SetAudioTimeShift) (REFERENCE_TIME rtAudioTimeShift) PURE;
	STDMETHOD(GetAutoVolumeControl) (bool& bAutoVolumeControl, bool& bNormBoost, int& iNormGain, int& iNormRealeaseTime) PURE;
	STDMETHOD(SetAutoVolumeControl) (bool bAutoVolumeControl, bool bNormBoost, int iNormGain, int iNormRealeaseTime) PURE;
	STDMETHOD(SetAudioGain) (float fGain_dB) PURE;
};

class __declspec(uuid("18C16B08-6497-420e-AD14-22D21C2CEAB7"))
	CAudioSwitcherFilter : public CStreamSwitcherFilter, public IAudioSwitcherFilter
{
	CAudioNormalizer m_AudioNormalizer;
	bool	m_bAutoVolumeControl;
	bool	m_bNormBoost;
	int		m_iNormGain;
	int		m_iNormRealeaseTime;
	float*	m_buffer;
	size_t	m_buf_size;

	float	m_fGain_dB;
	float	m_fGainFactor;

	REFERENCE_TIME m_rtAudioTimeShift;

	REFERENCE_TIME m_rtNextStart;

public:
	CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	CMediaType CreateNewOutputMediaType(CMediaType mt, long& cbBuffer);
	void OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut);

	HRESULT DeliverEndFlush();
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	// IAudioSwitcherFilter
	STDMETHODIMP_(REFERENCE_TIME) GetAudioTimeShift();
	STDMETHODIMP SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift);
	STDMETHODIMP GetAutoVolumeControl(bool& bAutoVolumeControl, bool& bNormBoost, int& iNormGain, int& iNormRealeaseTime);
	STDMETHODIMP SetAutoVolumeControl(bool bAutoVolumeControl, bool bNormBoost, int iNormGain, int iNormRealeaseTime);
	STDMETHODIMP SetAudioGain(float fGain_dB);

	// IAMStreamSelect
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
};
