#include "AudioModePWM.h"
#include "Audio.h"

void AudioModePWM::reset() {
  baseline = 0;
}

// Output 48kHz PWM with pulse widths proportional to sample value
uint16_t AudioModePWM::processSample(int32_t in) {
  // Clamp minimum input to zero
  if(in - baseline < 0)
    baseline = in;

  // Subtract away baseline
  in -= baseline;

  // Decay baseline to zero
  baseline = baseline*253/256;

  // Apply volume and make proportional to max PWM counter value
  return in * audio.pwmWidthMax / 0x10000;
}

const char *AudioModePWM::name() const {
  return "PWM";
}
