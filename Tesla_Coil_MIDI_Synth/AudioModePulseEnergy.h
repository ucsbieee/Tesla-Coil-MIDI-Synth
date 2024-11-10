#pragma once

#include "AudioMode.h"

class AudioModePulseEnergy: public AudioMode {
public:
  using AudioMode::AudioMode;

  virtual void reset() override;
  virtual uint16_t processSample(int32_t in) override;
  
  virtual const char *name() const override;

private:
  int32_t lastSample = 0;

  // History of pulse lengths defined at their starts
  uint32_t history[256] = {0};
  uint8_t histInd = 0;

  // Which history element we're currently adding to (start of the pulse)
  uint32_t *activeElem = nullptr;

  // Remaining duration of current pulse output
  uint16_t pulseWidthCount = 0;
};
