#ifndef VOICE_H
#define VOICE_H

#include <inttypes.h>
#include <sam.h>

#include "Drum.h"

namespace Voice {

#define MAX_ARP_NOTES 8

typedef struct {
  // MIDI information
  uint8_t midiChannel;
  uint8_t midiNote;
  uint8_t midiVel;
  int16_t midiPB;
  bool midiNoteDown;

  // Oscillator state
  uint32_t period;
  uint32_t pulseWidth;

  // This voice is still sounding
  bool active;

  // ADSR
  unsigned char adsrStage;
  unsigned long adsrTimestamp;
  uint8_t lastEnv;

  // Drum sample
  const Drum::Drum *drum;

  // Arp notes
  uint8_t arpNotes[MAX_ARP_NOTES];
  unsigned long arpNoteEndTimestamps[MAX_ARP_NOTES];
  uint8_t arpNotesIndex;
  unsigned long arpTimestamp;
  
  // Timestamp when this note started
  unsigned long noteDownTimestamp;
} Voice;

typedef struct {
  Tc *timer;
  TcChannel *channel;
  Pio *port;
  uint8_t portN;
  bool peripheralab;
  bool timerab;
} VoiceConfig;

static const VoiceConfig voiceConfigs[] = {
  {TC0, &TC0->TC_CHANNEL[0], PIOB, 25, 1, 0},
  {TC0, &TC0->TC_CHANNEL[1], PIOA, 2 , 0, 0},
  {TC0, &TC0->TC_CHANNEL[2], PIOA, 5 , 0, 0},
  {TC1, &TC1->TC_CHANNEL[0], PIOB, 0,  1, 0},
  {TC1, &TC1->TC_CHANNEL[1], PIOB, 2,  1, 0},
  {TC1, &TC1->TC_CHANNEL[2], PIOB, 4,  1, 0}
};

#define NVOICES (sizeof(::Voice::voiceConfigs)/sizeof(::Voice::VoiceConfig))

extern Voice voices[];
extern volatile uint8_t voicesUpdating;

// Functions
void initVoices();

}

#endif
