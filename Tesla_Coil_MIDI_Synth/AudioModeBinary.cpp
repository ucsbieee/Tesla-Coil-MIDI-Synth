#include "AudioModeBinary.h"

void AudioModeBinary::reset() {}

// Output high when sample is greater than noise gate threshold
uint16_t AudioModeBinary::processSample(int32_t in) {
  // Hope you like square waves
  return in > 0 ? 0xFFFF : 0;
}

const char *AudioModeBinary::name() const {
  return "Binary";
}
