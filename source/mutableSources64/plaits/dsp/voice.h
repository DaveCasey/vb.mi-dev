// Copyright 2016 Olivier Gillet.
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
// Main synthesis voice.

#ifndef PLAITS_DSP_VOICE_H_
#define PLAITS_DSP_VOICE_H_

#include "stmlib/stmlib.h"

#include "stmlib/dsp/filter.h"
#include "stmlib/dsp/limiter.h"
#include "stmlib/utils/buffer_allocator.h"

#include "plaits/dsp/engine/additive_engine.h"
#include "plaits/dsp/engine/bass_drum_engine.h"
#include "plaits/dsp/engine/chord_engine.h"
#include "plaits/dsp/engine/engine.h"
#include "plaits/dsp/engine/fm_engine.h"
#include "plaits/dsp/engine/grain_engine.h"
#include "plaits/dsp/engine/hi_hat_engine.h"
#include "plaits/dsp/engine/modal_engine.h"
#include "plaits/dsp/engine/noise_engine.h"
#include "plaits/dsp/engine/particle_engine.h"
#include "plaits/dsp/engine/snare_drum_engine.h"
#include "plaits/dsp/engine/speech_engine.h"
#include "plaits/dsp/engine/string_engine.h"
#include "plaits/dsp/engine/swarm_engine.h"
#include "plaits/dsp/engine/virtual_analog_engine.h"
#include "plaits/dsp/engine/waveshaping_engine.h"
#include "plaits/dsp/engine/wavetable_engine.h"

#include "plaits/dsp/envelope.h"

#include "plaits/dsp/fx/low_pass_gate.h"


namespace plaits {

const int kMaxEngines = 16;
const int kMaxTriggerDelay = 8;
const int kTriggerDelay = 5;

class ChannelPostProcessor {
 public:
  ChannelPostProcessor() { }
  ~ChannelPostProcessor() { }
  
  void Init() {
    lpg_.Init();
    Reset();
  }
  
  void Reset() {
    limiter_.Init();
  }
  
  void Process(
      double gain,
      bool bypass_lpg,
      double low_pass_gate_gain,
      double low_pass_gate_frequency,
      double low_pass_gate_hf_bleed,
      double* in,
      short* out,
      size_t size,
      size_t stride) {
    if (gain < 0.0) {
      limiter_.Process(-gain, in, size);
    }
    const double post_gain = (gain < 0.0 ? 1.0 : gain) * -32767.0;
    if (!bypass_lpg) {
      lpg_.Process(
          post_gain * low_pass_gate_gain,
          low_pass_gate_frequency,
          low_pass_gate_hf_bleed,
          in,
          out,
          size,
          stride);
    } else {
      while (size--) {
        *out = stmlib::Clip16(1 + static_cast<int32_t>(*in++ * post_gain));
        out += stride;
      }
    }
  }
    
    // new process, vb
    void Process(
                 double gain,
                 bool bypass_lpg,
                 double low_pass_gate_gain,
                 double low_pass_gate_frequency,
                 double low_pass_gate_hf_bleed,
                 double* in_out,
                 size_t size) {
        if (gain < 0.0) {
            limiter_.Process(-gain, in_out, size);
        }
        const double post_gain = (gain < 0.0 ? 1.0 : gain);
        if (!bypass_lpg) {
            lpg_.Process(
                         post_gain * low_pass_gate_gain,
                         low_pass_gate_frequency,
                         low_pass_gate_hf_bleed,
                         in_out,
                         size);
        } else {
            while (size--) {
                *in_out++ *= post_gain;
            }
        }
    }
    //
  
 private:
  stmlib::Limiter limiter_;
  LowPassGate lpg_;
  
  DISALLOW_COPY_AND_ASSIGN(ChannelPostProcessor);
};

struct Patch {
  double note;
  double harmonics;
  double timbre;
  double morph;
  double frequency_modulation_amount;
  double timbre_modulation_amount;
  double morph_modulation_amount;

  int engine;
  double decay;
  double lpg_colour;
};

struct Modulations {
  double engine;
  double note;
  double frequency;
  double harmonics;
  double timbre;
  double morph;
  double trigger;
  double level;

  bool frequency_patched;
  bool timbre_patched;
  bool morph_patched;
  bool trigger_patched;
  bool level_patched;
};

class Voice {
 public:
  Voice() { }
  ~Voice() { }
  
    
  struct Frame {
    short out;
    short aux;
  };
  
  void Init(stmlib::BufferAllocator* allocator);
    /*
  void RenderOld(
      const Patch& patch,
      const Modulations& modulations,
      Frame* frames,
      size_t size);
    */
    // pass output buffer directly into render function, vb
    void Render(
                const Patch& patch,
                const Modulations& modulations,
                double* out, double* aux,
                size_t size);
    //
    
    
  inline int active_engine() const { return previous_engine_index_; }
    
 private:
  void ComputeDecayParameters(const Patch& settings);
  
  inline double ApplyModulations(
      double base_value,
      double modulation_amount,
      bool use_external_modulation,
      double external_modulation,
      bool use_internal_envelope,
      double envelope,
      double default_internal_modulation,
      double minimum_value,
      double maximum_value) {
    double value = base_value;
    modulation_amount *= std::max(fabs(modulation_amount) - 0.05, 0.05);
      modulation_amount *= 1.05;
    
    double modulation = use_external_modulation
        ? external_modulation
        : (use_internal_envelope ? envelope : default_internal_modulation);
    value += modulation_amount * modulation;
    CONSTRAIN(value, minimum_value, maximum_value);
    return value;
  }
  
  AdditiveEngine additive_engine_;

  BassDrumEngine bass_drum_engine_;
  ChordEngine chord_engine_;
  FMEngine fm_engine_;
  GrainEngine grain_engine_;
  HiHatEngine hi_hat_engine_;
  ModalEngine modal_engine_;
  NoiseEngine noise_engine_;
  ParticleEngine particle_engine_;
  SnareDrumEngine snare_drum_engine_;
  SpeechEngine speech_engine_;
  StringEngine string_engine_;
  SwarmEngine swarm_engine_;
  VirtualAnalogEngine virtual_analog_engine_;
  WaveshapingEngine waveshaping_engine_;
  WavetableEngine wavetable_engine_;
     

  stmlib::HysteresisQuantizer engine_quantizer_;
  
  int previous_engine_index_;
  double engine_cv_;
  
  double previous_note_;
  bool trigger_state_;
  
  DecayEnvelope decay_envelope_;
  LPGEnvelope lpg_envelope_;
  
  double trigger_delay_line_[kMaxTriggerDelay];
  DelayLine<double, kMaxTriggerDelay> trigger_delay_;
  
  ChannelPostProcessor out_post_processor_;
  ChannelPostProcessor aux_post_processor_;
  
  EngineRegistry<kMaxEngines> engines_;
  
  // we don't use these anymore
  //double out_buffer_[kMaxBlockSize];
  //double aux_buffer_[kMaxBlockSize];
  
  DISALLOW_COPY_AND_ASSIGN(Voice);
};

}  // namespace plaits

#endif  // PLAITS_DSP_VOICE_H_
