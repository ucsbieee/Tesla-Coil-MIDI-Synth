#include "AudioModeBinaryOctave.h"

void AudioModeBinaryOctave::reset() {
    currentState = false;
    octaveDown = false;
}

// Output high when sample is greater than noise gate threshold
uint16_t AudioModeBinaryOctave::processSample(int32_t in) {
  // Hope you like LOWER square waves
  if (currentState && in <= 0) { //Toggle octave down on falling edge
    octaveDown = !octaveDown;
  }
  currentState = in > 0;
  return octaveDown ? 0xFFFF : 0;
}

const char *AudioModeBinaryOctave::name() const {
  return "Binary Octave Down";
}
