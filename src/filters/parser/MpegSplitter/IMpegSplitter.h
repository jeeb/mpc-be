/*
 * (C) 2011-2014 see Authors.txt
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

interface __declspec(uuid("1DC9C085-04AC-4BB8-B2BD-C49A4D30B104"))
IMpegSplitterFilter :
public IUnknown {
	STDMETHOD(Apply()) = 0;

	STDMETHOD(SetForcedSub(BOOL nValue)) = 0;
	STDMETHOD_(BOOL, GetForcedSub()) = 0;

	STDMETHOD(SetAudioLanguageOrder(WCHAR *nValue)) = 0;
	STDMETHOD_(WCHAR *, GetAudioLanguageOrder()) = 0;

	STDMETHOD(SetSubtitlesLanguageOrder(WCHAR *nValue)) = 0;
	STDMETHOD_(WCHAR *, GetSubtitlesLanguageOrder()) = 0;

	STDMETHOD(SetTrueHD(int nValue)) = 0;
	STDMETHOD_(int, GetTrueHD()) = 0;

	STDMETHOD(SetAlternativeDuration(BOOL nValue)) = 0;
	STDMETHOD_(BOOL, GetAlternativeDuration()) = 0;

	STDMETHOD(SetSubEmptyPin(BOOL nValue)) = 0;
	STDMETHOD_(BOOL, GetSubEmptyPin()) = 0;

	STDMETHOD_(int, GetMPEGType()) = 0;
};
