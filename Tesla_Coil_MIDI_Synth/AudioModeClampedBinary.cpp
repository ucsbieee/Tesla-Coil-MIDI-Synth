#include "AudioModeClampedBinary.h"
#include "Audio.h"

void AudioModeClampedBinary::reset() {
  pulseWidthCount = 0;
}

// Output high when audio exceeds the noise gate, clamped to maximum pulse width setting
uint16_t AudioModeClampedBinary::processSample(int32_t in) {
  if(in > 0) {
    if(pulseWidthCount < audio.pulseWidthMax) {
      pulseWidthCount++;
      return 0xFFFF;
    }
  } else pulseWidthCount = 0;
  
  return 0;
}

const char *AudioModeClampedBinary::name() const {
  return "Clamped Binary";
}
