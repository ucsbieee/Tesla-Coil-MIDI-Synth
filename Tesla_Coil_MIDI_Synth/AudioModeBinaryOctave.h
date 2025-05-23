#pragma once

#include "AudioMode.h"

class AudioModeBinaryOctave: public AudioMode {
public:
  using AudioMode::AudioMode;

  virtual void reset() override;
  virtual uint16_t processSample(int32_t in) override;
  
  virtual const char *name() const override;

private:
  bool currentState;
  bool octaveDown;
};