// Copyright 2015 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// FM Voice.

#ifndef RINGS_DSP_FM_VOICE_H_
#define RINGS_DSP_FM_VOICE_H_

#include "stmlib/stmlib.h"

#include <algorithm>

#include "stmlib/dsp/filter.h"

#include "rings/dsp/dsp.h"
#include "rings/dsp/follower.h"

#include "rings/resources.h"

namespace rings {

using namespace stmlib;

class FMVoice {
 public:
  FMVoice() { }
  ~FMVoice() { }
  
  void Init();
  void Process(
      const double* in,
      double* out,
      double* aux,
      size_t size);
  
  inline void set_frequency(double frequency) {
    carrier_frequency_ = frequency;
  }
  
  inline void set_ratio(double ratio) {
    ratio_ = ratio;
  }

  inline void set_brightness(double brightness) {
    brightness_ = brightness;
  }
  
  inline void set_damping(double damping) {
    damping_ = damping;
  }
  
  inline void set_position(double position) {
    position_ = position;
  }
  
  inline void set_feedback_amount(double feedback_amount) {
    feedback_amount_ = feedback_amount;
  }
  
  inline void TriggerInternalEnvelope() {
    amplitude_envelope_ = 1.0;
    brightness_envelope_ = 1.0;
  }
  
  inline double SineFm(uint32_t phase, double fm) const {
    phase += (static_cast<uint32_t>((fm + 4.0) * 536870912.0)) << 3;
    uint32_t integral = phase >> 20;
    double fractional = static_cast<double>(phase << 12) / 4294967296.0;
    double a = lut_sine[integral];
    double b = lut_sine[integral + 1];
    return a + (b - a) * fractional;
  }
  
 private:
  double carrier_frequency_;
  double ratio_;
  double brightness_;
  double damping_;
  double position_;
  double feedback_amount_;
  
  double previous_carrier_frequency_;
  double previous_modulator_frequency_;
  double previous_brightness_;
  double previous_damping_;
  double previous_feedback_amount_;
  
  double amplitude_envelope_;
  double brightness_envelope_;
  double gain_;
  double fm_amount_;
  uint32_t carrier_phase_;
  uint32_t modulator_phase_;
  double previous_sample_;
  
  Follower follower_;
  
  DISALLOW_COPY_AND_ASSIGN(FMVoice);
};

}  // namespace rings

#endif  // RINGS_DSP_FM_VOICE_H_