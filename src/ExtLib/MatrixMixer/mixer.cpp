#include "stdafx.h"
#include <math.h>
#include <memory.h>
#include "mixer.h"

#define value2db(value) ((value > 0)? log10(value)*20.0: 0)
#define db2value(db)    pow(10.0, db/20.0)

double min_gain = 0.1;

Mixer::Mixer()
{
  int ch;

  master = 1.0;
  gain   = 1.0;

  slev   = 1.0;
  clev   = 1.0;
  lfelev = 1.0;

  release = db2value(50);

  drc_on    = false;
  drc_power = 1.0;
  drc_level = 1.0;

  auto_gain = true;
  normalize = false;
  expand_stereo = true;
  voice_control = true;

  memset(&in_levels, 0, sizeof(in_levels));
  memset(&out_levels, 0, sizeof(out_levels));

  for (ch = 0; ch < NCHANNELS; ch++)
  {
    in_gains[ch] = 1.0;
    out_gains[ch] = 1.0;
  }

  in_mode  = Speakers(MODE_STEREO);
  out_mode = Speakers(MODE_STEREO);

  memset(&matrix, 0, sizeof(mixer_matrix_t));
  for (ch = 0; ch < NCHANNELS; ch++)
    matrix[CH_LFE][ch] = 1.0;
  calc_matrix();
}

void 
Mixer::reset()
{
  memset(&in_levels, 0, sizeof(in_levels));
  memset(&out_levels, 0, sizeof(out_levels));
  gain = master;
}

