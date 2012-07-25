#include "stdafx.h"
#include "spk.h"
#include <ks.h>
#include <ksmedia.h>

bool 
Speakers::get_wfx(WAVEFORMATEX *wfx, int sample_rate, bool use_extensible) const
{
  if (spdif)
  {
    // SPDIF format
    wfx->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfx->nChannels = 2;
    wfx->nSamplesPerSec = sample_rate;
    wfx->wBitsPerSample = 16;
    wfx->nBlockAlign = 4;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    wfx->cbSize = 0;
    return true;
  }

  WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE *)wfx;
  use_extensible &= !dolby && mode != MODE_STEREO && mode != MODE_MONO;

  if (use_extensible)
  {
    memset(wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
  }
  else
    memset(wfx, 0, sizeof(WAVEFORMATEX));

  int nch = nchans();

  switch (fmt)
  {
    case FORMAT_PCM16:
      wfx->wFormatTag = WAVE_FORMAT_PCM;
      wfx->nChannels = nch;
      wfx->nSamplesPerSec = sample_rate;
      wfx->wBitsPerSample = 16;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;

      if (use_extensible)
      {
        ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 16;
        ext->dwChannelMask = _ds_channels[mode & 15];
      }
      break;

    case FORMAT_PCM24:
      wfx->wFormatTag = WAVE_FORMAT_PCM;
      wfx->nChannels = nch;
      wfx->nSamplesPerSec = sample_rate;
      wfx->wBitsPerSample = 24;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;

      if (use_extensible)
      {
        ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 24;
        ext->dwChannelMask = _ds_channels[mode & 15];
      }
      break;

    case FORMAT_PCM32:
      wfx->wFormatTag = WAVE_FORMAT_PCM;
      wfx->nChannels = nch;
      wfx->nSamplesPerSec = sample_rate;
      wfx->wBitsPerSample = 32;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;

      if (use_extensible)
      {
        ext->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        ext->Samples.wValidBitsPerSample = 32;
        ext->dwChannelMask = _ds_channels[mode & 15];
      }
      break;

    case FORMAT_FLOAT:
      wfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
      wfx->nChannels = nch;
      wfx->nSamplesPerSec = sample_rate;
      wfx->wBitsPerSample = 32;
      wfx->nBlockAlign = wfx->wBitsPerSample / 8 * wfx->nChannels;
      wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;

      if (use_extensible)
      {
        ext->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        ext->Samples.wValidBitsPerSample = 32;
        ext->dwChannelMask = _ds_channels[mode & 15];
      }
      break;

    default:
      // unknown format
      return false;
  }

  if (use_extensible)
  {
    wfx->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx->cbSize = 22;
  }

  return true;
};

bool 
Speakers::set_wfx(WAVEFORMATEX *wfx)
{
  int _mode, _fmt;
  WAVEFORMATEXTENSIBLE *wfex = 0;
  if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
  {
    // extensible
    wfex = (WAVEFORMATEXTENSIBLE *)wfx;

    // determine sample format
    if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
      _fmt = FORMAT_FLOAT;
    else if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
      switch (wfx->wBitsPerSample)
      {
        case 16: _fmt = FORMAT_PCM16; break;
        case 24: _fmt = FORMAT_PCM24; break;
        case 32: _fmt = FORMAT_PCM32; break;
        default: return false;
      }
    else
      return false;

    // determine audio mode
    for (_mode = 0; _mode < sizeof(_ds_channels) / sizeof(_ds_channels[0]); _mode++)
      if (_ds_channels[_mode] == wfex->dwChannelMask)
        break;

    if (_mode == sizeof(_ds_channels) / sizeof(_ds_channels[0]))
      return false;

  }
  else
  {
    // determine sample format
    if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
      _fmt = FORMAT_FLOAT;
    else if (wfx->wFormatTag == WAVE_FORMAT_PCM)
      switch (wfx->wBitsPerSample)
      {
        case 16: _fmt = FORMAT_PCM16; break;
        case 24: _fmt = FORMAT_PCM24; break;
        case 32: _fmt = FORMAT_PCM32; break;
        default: return false;
      }
    else
      return false;

    // determine audio mode
    switch (wfx->nChannels)
    {
      case 1: _mode = MODE_MONO;   break;
      case 2: _mode = MODE_STEREO; break;
      case 3: _mode = MODE_3_0;    break;
      case 4: _mode = MODE_QUADRO; break;
      case 5: _mode = MODE_3_2;    break;
      case 6: _mode = MODE_5_1;    break;
      default: return false;
    }
  }

  set(_mode, NO_DOLBY, _fmt, NO_SPDIF);
  return true;
}

int 
Speakers::samples_size(int nsamples)
{
  switch (fmt)
  {
    case FORMAT_PCM16: return nsamples * nchans() * sizeof(int16_t);
    case FORMAT_PCM24: return nsamples * nchans() * sizeof(int24_t);
    case FORMAT_PCM32: return nsamples * nchans() * sizeof(int32_t);
    case FORMAT_FLOAT: return nsamples * nchans() * sizeof(float);
    default: return 0; // note: should not ever be here
  }
}