#include "AudioModeSchmitt.h"
#include "Audio.h"

void AudioModeSchmitt::reset() {
  pulseWidthCount = 0;
  state = false;
}

// Output on when audio exceeds the noise gate level, then only turn off again once audio falls below negative noise gate level (Schmitt trigger behavior). Pulse width clamped to maximum setting.
uint16_t AudioModeSchmitt::processSample(int32_t in) {
  if(in > 0) state = true;
  else if(in < -(audio.audioNoiseGate << 7) * audio.audioGain / 0x20) state = false;
  
  if(state) {
    if(pulseWidthCount < audio.pulseWidthMax) {
      pulseWidthCount++;
      return 0xFFFF;
    }
  } else pulseWidthCount = 0;
  
  return 0;
}

const char *AudioModeSchmitt::name() const {
  return "Schmitt";
}
