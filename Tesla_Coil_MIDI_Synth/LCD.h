#pragma once

#include "Synth.h"
#include "MIDI.h"

#include <Arduino.h>
#include <inttypes.h>

namespace LCD {

#define LCD_UPDATE_PERIOD 30 // ms
#define MIDI_PING_LINGER 3 // frames
#define SCREEN_TIMEOUT (10000/LCD_UPDATE_PERIOD) // frames

enum {
  SCREEN_PULSE_WIDTH,
  SCREEN_A,
  SCREEN_D,
  SCREEN_S,
  SCREEN_R,
  SCREEN_TREM_DEPTH,
  SCREEN_TREM_PERIOD,
  SCREEN_TREM_DELAY,
  SCREEN_VIBR_DEPTH,
  SCREEN_VIBR_PERIOD,
  SCREEN_VIBR_DELAY,
  SCREEN_ARP_PERIOD,
  SCREEN_MIDI_MIN_NOTE,
  SCREEN_MIDI_MAX_NOTE,
  SCREEN_MIDI_BASE,
  SCREEN_END
};

extern uint8_t LCDstate;
extern bool editing;

// Functions to display value
uint16_t displayTypeUINT32(void *data);
uint16_t displayTypeUINT8scaled(void *data);
uint16_t displayTypeMIDIchannel(void *data);
uint16_t displayTypeUINT32Hz(void *data);

typedef struct {
  // Text to display
  const char *title;
  const char *units;

  // Function to convert to displayed value
  uint16_t (*convert)(void*);

  // Current value
  void *dataValue;

  // Current value (CC)
  uint8_t *dataCC;

  // MIDI CC number
  uint8_t cc;
  
} LCD_screen_descriptor;

extern const LCD_screen_descriptor screens[];

// Note name sequence
extern const char *noteNames[];

// Functions
void initLCD();
void updateLCD();
void MIDIping(int8_t c);

}
