#include "AudioModePredictive.h"
#include "Audio.h"

#include <string.h>

void AudioModePredictive::reset() {
  memset(history, 0, sizeof(history));
  histInd = 0;
  lastDDT = 0;
  pulseWidthCount = 0;
  pulseWidthTarget = 0;
  ddtFall = false;
}

// Predict how much energy will be in a pulse based on the filtered derivative near zero crossing
uint16_t AudioModePredictive::processSample(int32_t in) {
  // Keep sample history
  history[histInd] = in;
  histInd = (histInd + 1) % AMP_HIST_LEN;

  // Compute derivative over a longer time to reduce noise
  const int32_t ddt = in - history[histInd];

  if(in > 0) {
    if(ddt > 0) {
      // If second derivative is negative, stop recomputing pulse width
      if(ddt < lastDDT)
        ddtFall = true;
      
      lastDDT = ddt;
      
      // If derivative hasn't started falling yet...
      if(!ddtFall) {
        // Continuously recopmute target duration
        pulseWidthTarget = ddt * audio.pulseWidthMax / 0x2000;
        
        // Keep output on
        if(pulseWidthTarget <= pulseWidthCount)
          pulseWidthTarget = pulseWidthCount + 1;
        
        // Clamp to max pulse width
        if(pulseWidthTarget > audio.pulseWidthMax)
          pulseWidthTarget = audio.pulseWidthMax;
      }
    }
    
    // If derivative goes negative after a pulse has started, stop recomputing pulse width
    else if(pulseWidthTarget) ddtFall = true;

    if(pulseWidthCount < pulseWidthTarget) {
      pulseWidthCount++;
      return 0xFFFF;
    }
  }

  // Reset flag when outside pulse
  else {
    ddtFall = false;
    lastDDT = 0;
    pulseWidthCount = 0;
    pulseWidthTarget = 0;
  }

  return 0;
}

const char *AudioModePredictive::name() const {
  return "Predictive";
}
