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

#define NVOICES 6

extern const VoiceConfig voiceConfigs[NVOICES];

extern Voice voices[NVOICES];
extern volatile uint8_t voicesUpdating;

// Functions
void initVoices();

}

#endif
