#ifndef VAC3DEC_DEFS_H
#define VAC3DEC_DEFS_H

#include <windows.h>
#include <mmreg.h>
#include <memory.h>

// base word types
typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed int      int32_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
#pragma pack(push, 1)   // do not justify following structure
struct int24_t {
  uint16_t low; 
  int8_t   high;
  int24_t(int32_t i)
  {
    low  = uint16_t(i & 0xFFFF);
    high = int8_t(i >> 16);
  }
  operator int32_t()
  {
    return (high << 16) + low;
  }
};
#pragma pack(pop)


#define SPDIF_HEADER_SIZE  8
#define SPDIF_FRAME_SIZE   0x1800
#define MAX_AC3_FRAME_SIZE 3840

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#endif

// level multipliers
#define LEVEL_PLUS6DB 2.0
#define LEVEL_PLUS3DB 1.4142135623730951
#define LEVEL_3DB     0.7071067811865476
#define LEVEL_45DB    0.5946035575013605
#define LEVEL_6DB     0.5  

// basic constants
#define NCHANNELS    6
#define NSAMPLES     512
#define NSAMPLES2    1024

// audio sample
typedef float _sample_t; // conflict with a52 library ...

#pragma warning(disable: 4002 4244 4305) // do not warn about double to float conversion
 
// block of audio samples
typedef _sample_t sample_buffer_t[NCHANNELS][NSAMPLES2];
// mixing matrix 6-to-6 channels
typedef _sample_t mixer_matrix_t[NCHANNELS][NCHANNELS];
// channel order definition
typedef int channel_order_t[6];

// audio coding modes
// equivalent to A52/A standart modes except for DPL, DPLII and SPDIF modes
#define MODE_DUAL    0
#define MODE_1_0     1
#define MODE_2_0     2
#define MODE_3_0     3
#define MODE_2_1     4
#define MODE_3_1     5
#define MODE_2_2     6
#define MODE_3_2     7
#define MODE_LFE     8  // can be combined with any of previous modes
#define MODE_ERROR   16

#define MODE_MONO    1
#define MODE_STEREO  2
#define MODE_QUADRO  6
#define MODE_5_1     (MODE_3_2 | MODE_LFE)

// Dolby encoding mode
#define NO_DOLBY          0  // Stream is not Dolby-encoded
#define DOLBY_SURROUND    1  // Dolby Surround
#define DOLBY_PROLOGIC    2  // Dolby Pro Logic
#define DOLBY_PROLOGICII  3  // Dolby Pro Logic II
#define DOLBY_LFE    	    4  // LFE presence flag

// SPDIF mode
#define NO_SPDIF     0  // No SPDIF
#define SPDIF_AUTO   1  // Auto SPDIF configuration (=SPDIF_MODE1 by now)

// Sample formats
#define FORMAT_PCM16 0  // PCM 16bit (default)
#define FORMAT_PCM24 1  // PCM 24bit
#define FORMAT_PCM32 2  // PCM 32bit
#define FORMAT_FLOAT 3  // PCM Float

// channel numbers
// used as index in arrays
#define CH_L         0  // Left channel
#define CH_C         1  // Center channel
#define CH_R         2  // Right channel
#define CH_SL        3  // Surround left channel
#define CH_SR        4  // Surround right channel
#define CH_LFE       5  // LFE channel
#define CH_NONE      6  // indicates that channel is not used in channel order

#define CH_M         1  // Mono channel = center channel
#define CH_CH1       0  // Channel 1 in Dual mono mode
#define CH_CH2       2  // Channel 2 in Dual mono mode
#define CH_S         3  // Surround channel for x/1 modes

// channel masks
// used as channel presence flag
#define CH_MASK_L    1
#define CH_MASK_C    2
#define CH_MASK_R    4
#define CH_MASK_SL   8
#define CH_MASK_SR   16
#define CH_MASK_LFE  32
#define CH_MASK_LAST 32

#define CH_MASK_M    2
#define CH_MASK_C1   0
#define CH_MASK_C2   4
#define CH_MASK_S    8
#define CH_MASK(ch)  (1 << (ch))



