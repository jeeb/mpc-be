/*
 * (C) 2016 see Authors.txt
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
#include "AudioHelper.h"
#include "LowPassFilter.h"

// CLowPassFilter

void CLowPassFilter::SetParams(int shift, int step, int samplerate, int freq)
{
	double Fc = (double)freq / samplerate;
	float X = exp(-2.0 * M_PI * Fc);
	A = 1.0f - X;
	B = X;

	m_shift = shift;
	m_step = step;

	float m_sample = 0.0f;
}

#define LOWPASSPROCESS(fmt) \
void CLowPassFilter::Process_##fmt## (##fmt##_t* p, const int samples) \
{ \
    p += m_shift; \
    for (int i = 0; i < samples; i++) { \
        m_sample = SAMPLE_##fmt##_to_float(*p) * A + m_sample * B; \
        *p = SAMPLE_float_to_##fmt##(m_sample); \
        p += m_step; \
    } \
} \
 

LOWPASSPROCESS(uint8)
LOWPASSPROCESS(int16)
LOWPASSPROCESS(int32)
LOWPASSPROCESS(float)
LOWPASSPROCESS(double)