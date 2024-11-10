#include "AudioModePulseEnergy.h"
#include "Audio.h"

#include <string.h>

void AudioModePulseEnergy::reset() {
  lastSample = 0;
  memset(history, 0, sizeof(history));
  histInd = 0;
  activeElem = nullptr;
  pulseWidthCount = 0;
}

// Generate pulses after a delay that have area proportional to the area of input pulses
uint16_t AudioModePulseEnergy::processSample(int32_t in) {
  // When input is positive...
  if(in > 0) {
    // If the input is crossing zero, we're entering a pulse
    if(lastSample <= 0)
      activeElem = history + histInd;

    // Otherwise indicate we're in a pulse by storing 1
    else history[histInd] = 1;

    // If we are currently integrating a pulse, integrate into the history element where it started
    if(activeElem)
      *activeElem += in;
  }

  lastSample = in;

  // Move history (now looking at oldest history value)
  histInd++;

  // Cancel pulse integration if it's still integrating by the time we need to otuput it
  if(activeElem == history + histInd)
    activeElem = nullptr;

  // Generate an output pulse from the history
  uint32_t width = history[histInd];
  history[histInd] = 0;

  // If hit the start of a pulse, scale and start outputting
  if(width > 1) {
    // Weirdness to avoid overflow while maintaining decent precision
    // Approximately full pulse width for full amplitude 1kHz quarter sine wave pulse
    width = width / 256 * audio.pulseWidthMax / 1024;
    
    if(width > audio.pulseWidthMax)
      pulseWidthCount = audio.pulseWidthMax;
    else
      pulseWidthCount = width;
  }

  // If the input went negative at this time (0 stored in history), cancel pulse
  else if(width == 0) pulseWidthCount = 0;
  
  if(pulseWidthCount) {
    pulseWidthCount--;
    return 0xFFFF;
  }

  return 0;
}

const char *AudioModePulseEnergy::name() const {
  return "Pulse Energy";
}
