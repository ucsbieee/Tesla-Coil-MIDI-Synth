#pragma once

#include "AudioMode.h"

#define AMP_HIST_LEN 8

class AudioModePredictive: public AudioMode {
public:
  using AudioMode::AudioMode;

  virtual void reset() override;
  virtual uint16_t processSample(int32_t in) override;
  
  virtual const char *name() const override;

private:
  // History of audio samples
  int32_t history[AMP_HIST_LEN] = {0};
  uint8_t histInd = 0;

  // Last derivative computed
  int32_t lastDDT = 0;

  // Duration of current pulse output
  uint16_t pulseWidthCount = 0;
  uint16_t pulseWidthTarget = 0;

  // Flag if derivative has started falling
  bool ddtFall = false;
};