// ac3 channel masks
static const int _mode_ch[16] =
{
  CH_MASK_C1 | CH_MASK_C2,
  CH_MASK_M,
  CH_MASK_L  | CH_MASK_R,
  CH_MASK_L  | CH_MASK_C   | CH_MASK_R,
  CH_MASK_L  | CH_MASK_R   | CH_MASK_S,
  CH_MASK_L  | CH_MASK_C   | CH_MASK_R   | CH_MASK_S,
  CH_MASK_L  | CH_MASK_R   | CH_MASK_SL  | CH_MASK_SR,
  CH_MASK_L  | CH_MASK_C   | CH_MASK_R   | CH_MASK_SL | CH_MASK_SR,

  CH_MASK_C1 | CH_MASK_C2  | CH_MASK_LFE,
  CH_MASK_M  | CH_MASK_LFE,
  CH_MASK_L  | CH_MASK_R   | CH_MASK_LFE,
  CH_MASK_L  | CH_MASK_C   | CH_MASK_R   | CH_MASK_LFE,
  CH_MASK_L  | CH_MASK_R   | CH_MASK_S   | CH_MASK_LFE,
  CH_MASK_L  | CH_MASK_C   | CH_MASK_R   | CH_MASK_S   | CH_MASK_LFE,
  CH_MASK_L  | CH_MASK_R   | CH_MASK_SL  | CH_MASK_SR  | CH_MASK_LFE,
  CH_MASK_L  | CH_MASK_C   | CH_MASK_R   | CH_MASK_SL  | CH_MASK_SR | CH_MASK_LFE,
};

// ac3 channel order
static const int _ch_order[16][6] = 
{
  { CH_CH1, CH_CH2,   CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_M,   CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_C,     CH_R,     CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_S,     CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_C,     CH_R,     CH_S,     CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_SL,    CH_SR,    CH_NONE,  CH_NONE },
  { CH_L,   CH_C,     CH_R,     CH_SL,    CH_SR,    CH_NONE },
  { CH_CH1, CH_CH2,   CH_LFE,   CH_NONE,  CH_NONE,  CH_NONE },
  { CH_M,   CH_LFE,   CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_LFE,   CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_C,     CH_R,     CH_LFE,   CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_S,     CH_LFE,   CH_NONE,  CH_NONE },
  { CH_L,   CH_C,     CH_R,     CH_S,     CH_LFE,   CH_NONE },
  { CH_L,   CH_R,     CH_SL,    CH_SR,    CH_LFE,   CH_NONE },
  { CH_L,   CH_C,     CH_R,     CH_SL,    CH_SR,    CH_LFE  }
};

static const int _ds_channels[16] = 
{
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT,  // double mono as stereo
  SPEAKER_FRONT_CENTER,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_CENTER,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_CENTER,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT,

  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,  // double mono as stereo
  SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_CENTER  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_CENTER  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT   | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_LOW_FREQUENCY
};

static const int _ds_order[16][6] = 
{
  { CH_L,   CH_R,     CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_M,   CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_C,     CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_S,     CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_C,     CH_S,     CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_SL,    CH_SR,    CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_C,     CH_SL,    CH_SR,    CH_NONE },

  { CH_L,   CH_R,     CH_LFE,   CH_NONE,  CH_NONE,  CH_NONE },
  { CH_M,   CH_LFE,   CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_LFE,   CH_NONE,  CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_C,     CH_LFE,   CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_LFE,   CH_S,     CH_NONE,  CH_NONE },
  { CH_L,   CH_R,     CH_C,     CH_LFE,   CH_S,     CH_NONE },
  { CH_L,   CH_R,     CH_LFE,   CH_SL,    CH_SR,    CH_NONE },
  { CH_L,   CH_R,     CH_C,     CH_LFE,   CH_SL,    CH_SR   }
};



