#pragma once

#include <inttypes.h>

// Uncomment to print incoming midi bytes over USB
// #define PRINTMIDI

// Uncomment to save MIDI base channel to flash
// #define SAVE_BASE_MIDI

class Voices;
class Synth;
class LCD;

class MIDI {
public:
  MIDI(Voices &voices, Synth &synth, LCD &lcd);
  
  static const float midi2freq[];
  
  // Channels
  enum {
    CHANNEL_CLEAN,
    CHANNEL_FX,
    CHANNEL_ARP,
    CHANNEL_DRUM,
    CHANNEL_INVALID
  };
  
  uint8_t MIDIbaseChannel = 0;

// Effect settings
#define TREMOLO_PERIOD_MAX 3000
#define TREMOLO_PERIOD_MIN 10
#define TREMOLO_DELAY_MAX 10000

#define TREMOLO_DEPTH_FROM_CC(value) ((uint32_t)value*255/127)
#define TREMOLO_PERIOD_FROM_CC(value) (((uint32_t)TREMOLO_PERIOD_MAX-TREMOLO_PERIOD_MIN)*(0x7F-value)/0x7F+TREMOLO_PERIOD_MIN)
#define TREMOLO_DELAY_FROM_CC(value) ((uint32_t)TREMOLO_DELAY_MAX*value/0x7F)

#define TREMOLO_DEPTH_DEFAULT_CC 0
#define TREMOLO_PERIOD_DEFAULT_CC 64
#define TREMOLO_DELAY_DEFAULT_CC 13

#define TREMOLO_DEPTH_DEFAULT TREMOLO_DEPTH_FROM_CC(TREMOLO_DEPTH_DEFAULT_CC)
#define TREMOLO_PERIOD_DEFAULT TREMOLO_PERIOD_FROM_CC(TREMOLO_PERIOD_DEFAULT_CC)
#define TREMOLO_DELAY_DEFAULT TREMOLO_DELAY_FROM_CC(TREMOLO_DELAY_DEFAULT_CC)

#define TREMOLO_DEPTH_CC 92
#define TREMOLO_PERIOD_CC 102
#define TREMOLO_DELAY_CC 103

  uint8_t tremoloDepth = TREMOLO_DEPTH_DEFAULT;
  uint32_t tremoloPeriod = TREMOLO_PERIOD_DEFAULT;
  uint32_t tremoloDelay = TREMOLO_DELAY_DEFAULT;
  uint8_t tremoloDepthCC = TREMOLO_DEPTH_DEFAULT_CC;
  uint8_t tremoloPeriodCC = TREMOLO_PERIOD_DEFAULT_CC;
  uint8_t tremoloDelayCC = TREMOLO_DELAY_DEFAULT_CC;

#define VIBRATO_DEPTH_MAX 128
#define VIBRATO_PERIOD_MAX 3000
#define VIBRATO_PERIOD_MIN 10
#define VIBRATO_DELAY_MAX 10000

#define VIBRATO_DEPTH_FROM_CC(value) ((uint32_t)VIBRATO_DEPTH_MAX*value/0x7F)
#define VIBRATO_PERIOD_FROM_CC(value) (((uint32_t)VIBRATO_PERIOD_MAX-VIBRATO_PERIOD_MIN)*(0x7F-value)/0x7F+VIBRATO_PERIOD_MIN)
#define VIBRATO_DELAY_FROM_CC(value) ((uint32_t)VIBRATO_DELAY_MAX*value/0x7F)

#define VIBRATO_DEPTH_DEFAULT_CC 0
#define VIBRATO_PERIOD_DEFAULT_CC 64
#define VIBRATO_DELAY_DEFAULT_CC 13

#define VIBRATO_DEPTH_DEFAULT VIBRATO_DEPTH_FROM_CC(VIBRATO_DEPTH_DEFAULT_CC)
#define VIBRATO_PERIOD_DEFAULT VIBRATO_PERIOD_FROM_CC(VIBRATO_PERIOD_DEFAULT_CC)
#define VIBRATO_DELAY_DEFAULT VIBRATO_DELAY_FROM_CC(VIBRATO_DELAY_DEFAULT_CC)

#define VIBRATO_DEPTH_CC 77
#define VIBRATO_PERIOD_CC 76
#define VIBRATO_DELAY_CC 78

