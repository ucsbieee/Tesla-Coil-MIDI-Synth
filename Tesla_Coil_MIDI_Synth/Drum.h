#pragma once

#include <inttypes.h>

namespace Drum {

// Drum presets
typedef struct {
  // MIDI note to be triggered by
  uint8_t midiNote;

  // Frequency to apply modulation around
  float baseNote;
  
  // ADSR envelope (drums are momentary things, don't need D or S)
  uint32_t a;
  uint32_t r;

  // Amount/type of frequency modulation
  float noiseMod;
  float envMod;
} Drum;

#define NDRUMS 5
extern const Drum drumPresets[NDRUMS];

}
