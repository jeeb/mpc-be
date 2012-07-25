/*
  Mixer

  Mixer and gain control class. Smooth gain control based on ac3 time window, 
  so delayed samples are also required for operation.

  Usage. 
    Create instance of a mixer, set input and output modes, set matrix 
    directly or call calc_matrix() function, then call mix() function.

    'level' - desired output level. It is guaranteed that Samples will not 
      exceed it.
    'clev', 'slev', 'lfelev' are params for matrix calculation calc_matrix().
    'master', 'gain', 'dynrng' are used in gain control and matrix-independent.
    'normalize' flag controls gain control behavior. True means one-pass
      normalization. So at at the beginning mixing use 'gain' = 'master'.
      When overflow occur gain is decreased and so on. When 'normalize' = false
      then after overflow gain begins to increase bit by bit until it
      reaches 'master' again or other overflow occur.
    'auto_gain' - automatic gain control. It will automatically lower gain 
      level on overload and restore it back then.
    'voice_control' - (only when stereo input) enables voice control when
      stereo input. Amplifies in-phase signal and route it to center 
      speaker if present. Only when auto_matrix = true.
    'expand_stereo' - (only when stereo input) enables surround control when
      stereo input. Amplifies out-of-phase signal and route it to surround 
      speakers if present. Only when auto_matrix = true.



    calc_matrix() - calc mixing matrix complied with ac3 standart (not normalized)
    normalize() - normalizes matrix so no overflow at output if no overflow at input.
    reset() - reset time window, reset to 'master' to 'gain' and 'dynrng' to 1.0.
*/

#ifndef MIXER_H
#define MIXER_H

#include "spk.h"

class Mixer
{
public:
  class Speakers in_mode;
  class Speakers out_mode;

  _sample_t in_gains[NCHANNELS];   // input channel gains
  _sample_t out_gains[NCHANNELS];  // output channel gains

  _sample_t in_levels[NCHANNELS];  // input channel levels
  _sample_t out_levels[NCHANNELS]; // output channel levels

  _sample_t master;             // desired gain (manually set)
  _sample_t gain;               // current gain

  _sample_t clev;               // center mix level
  _sample_t slev;               // surround mix level
  _sample_t lfelev;             // lfe mix level

  _sample_t release;            // release speed dB/s

  bool     drc_on;             // DRC enabled
  _sample_t drc_power;          // DRC power
  _sample_t drc_level;          // current DRC gain

  bool     auto_gain;          // auto gain contol
  bool     normalize;          // one-pass normalize
  bool     expand_stereo;      // controls out-of-phase signal components
  bool     voice_control;      // controls in-phase signal components

  mixer_matrix_t matrix;

  Mixer();
  void reset();
  void calc_matrix(bool _normalize = false);
  void normalize_matrix();
  void mix(sample_buffer_t &samples);
};

#endif
