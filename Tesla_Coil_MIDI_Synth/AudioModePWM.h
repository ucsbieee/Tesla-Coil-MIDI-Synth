#pragma once

#include "AudioMode.h"

class AudioModePWM: public AudioMode {
public:
  using AudioMode::AudioMode;

  virtual void reset() override;
  virtual uint16_t processSample(int32_t in) override;
  
  virtual const char *name() const override;

private:
  int32_t baseline = 0;
};
