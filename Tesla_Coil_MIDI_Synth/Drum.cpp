#include "Drum.h"

namespace Drum {

const Drum drumPresets[] = {
  {42, 500,  0, 50,  0.5,  0.0},  // Closed hi-hat (F#2)
  {39, 1000, 0, 125, 0.75, 0.0},  // Clap (D#2)
  {48, 100,  0, 250, 0.25, 0.25}, // Tom (C3)
  {40, 800,  0, 500, 0.5, -0.5},  // Snare (E2)
  {36, 30,   0, 200, 0.0,  0.15}  // Kick (C2)
};

}
