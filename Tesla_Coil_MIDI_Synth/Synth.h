#pragma once

#include <inttypes.h>

namespace Synth {

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

#define DEFAULT_VOL 64
extern uint8_t vol;

// Functions
void initSynth();
void stopSynth();
void updateSynth();

}
