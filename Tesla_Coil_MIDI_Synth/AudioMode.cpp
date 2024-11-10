#include "AudioMode.h"

AudioMode::AudioMode(Audio &audio): audio(audio) {}

void AudioMode::reset() {}

uint16_t AudioMode::processSample(int32_t in) {
  (void)in;
  return 0;
}

const char *AudioMode::name() const {
  return "Off";
}
