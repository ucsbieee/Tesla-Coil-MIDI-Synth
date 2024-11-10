#pragma once

#include <inttypes.h>

class Synth;
class Audio;

class Connected {
public:
  Connected(Synth &synth, Audio &audio);
  
  void checkConnected();
  
private:
  Synth &synth;
  Audio &audio;
  
  static uint16_t lastFrameNumber;
  static int8_t missedFrameCount;
};
