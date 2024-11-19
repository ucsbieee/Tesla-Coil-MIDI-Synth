#include "AudioModeBinaryDDT.h"
#include "Audio.h"

void AudioModeBinaryDDT::reset() {
  lastSample = 0;
  pulseWidthCount = 0;
}

// On when derivative exceeds a certain threshold, clamped to maximum pulse width setting
uint16_t AudioModeBinaryDDT::processSample(int32_t in) {
  uint16_t out = 0;
  
  if(in - lastSample > 500) {
    if(pulseWidthCount < audio.pulseWidthMax) {
      out = 0xFFFF;
      pulseWidthCount++;
    }
  } else pulseWidthCount = 0;

  lastSample = in;

  return out;
}

const char *AudioModeBinaryDDT::name() const {
  return "Binary DDT";
}