void 
Mixer::mix(sample_buffer_t &samples)
{
  _sample_t buf[NCHANNELS];
  int ch, nsample;

  int in_ch   = in_mode.channels;
  int out_ch  = out_mode.channels;
  int in_nch  = in_mode.nchans();
  int out_nch = out_mode.nchans();
  int in_order[6];
  int out_order[6];

  memcpy(in_order, in_mode.order, sizeof(in_order));
  memcpy(out_order, out_mode.order, sizeof(out_order));

  _sample_t max;
  _sample_t factor;
  _sample_t s;
  _sample_t *sptr;

  _sample_t in_levels_loc[NCHANNELS];
  _sample_t out_levels_loc[NCHANNELS];

  memset(in_levels_loc, 0, sizeof(in_levels_loc));
  memset(out_levels_loc, 0, sizeof(out_levels_loc));

  _sample_t release_factor;
  
  if (release < 1.0)
    release = 1.0;

  release_factor = pow(double(release), double(NSAMPLES) / 48000);

  ///////////////////////////////////////
  // Reorder matrix

  mixer_matrix_t m;
  memset(&m, 0, sizeof(m));
  int ch1, ch2;
  for (ch1 = 0; ch1 < in_nch; ch1++)
    for (ch2 = 0; ch2 < out_nch; ch2++)
      m[ch1][ch2] = matrix[in_order[ch1]][out_order[ch2]] * in_gains[in_order[ch1]] * out_gains[out_order[ch2]];

  ///////////////////////////////////////
  // Adjust gain

  ///////////////////////////////////////
  // Adjust gain

  if (auto_gain && !normalize)
    if (gain * release_factor > master)
      gain = master;
    else
      gain *= release_factor;

  ///////////////////////////////////////
  // Input levels

  // optimize: expand channels cycle
  for (ch = 0; ch < in_nch; ch++)
  {
    max = 0;
    sptr = samples[ch];
    for (nsample = 0; nsample < NSAMPLES2/8; nsample++)
    {
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
    }
    in_levels_loc[ch] = max;
  }

  max = in_levels_loc[0];
  for (ch = 1; ch < in_nch; ch++)
    if (max < in_levels_loc[ch]) max = in_levels_loc[ch];

  ///////////////////////////////////////
  // DRC

  // drc_power means level increase at -50dB
  if (drc_on)
    if (drc_level * release_factor > pow(max/in_mode.level, -log10(drc_power)*20/50))
      drc_level = pow(max/in_mode.level, -log10(drc_power)*20/50);
    else
      drc_level *= release_factor;
  else
    drc_level = 1.0;

  ///////////////////////////////////////
  // Mix samples

  // optimize: do not multiply by nulls (most of matrix)
  factor = gain * drc_level * out_mode.level / in_mode.level;
  memset(buf, 0, sizeof(buf));
  for (nsample = 0; nsample < NSAMPLES2; nsample++)
  {
    for (ch = 0; ch < out_nch; ch++)
    {
      buf[ch] = 0;
      for (ch2 = 0; ch2 < in_nch; ch2++)
        buf[ch] += samples[ch2][nsample] * m[ch2][ch];
    }
    for (ch = 0; ch < out_nch; ch++)
      samples[ch][nsample] = buf[ch] * factor;
  }

  ///////////////////////////////////////
  // Output levels

  memset(out_levels_loc, 0, sizeof(out_levels_loc));
  for (ch = 0; ch < out_nch; ch++)
  {
    max = 0;
    sptr = samples[ch];
    for (nsample = 0; nsample < NSAMPLES2/8; nsample++)
    {
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
      if (fabs(*sptr) > max) max = fabs(*sptr); sptr++;
    }
    out_levels_loc[ch] = max;
  }

  max = out_levels_loc[0];
  for (ch = 1; ch < out_nch; ch++)
    if (max < out_levels_loc[ch]) max = out_levels_loc[ch];

  ///////////////////////////////////////
  // Auto Gain Control

  if (auto_gain)
  {
    if (max > out_mode.level)
    {
      factor = out_mode.level / max;
      gain *= factor;
      if (gain < min_gain)
      {
        factor *= min_gain / gain;
        gain = min_gain;
      }

      max *= factor;

      sptr = (float *)samples;
      nsample = out_nch * NSAMPLES2 / 8;

      while (nsample--)
      {
        *sptr++ *= factor; *sptr++ *= factor;
        *sptr++ *= factor; *sptr++ *= factor;
        *sptr++ *= factor; *sptr++ *= factor;
        *sptr++ *= factor; *sptr++ *= factor;
      }
    }
  }

  ///////////////////////////////////////
  // Clip
  // should be rare

  if (max > out_mode.level)
  {
    sptr = (_sample_t *)samples;
    nsample = out_nch * NSAMPLES2;

    while (nsample--)
    {
      s = *sptr;
      if (s > +out_mode.level) *sptr = +out_mode.level;
      if (s < -out_mode.level) *sptr = -out_mode.level;
      sptr++;
    }
  }

  ///////////////////////////////////////
  // Normalize output levels

  for (ch = 0; ch < NCHANNELS; ch++)
  {
    if (CH_MASK(in_order[ch]) & in_ch)
    {
      in_levels_loc[ch] /= in_mode.level;
      in_levels_loc[ch] *= in_gains[in_order[ch]];
      if (in_levels_loc[ch] > in_levels[in_order[ch]]) 
        in_levels[in_order[ch]] = in_levels_loc[ch];
    }

    if (CH_MASK(out_order[ch]) & out_ch)
    {
      out_levels_loc[ch] /= out_mode.level;
      if (out_levels_loc[ch] > out_levels[out_order[ch]]) 
        out_levels[out_order[ch]] = out_levels_loc[ch];
    }
  }
}

