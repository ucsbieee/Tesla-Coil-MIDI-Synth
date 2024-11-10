#include "AudioModePWMDDT.h"
#include "Audio.h"

void AudioModePWMDDT::reset() {
  lastSample = 0;
}

// 48kHz PWM duty cycle proportional to derivative
uint16_t AudioModePWMDDT::processSample(int32_t in) {
  // Derivative is over a single cycle -- significant amplification of high frequencies
  // Magic number 4 seems ok given histograms of dt for example audio file
  int32_t dt = (in - lastSample)*4;
  if(dt > 0x7FFF) dt = 0x7FFF;

  lastSample = in;

  // Clamp to positive
  // Apply volume and make proportional to max PWM counter value
  if(dt > 0)
    return dt * audio.pwmWidthMax / 0x8000;

  return 0;
}

const char *AudioModePWMDDT::name() const {
  return "PWM DDT";
}
