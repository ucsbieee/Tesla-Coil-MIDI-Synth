#ifndef DRUM_H
#define DRUM_H

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

static const Drum drumPresets[] = {
  {42, 500,  0, 50,  0.5,  0.0},  // Closed hi-hat (F#2)
  {39, 1000, 0, 125, 0.75, 0.0},  // Clap (D#2)
  {48, 100,  0, 250, 0.25, 0.25}, // Tom (C3)
  {40, 800,  0, 500, 0.5, -0.5},  // Snare (E2)
  {36, 30,   0, 200, 0.0,  0.15}  // Kick (C2)
};

}

#endif