static const char *_mode_names[] =
{
  "Dual mono",
  "1/0 - mono",
  "2/0 - stereo",
  "3/0 - 3 front",
  "2/1 - surround",
  "3/1 - surround",
  "2/2 - quadro",
  "3/2 - 5 channels",
  "Dual mono + LFE",
  "1/0+LFE 1.1 mono",
  "2/0+LFE 2.1 stereo",
  "3/0+LFE 3.1 front",
  "2/1+LFE 3.1 surround",
  "3/1+LFE 4.1 surround",
  "2/2+LFE 4.1 quadro",
  "3/2+LFE 5.1 channels"
};

static const _sample_t _levels_tbl[4] = { 32767.0, 8388607.0, 2147483647.0, 1.0 };



inline int count_channels(int channels);

class Speakers
{
public:
  // audio coding mode, used to specify audio format
  // MODE_XXXX constants and may be flagged with MODE_LFE
  int mode;

  // Dolby-encoded format type
  // one of DOLBY_XXXX constants
  // mode is ignored if non-zero
  int dolby;

  // Sample format
  // SAMPLE_XXXX constants
  int fmt;

  // SPDIF mode
  // SPDIF_XXXX constants
  // mode, dolby and fmt are ignored if non-zero
  int spdif;

  // channels definition, used to define channel configuration
  // composed of number of CH_MASK_X masks
  int channels;

  // channel order definition
  // array of channel names CH_XXX
  int order[NCHANNELS];

  // 0dB level
  _sample_t level;

  Speakers(int _mode = MODE_STEREO, int _dolby = NO_DOLBY, int _fmt = FORMAT_PCM16, int _spdif = NO_SPDIF)
  {
    set(_mode, _dolby, _fmt, _spdif);
  }

  inline void set(int _mode = MODE_STEREO, int _dolby = NO_DOLBY, int _fmt = FORMAT_PCM16, int _spdif = NO_SPDIF)
  {
    mode  = _mode;
    dolby = _dolby;
    fmt   = _fmt;
    spdif = _spdif;

    channels = _mode_ch[mode&15]; 
    memcpy(order, _ds_order[mode&15], sizeof(order));

    level = _levels_tbl[fmt&3];
  }

  inline void set_order(int _order[6]) { memcpy(order, _order, sizeof(order)); }
  inline void get_order(int _order[6]) { memcpy(_order, order, sizeof(order)); }

  inline const char *get_name() const { return spdif? "SPDIF": dolby? "Dolby surround": _mode_names[mode&15]; }

  inline int  acmod()    const { return mode & ~MODE_LFE;        }
  inline bool error()    const { return (mode & ~15)      != 0;  }
  inline bool lfe()      const { return (mode & MODE_LFE) != 0;  }
  inline bool surround() const { return (channels & (CH_MASK_SL | CH_MASK_SR)) != 0;  }

  inline int  nchans()   const { return count_channels(channels); }
  inline int  nfchans()  const { return count_channels(channels & ~CH_MASK_LFE); }
  inline int  nfront()   const { return count_channels(channels & (CH_MASK_L | CH_MASK_C | CH_MASK_R)); }
  inline int  nrear()    const { return count_channels(channels & (CH_MASK_SL | CH_MASK_SR)); }

  inline bool operator ==(Speakers &spk) const
  { 
    return spdif || spk.spdif? spdif == spk.spdif:
           dolby || spk.dolby? dolby == spk.dolby && fmt == spk.fmt:
           mode == spk.mode && fmt == spk.fmt;
  }

  inline bool operator !=(Speakers &spk) const
  { 
    return spdif || spk.spdif? spdif != spk.spdif:
           dolby || spk.dolby? dolby != spk.dolby || fmt != spk.fmt: 
           mode != spk.mode || fmt != spk.fmt;
  }

  bool get_wfx(WAVEFORMATEX *wfx, int sample_rate, bool use_extensible) const;
  bool set_wfx(WAVEFORMATEX *wfx);
  int samples_size(int nsamples);
};




inline int count_channels(int channels)
{
  return ((channels     ) & 1) +
         ((channels >> 1) & 1) +
         ((channels >> 2) & 1) +
         ((channels >> 3) & 1) +
         ((channels >> 4) & 1) +
         ((channels >> 5) & 1);
}

#endif
