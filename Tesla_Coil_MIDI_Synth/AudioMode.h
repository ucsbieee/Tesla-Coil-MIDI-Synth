#pragma once

#include <inttypes.h>

class Audio;

// Base class for all processing algorithms
class AudioMode {
public:
  AudioMode(Audio &audio);
  
  virtual void reset();
  virtual uint16_t processSample(int32_t in);
  
  virtual const char *name() const;
  
protected:
  Audio &audio;
};
