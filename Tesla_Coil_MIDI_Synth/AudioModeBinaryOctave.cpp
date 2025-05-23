#include "AudioModeBinaryOctave.h"
#include "Audio.h"

void AudioModeBinaryOctave::reset() {
  pulseWidthCount = 0;
  state = false;
  lastState = false;
  octaveDown = false;
}

// Toggle output on rising edge. Input schmitt triggered. Output pulse width limited.
uint16_t AudioModeBinaryOctave::processSample(int32_t in) {
  // Hope you like LOWER square waves

  // Schmitt trigger
  if(in > 0) state = true;
  else if(in < -(audio.audioNoiseGate << 7) * audio.audioGain / 0x20) state = false;

  // Toggle on rising edge
  if(state && !lastState)
    octaveDown = !octaveDown;

  lastState = state;

  // Limit pulse width
  if(octaveDown) {
    if(pulseWidthCount < audio.pulseWidthMax) {
      pulseWidthCount++;
      return 0xFFFF;
    }
  } else pulseWidthCount = 0;

  return 0;
}

const char *AudioModeBinaryOctave::name() const {
  return "Bin Oct Down";
}
