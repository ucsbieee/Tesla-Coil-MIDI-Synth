#ifndef LCD_H
#define LCD_H

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

static const LCD_screen_descriptor screens[] = {
  { // SCREEN_PULSE_WIDTH
    "P Width: ",
    "%",
    NULL,
    NULL, // Special
    NULL,
    0
  },
  { // SCREEN_A
    "Attack",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::attack,
    &MIDI::attackCC,
    ATTACK_CC
  },
  { // SCREEN_D
    "Decay",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::decay,
    &MIDI::decayCC,
    DECAY_CC
  },
  { // SCREEN_S
    "Sustain",
    "%",
    &displayTypeUINT8scaled,
    (void*)&MIDI::sustain,
    &MIDI::sustainCC,
    SUSTAIN_CC
  },
  { // SCREEN_R
    "Release",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::release,
    &MIDI::releaseCC,
    RELEASE_CC
  },
  { // SCREEN_TREM_DEPTH
    "Trem. Depth",
    "%",
    &displayTypeUINT8scaled,
    (void*)&MIDI::tremoloDepth,
    &MIDI::tremoloDepthCC,
    TREMOLO_DEPTH_CC
  },
  { // SCREEN_TREM_PERIOD
    "Trem. Period",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::tremoloPeriod,
    &MIDI::tremoloPeriodCC,
    TREMOLO_PERIOD_CC
  },
  { // SCREEN_TREM_DELAY
    "Trem. Delay",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::tremoloDelay,
    &MIDI::tremoloDelayCC,
    TREMOLO_DELAY_CC
  },
  { // SCREEN_VIBR_DEPTH
    "Vibr. Depth",
    "%",
    &displayTypeUINT8scaled,
    (void*)&MIDI::vibratoDepth,
    &MIDI::vibratoDepthCC,
    VIBRATO_DEPTH_CC
  },
  { // SCREEN_VIBR_PERIOD
    "Vibr. Period",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::vibratoPeriod,
    &MIDI::vibratoPeriodCC,
    VIBRATO_PERIOD_CC
  },
  { // SCREEN_VIBR_DELAY
    "Vibr. Delay",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::vibratoDelay,
    &MIDI::vibratoDelayCC,
    VIBRATO_DELAY_CC
  },
  { // SCREEN_ARP_PERIOD
    "Arp. Period",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::arpeggioPeriod,
    &MIDI::arpeggioPeriodCC,
    ARPEGGIO_CC
  },
  { // SCREEN_MIDI_MIN_NOTE
    "Min Note",
    "",
    NULL,
    NULL,
    NULL,
    0
  },
  { // SCREEN_MIDI_MAX_NOTE
    "Max Note",
    "",
    NULL,
    NULL,
    NULL,
    0
  },
  { // SCREEN_MIDI_BASE
    "MIDI Base CH",
    "",
    &displayTypeMIDIchannel,
    &MIDI::MIDIbaseChannel,
    NULL,
    0
  }
};

// Note name sequence
static const char *noteNames[] = {
  "C",
  "C#",
  "D",
  "D#",
  "E",
  "F",
  "F#",
  "G",
  "G#",
  "A",
  "A#",
  "B",
};

// Functions
void initLCD();
void updateLCD();
void MIDIping(int8_t c);

}

#endif
