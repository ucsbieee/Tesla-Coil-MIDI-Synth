#pragma once

#include <inttypes.h>

// Uncomment to automatically reduce pulse width when many voices are playing at once
//#define AUTODUCK

// Uncomment to make MIDI velocity control absolute pulse width instead of duty cycle
//#define ABSOLUTE_PULSE_WIDTH

#define EXP_CRUNCH 3

#define MAX_WIDTH ((uint32_t)(F_CPU/2*3e-3)) // max pulse width (3ms)
#define MIN_WIDTH ((uint32_t)(F_CPU/2*10e-6))  // min pulse width (10us)
#define MIN_OFF_TIME ((int32_t)(F_CPU/2*50e-6)) // minimum time between pulses on each channel (50us)
#define VEL_THRESH 1 // minimum velocity
#define MAX_FREQ 4000 // if frequency is too high, pulses just merge together

#define LUTSIZE 256

class Voices;
class MIDI;

class Synth {
public:
  Synth(Voices &voices, MIDI &midi);
  
  uint8_t vol = 64;
  
  // Functions
  void stop();
  void update();
  
private:
  Voices &voices;
  MIDI &midi;
  
  static uint8_t eLookup[LUTSIZE];
  static int8_t sinLookup[LUTSIZE];
  
  void updateWidth(uint8_t chan, uint32_t pulseWidth);
  void updatePeriod(uint8_t chan, uint32_t period);
};
