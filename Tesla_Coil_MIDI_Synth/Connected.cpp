#include "Connected.h"
#include "Synth.h"
#include "Audio.h"

#include <Arduino.h>

uint16_t Connected::lastFrameNumber;
int8_t Connected::missedFrameCount = -1;

Connected::Connected(Synth &synth, Audio &audio): synth(synth), audio(audio) {}

void Connected::checkConnected() {
  // Should increment at 1ms USB frame interval
  const uint16_t frameNumber = (UOTGHS->UOTGHS_DEVFNUM & UOTGHS_DEVFNUM_FNUM_Msk) >> UOTGHS_DEVFNUM_FNUM_Pos;

  if(frameNumber == lastFrameNumber) {
    // Only count missed frames if we have received some in the past
    if(missedFrameCount >= 0)
      missedFrameCount++;
  }

  else missedFrameCount = 0;

  lastFrameNumber = frameNumber;

  // Disable output if we go more than 10ms without a USB frame
  if(missedFrameCount >= 10) {
    missedFrameCount = -1;
    synth.stop();
    audio.stop();
  }
}