void 
Mixer::calc_matrix(bool normalize)
{
  int in_ch      = in_mode.channels;
  int out_ch     = out_mode.channels;
  int in_nfront  = in_mode.nfront();
  int in_nrear   = in_mode.nrear();
  int out_nfront = out_mode.nfront();
  int out_nrear  = out_mode.nrear();

  for (int i = 0; i < NCHANNELS-1; i++)
    for (int j = 0; j < NCHANNELS; j++)
      matrix[i][j] = 0;
//  memset(&matrix, 0, sizeof(mixer_matrix_t));



  // Dolby modes are backwards-compatible
  if (in_mode.dolby & out_mode.dolby)
  {
    matrix[CH_L][CH_L] = 1;
    matrix[CH_R][CH_R] = 1;
  }
  // Downmix to Dolby Surround/ProLogic/ProLogicII
  else if (out_mode.dolby)
  {
    if (in_nfront >= 2)
    {
      matrix[CH_L][CH_L] = 1;
      matrix[CH_R][CH_R] = 1;
    }

    if (in_nfront != 2)
    {
      matrix[CH_C][CH_L] = 0.7071 * clev;
      matrix[CH_C][CH_R] = 0.7071 * clev;
    }
    if (in_nrear == 1)
    {
      matrix[CH_S][CH_L] = -0.7071 * slev;
      matrix[CH_S][CH_R] = +0.7071 * slev;
    }
    else if (in_nrear == 2)
    {
      switch (out_mode.dolby)
      { 
      case DOLBY_PROLOGICII:
        matrix[CH_SL][CH_L] = -0.8660*slev;
        matrix[CH_SR][CH_L] = -0.5000*slev;
        matrix[CH_SL][CH_R] = +0.5000*slev;
        matrix[CH_SR][CH_R] = +0.8660*slev;
        break;

      case DOLBY_SURROUND:
      case DOLBY_PROLOGIC:
      default:
        matrix[CH_SL][CH_L] = -slev;
        matrix[CH_SR][CH_L] = -slev;
        matrix[CH_SL][CH_R] = +slev;
        matrix[CH_SR][CH_R] = +slev;
        break;
      }
    }
  }
  else
  // A/52 standart mixes
  {
    // direct route equal channels
    if (in_ch & out_ch & CH_MASK_L)   matrix[CH_L]  [CH_L]   = 1.0;
    if (in_ch & out_ch & CH_MASK_R)   matrix[CH_R]  [CH_R]   = 1.0;
    if (in_ch & out_ch & CH_MASK_C)   matrix[CH_C]  [CH_C]   = clev;
    if (in_ch & out_ch & CH_MASK_SL)  matrix[CH_SL] [CH_SL]  = slev;
    if (in_ch & out_ch & CH_MASK_SR)  matrix[CH_SR] [CH_SR]  = slev;

    // calc matrix for fbw channels
    if (out_nfront == 1)
    {
      if (in_nfront != 1)
      {
        matrix[CH_L][CH_M] = LEVEL_3DB;
        matrix[CH_R][CH_M] = LEVEL_3DB;
      }
      if (in_nfront == 3)
      {
        matrix[CH_C][CH_M] = clev * LEVEL_PLUS3DB;
      }
      if (in_nrear == 1)
      {
        matrix[CH_S][CH_M] = slev * LEVEL_3DB;
      }
      else
      {
        matrix[CH_SL][CH_M] = slev * LEVEL_3DB;
        matrix[CH_SR][CH_M] = slev * LEVEL_3DB;
      }
    }
    else // not mono modes
    {
      if (out_nfront == 2)
      {
        if (in_nfront == 1)
        {
          matrix[CH_M][CH_L] = LEVEL_3DB;
          matrix[CH_M][CH_R] = LEVEL_3DB;
        }
        else if (in_nfront == 3)
        {
          matrix[CH_C][CH_L] = clev;
          matrix[CH_C][CH_R] = clev;
        }
      }
      if (in_nrear == 1)
      {
        if (out_nrear == 0)
        {
          matrix[CH_S][CH_L] = slev * LEVEL_3DB;
          matrix[CH_S][CH_R] = slev * LEVEL_3DB;
        }
        else if (out_nrear == 2)
        {
          matrix[CH_S][CH_SL] = slev * LEVEL_3DB;
          matrix[CH_S][CH_SR] = slev * LEVEL_3DB;
        }
      }
      else if (in_nrear == 2)
      {
        if (out_nrear == 0)
        {
          matrix[CH_SL][CH_L] = slev;
          matrix[CH_SR][CH_R] = slev;
        }         
        else if (out_nrear == 1)
        {
          matrix[CH_SL][CH_S] = slev * LEVEL_3DB;
          matrix[CH_SR][CH_S] = slev * LEVEL_3DB;
        }
      }
    }
  }

  // Expand stereo & Voice control
  bool expand_stereo_allowed = expand_stereo && !in_nrear;
  bool voice_control_allowed = voice_control && (in_nfront == 2);

  if ((voice_control_allowed || expand_stereo_allowed) && !out_mode.dolby)
  {
    if (voice_control_allowed && out_nfront != 2)
    {
      // C' = clev * (L + R) * LEVEL_3DB
      matrix[CH_L][CH_M] = clev * LEVEL_3DB;
      matrix[CH_R][CH_M] = clev * LEVEL_3DB;
    }

    if (expand_stereo_allowed && in_nfront == 2 && out_nrear)
    {
      if (out_nrear == 1)
      {
        // S' = slev * (L - R)
        matrix[CH_L][CH_S] = + slev;
        matrix[CH_R][CH_S] = - slev;
      }
      if (out_nrear == 2)
      {
        // SL' = slev * 1/2 (L - R)
        // SR' = slev * 1/2 (R - L)
        matrix[CH_L][CH_SL] = + 0.5 * slev;
        matrix[CH_R][CH_SL] = - 0.5 * slev;
        matrix[CH_L][CH_SR] = - 0.5 * slev;
        matrix[CH_R][CH_SR] = + 0.5 * slev;
      }
    }

    if (in_nfront != 1)
    {
      if (expand_stereo_allowed && voice_control_allowed)
      {
        // L' = L * 1/2 (slev + clev) - R * 1/2 (slev - clev)
        // R' = R * 1/2 (slev + clev) - L * 1/2 (slev - clev)
        matrix[CH_L][CH_L] = + 0.5 * (slev + clev);
        matrix[CH_R][CH_L] = - 0.5 * (slev - clev);
        matrix[CH_L][CH_R] = - 0.5 * (slev - clev);
        matrix[CH_R][CH_R] = + 0.5 * (slev + clev);
      }
      else if (expand_stereo_allowed)
      {
        matrix[CH_L][CH_L] = + 0.5 * (slev + 1);
        matrix[CH_R][CH_L] = - 0.5 * (slev - 1);
        matrix[CH_L][CH_R] = - 0.5 * (slev - 1);
        matrix[CH_R][CH_R] = + 0.5 * (slev + 1);
      }
      else // if (voice_control_allowed)
      {
        matrix[CH_L][CH_L] = + 0.5 * (1 + clev);
        matrix[CH_R][CH_L] = - 0.5 * (1 - clev);
        matrix[CH_L][CH_R] = - 0.5 * (1 - clev);
        matrix[CH_R][CH_R] = + 0.5 * (1 + clev);
      }
    }
  }

  // LFE mixing
  if (in_ch & out_ch & CH_MASK_LFE) 
    matrix[CH_LFE][CH_LFE] = lfelev;

/*
  // mix LFE into front channels if exists in input
  // and absent in output
  if (in_ch & ~out_ch & CH_MASK_LFE)
  {
    if (out_nfront > 1)
    {
      matrix[CH_LFE][CH_L]  = lfelev;
      matrix[CH_LFE][CH_R]  = lfelev;
    }
    else
      matrix[CH_LFE][CH_C]  = lfelev;
  }
*/

  if (normalize)
    normalize_matrix();
}

