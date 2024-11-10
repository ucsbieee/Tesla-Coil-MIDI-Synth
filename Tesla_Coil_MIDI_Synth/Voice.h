#pragma once

#include <inttypes.h>
#include <sam.h>

#include "Drum.h"

#define NVOICES 6
#define MAX_ARP_NOTES 8

class Voices {
public:
  Voices();

  struct Voice {
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
  };

  struct VoiceConfig {
    Tc *timer;
    TcChannel *channel;
    Pio *port;
    uint8_t portN;
    bool peripheralab;
    bool timerab;
  };
  
  const VoiceConfig voiceConfigs[NVOICES];
  
  volatile uint8_t updating = 0;
  
  // Functions
  void init();
  
  Voice &operator[](unsigned int n);
  
private:
  Voice voices[NVOICES];
};