  uint8_t vibratoDepth = VIBRATO_DEPTH_DEFAULT;
  uint32_t vibratoPeriod = VIBRATO_PERIOD_DEFAULT;
  uint32_t vibratoDelay = VIBRATO_DELAY_DEFAULT;
  uint8_t vibratoDepthCC = VIBRATO_DEPTH_DEFAULT_CC;
  uint8_t vibratoPeriodCC = VIBRATO_PERIOD_DEFAULT_CC;
  uint8_t vibratoDelayCC = VIBRATO_DELAY_DEFAULT_CC;

#define ADSR_MAX 2000

#define ADR_FROM_CC(value) ((uint32_t)ADSR_MAX*value/0x7F)
#define S_FROM_CC(value) ((uint32_t)value*255/127)

#define ATTACK_DEFAULT_CC 0
#define DECAY_DEFAULT_CC 0
#define SUSTAIN_DEFAULT_CC 127
#define RELEASE_DEFAULT_CC 0

#define ATTACK_DEFAULT ADR_FROM_CC(ATTACK_DEFAULT_CC)
#define DECAY_DEFAULT ADR_FROM_CC(DECAY_DEFAULT_CC)
#define SUSTAIN_DEFAULT S_FROM_CC(SUSTAIN_DEFAULT_CC)
#define RELEASE_DEFAULT ADR_FROM_CC(RELEASE_DEFAULT_CC)

#define ATTACK_CC 116
#define DECAY_CC 117
#define SUSTAIN_CC 118
#define RELEASE_CC 119

  uint32_t attack = ATTACK_DEFAULT;
  uint32_t decay = DECAY_DEFAULT;
  uint8_t sustain = SUSTAIN_DEFAULT;
  uint32_t release = RELEASE_DEFAULT;
  uint8_t attackCC = ATTACK_DEFAULT_CC;
  uint8_t decayCC = DECAY_DEFAULT_CC;
  uint8_t sustainCC = SUSTAIN_DEFAULT_CC;
  uint8_t releaseCC = RELEASE_DEFAULT_CC;

#define ARPEGGIO_PERIOD_MIN 5
#define ARPEGGIO_PERIOD_MAX 100
#define ARPEGGIO_LINGER 50

#define ARPEGGIO_PERIOD_FROM_CC(value) (((uint32_t)ARPEGGIO_PERIOD_MAX-ARPEGGIO_PERIOD_MIN)*(0x7F-value)/0x7F+ARPEGGIO_PERIOD_MIN)

#define ARPEGGIO_PERIOD_DEFAULT_CC 66

#define ARPEGGIO_PERIOD_DEFAULT ARPEGGIO_PERIOD_FROM_CC(ARPEGGIO_PERIOD_DEFAULT_CC)

#define ARPEGGIO_CC 14

  uint32_t arpeggioPeriod = ARPEGGIO_PERIOD_DEFAULT;
  uint8_t arpeggioPeriodCC = ARPEGGIO_PERIOD_DEFAULT_CC;

#define PITCH_BEND_RANGE 1.1224620483f // 2 semitones

// Limit note range
#define MIDI_MAX_NOTE 107 // B7

  uint8_t minNote = 0;
  uint8_t maxNote = MIDI_MAX_NOTE;

  // Functions
  void noteDown(uint8_t channel, uint8_t note, uint8_t vel);
  void noteUp(uint8_t channel, uint8_t note);
  void aftertouch(uint8_t channel, uint8_t note, uint8_t vel);
  void pitchBend(uint8_t channel, uint8_t low7, uint8_t high7);
  void cc(uint8_t channel, uint8_t control, uint8_t value);
  void handleMIDI(unsigned char byte1, unsigned char byte2, unsigned char byte3);
  void init();
  void process();

private:
  Voices &voices;
  Synth &synth;
  LCD &lcd;
  
  // Buffer data coming in through hardware MIDI
  unsigned char hwMIDIbuf[3], hwMIDIbufInd = 0;

  // Expected length of a MIDI message based on the first byte
  static uint8_t expectedLength(uint8_t command);
};