/*
void 
Mixer::calc_matrix(bool _normalize)
{
  int in_ch      = in_mode.channels;
  int out_ch     = out_mode.channels;
  int in_nfront  = in_mode.nfront();
  int in_nrear   = in_mode.nrear();
  int out_nfront = out_mode.nfront();
  int out_nrear  = out_mode.nrear();

  memset(&matrix, 0, sizeof(mixer_matrix_t));

  // Dolby modes are backwards-compatible
  if (in_mode.dolby & out_mode.dolby)
  {
    matrix[CH_L][CH_L] = 1;
    matrix[CH_R][CH_R] = 1;
  }
  // Downmix to Dolby Surround/ProLogic/ProLogicII
  else if (out_mode.dolby)
  {
    if (in_nfront >= 2)
    {
      matrix[CH_L][CH_L] = 1;
      matrix[CH_R][CH_R] = 1;
    }

    if (in_nfront != 2)
    {
      matrix[CH_C][CH_L] = clev;
      matrix[CH_C][CH_R] = clev;
    }
    if (in_nrear == 1)
    {
      matrix[CH_S][CH_L] = -slev;
      matrix[CH_S][CH_R] = +slev;
    }
    else if (in_nrear == 2)
    {
      switch (out_mode.dolby)
      { 
      case DOLBY_PROLOGICII:
        matrix[CH_SL][CH_L] = -0.8165*slev;
        matrix[CH_SR][CH_L] = -0.5774*slev;
        matrix[CH_SL][CH_R] = +0.5774*slev;
        matrix[CH_SR][CH_R] = +0.8165*slev;
        break;

      case DOLBY_SURROUND:
      case DOLBY_PROLOGIC:
      default:
        matrix[CH_SL][CH_L] = -slev;
        matrix[CH_SR][CH_L] = -slev;
        matrix[CH_SL][CH_R] = +slev;
        matrix[CH_SR][CH_R] = +slev;
        break;
      }
    }
  }
  else
  // A/52 standart mixes
  {
    // direct route equal channels
    if (in_ch & out_ch & CH_MASK_L)   matrix[CH_L]  [CH_L]   = 1.0;
    if (in_ch & out_ch & CH_MASK_R)   matrix[CH_R]  [CH_R]   = 1.0;
    if (in_ch & out_ch & CH_MASK_C)   matrix[CH_C]  [CH_C]   = clev;
    if (in_ch & out_ch & CH_MASK_SL)  matrix[CH_SL] [CH_SL]  = slev;
    if (in_ch & out_ch & CH_MASK_SR)  matrix[CH_SR] [CH_SR]  = slev;
    if (in_ch & out_ch & CH_MASK_LFE) matrix[CH_LFE][CH_LFE] = lfelev;

    // calc matrix for fbw channels
    if (out_nfront == 1)
    {
      if (in_nfront != 1)
      {
        matrix[CH_L][CH_M] = LEVEL_3DB;
        matrix[CH_R][CH_M] = LEVEL_3DB;
      }
      if (in_nfront == 3)
      {
        matrix[CH_C][CH_M] = clev * LEVEL_PLUS3DB;
      }
      if (in_nrear == 1)
      {
        matrix[CH_S][CH_M] = slev * LEVEL_3DB;
      }
      else
      {
        matrix[CH_SL][CH_M] = slev * LEVEL_3DB;
        matrix[CH_SR][CH_M] = slev * LEVEL_3DB;
      }
    }
    else // not mono modes
    {
      if (out_nfront == 2)
      {
        if (in_nfront == 1)
        {
          matrix[CH_M][CH_L] = LEVEL_3DB;
          matrix[CH_M][CH_R] = LEVEL_3DB;
        }
        else if (in_nfront == 3)
        {
          matrix[CH_C][CH_L] = clev;
          matrix[CH_C][CH_R] = clev;
        }
      }
      if (in_nrear == 1)
      {
        if (out_nrear == 0)
        {
          matrix[CH_S][CH_L] = slev * LEVEL_3DB;
          matrix[CH_S][CH_R] = slev * LEVEL_3DB;
        }
        else if (out_nrear == 2)
        {
          matrix[CH_S][CH_SL] = slev * LEVEL_3DB;
          matrix[CH_S][CH_SR] = slev * LEVEL_3DB;
        }
      }
      else if (in_nrear == 2)
      {
        if (out_nrear == 0)
        {
          matrix[CH_SL][CH_L] = slev;
          matrix[CH_SR][CH_R] = slev;
        }         
        else if (out_nrear == 1)
        {
          matrix[CH_SL][CH_S] = slev * LEVEL_3DB;
          matrix[CH_SR][CH_S] = slev * LEVEL_3DB;
        }
      }
    }
  }

  // Expand stereo & Voice control
  bool expand_stereo_allowed = expand_stereo && !in_nrear;
  bool voice_control_allowed = voice_control && (in_nfront == 2);

  if ((voice_control_allowed || expand_stereo_allowed) && !out_mode.dolby)
  {
    if (voice_control_allowed && out_nfront != 2)
    {
      // C' = clev * (L + R) * LEVEL_3DB
      matrix[CH_L][CH_M] = clev * LEVEL_3DB;
      matrix[CH_R][CH_M] = clev * LEVEL_3DB;
    }

    if (expand_stereo_allowed && in_nfront == 2 && out_nrear)
    {
      if (out_nrear == 1)
      {
        // S' = slev * (L - R)
        matrix[CH_L][CH_S] = + slev;
        matrix[CH_R][CH_S] = - slev;
      }
      if (out_nrear == 2)
      {
        // SL' = slev * 1/2 (L - R)
        // SR' = slev * 1/2 (R - L)
        matrix[CH_L][CH_SL] = + 0.5 * slev;
        matrix[CH_R][CH_SL] = - 0.5 * slev;
        matrix[CH_L][CH_SR] = - 0.5 * slev;
        matrix[CH_R][CH_SR] = + 0.5 * slev;
      }
    }

    if (in_nfront != 1 && out_nfront != 1)
    {
      if (expand_stereo_allowed && voice_control_allowed)
      {
        // L' = L * 1/2 (slev + clev) - R * 1/2 (slev - clev)
        // R' = R * 1/2 (slev + clev) - L * 1/2 (slev - clev)
        matrix[CH_L][CH_L] = + 0.5 * (slev + clev);
        matrix[CH_R][CH_L] = - 0.5 * (slev - clev);
        matrix[CH_L][CH_R] = - 0.5 * (slev - clev);
        matrix[CH_R][CH_R] = + 0.5 * (slev + clev);
      }
      else if (expand_stereo_allowed)
      {
        matrix[CH_L][CH_L] = + 0.5 * (slev + 1);
        matrix[CH_R][CH_L] = - 0.5 * (slev - 1);
        matrix[CH_L][CH_R] = - 0.5 * (slev - 1);
        matrix[CH_R][CH_R] = + 0.5 * (slev + 1);
      }
      else // if (voice_control_allowed)
      {
        matrix[CH_L][CH_L] = + 0.5 * (1 + clev);
        matrix[CH_R][CH_L] = - 0.5 * (1 - clev);
        matrix[CH_L][CH_R] = - 0.5 * (1 - clev);
        matrix[CH_R][CH_R] = + 0.5 * (1 + clev);
      }
    }
  }

  // mix LFE into front channels if exists in input & not in output
  if (in_ch & ~out_ch & CH_MASK_LFE)
  {
    matrix[CH_LFE][CH_L]  = lfelev;
    matrix[CH_LFE][CH_R]  = lfelev;
    matrix[CH_LFE][CH_C]  = lfelev;
    matrix[CH_LFE][CH_SR] = 0;
    matrix[CH_LFE][CH_SL] = 0;
  }
  if (~in_ch & out_ch & CH_MASK_LFE)
  {
    matrix[CH_L][CH_LFE]  = 1.0;
    matrix[CH_C][CH_LFE]  = 1.0;
    matrix[CH_R][CH_LFE]  = 1.0;
    matrix[CH_SL][CH_LFE] = 1.0;
    matrix[CH_SR][CH_LFE] = 1.0;
  }

  if (_normalize)
    normalize_matrix();
}
*/
void 
Mixer::normalize_matrix()
{
  double levels[NCHANNELS] = { 0, 0, 0, 0, 0, 0 };
  double max_level;
  double norm;
  int i, j;

  for (i = 0; i < NCHANNELS; i++)
    for (j = 0; j < NCHANNELS; j++)
      levels[i] += matrix[j][i];

  max_level = levels[0];
  for (i = 1; i < NCHANNELS; i++)
    if (levels[i] > max_level) 
      max_level = levels[i];

  if (max_level > 0)
    norm = 1.0/max_level;
  else
    norm = 1.0;

  for (i = 0; i < NCHANNELS; i++)
    for (j = 0; j < NCHANNELS; j++)
      matrix[j][i] *= norm;
}
