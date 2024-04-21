#include "MIDI.h"
#include "Synth.h"
#include "Voice.h"
#include "LCD.h"

#include <Arduino.h>
#include <limits.h>
#include <MIDIUSB.h>

namespace MIDI {

const float midi2freq[] = {8.18,8.66,9.18,9.72,10.30,10.91,11.56,12.25,12.98,13.75,14.57,15.43,16.35,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,55.00,58.27,61.74,65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,554.37,587.33,622.25,659.26,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50,1108.73,1174.66,1244.51,1318.51,1396.91,1479.98,1567.98,1661.22,1760.00,1864.66,1975.53,2093.00,2217.46,2349.32,2489.02,2637.02,2793.83,2959.96,3135.96,3322.44,3520.00,3729.31,3951.07,4186.01,4434.92,4698.64,4978.03,5274.04,5587.65,5919.91,6271.93,6644.88,7040.00,7458.62,7902.13,8372.02,8869.84,9397.27,9956.06,10548.08,11175.30,11839.82,12543.85};

// CC parameters
uint8_t tremoloDepth = TREMOLO_DEPTH_DEFAULT;
uint32_t tremoloPeriod = TREMOLO_PERIOD_DEFAULT;
uint32_t tremoloDelay = TREMOLO_DELAY_DEFAULT;
uint8_t tremoloDepthCC = TREMOLO_DEPTH_DEFAULT_CC;
uint8_t tremoloPeriodCC = TREMOLO_PERIOD_DEFAULT_CC;
uint8_t tremoloDelayCC = TREMOLO_DELAY_DEFAULT_CC;

uint8_t vibratoDepth = VIBRATO_DEPTH_DEFAULT;
uint32_t vibratoPeriod = VIBRATO_PERIOD_DEFAULT;
uint32_t vibratoDelay = VIBRATO_DELAY_DEFAULT;
uint8_t vibratoDepthCC = VIBRATO_DEPTH_DEFAULT_CC;
uint8_t vibratoPeriodCC = VIBRATO_PERIOD_DEFAULT_CC;
uint8_t vibratoDelayCC = VIBRATO_DELAY_DEFAULT_CC;

uint32_t attack = ATTACK_DEFAULT;
uint32_t decay = DECAY_DEFAULT;
uint8_t sustain = SUSTAIN_DEFAULT;
uint32_t release = RELEASE_DEFAULT;
uint8_t attackCC = ATTACK_DEFAULT_CC;
uint8_t decayCC = DECAY_DEFAULT_CC;
uint8_t sustainCC = SUSTAIN_DEFAULT_CC;
uint8_t releaseCC = RELEASE_DEFAULT_CC;

uint32_t arpeggioPeriod = ARPEGGIO_PERIOD_DEFAULT;
uint8_t arpeggioPeriodCC = ARPEGGIO_PERIOD_DEFAULT_CC;

// Limit note range
uint8_t minNote = 0;
uint8_t maxNote = MIDI_MAX_NOTE;

// Buffer data coming in through hardware MIDI
unsigned char hwMIDIbuf[3], hwMIDIbufInd = 0;

// Base channel
uint8_t MIDIbaseChannel = 0;

void noteDown(uint8_t channel, uint8_t note, uint8_t vel) {
  if(channel >= CHANNEL_INVALID) return;

  // Reject notes we shouldn't play
  if(note < minNote || note > maxNote) return;

  unsigned long ms = millis();

  const Drum::Drum *drum = NULL;
  if(channel == CHANNEL_DRUM) {
    for(unsigned int x=0; x<NDRUMS; x++)
      if(Drum::drumPresets[x].midiNote == note) {
        drum = Drum::drumPresets+x;
        break;
      }
    if(!drum) return;
  }
  
  Voice::voicesUpdating = 1;
  
  int chosen = -1;

  // Arp notes should be combined
  if(channel == CHANNEL_ARP)
    for(unsigned int x=0; x<NVOICES; x++)
      if(Voice::voices[x].midiChannel == CHANNEL_ARP && (Voice::voices[x].midiNoteDown == Voice::voices[x].active)) {
        chosen = x;
        break;
      }

  // Use any unused voice
  for(unsigned int x=0; chosen<0 && x<NVOICES; x++)
    if(!Voice::voices[x].active)
      chosen = x;

  // If all voices are in use...
  if(chosen < 0 && channel == CHANNEL_DRUM) {
    // Drum prefers to replace another drum
    unsigned long longest = 0;
    for(unsigned int x=0; x<NVOICES; x++) {
      if(Voice::voices[x].midiChannel == CHANNEL_DRUM) {
        unsigned long dt = ms - Voice::voices[x].noteDownTimestamp;
        if(dt >= longest) {
          chosen = x;
          longest = dt;
        }
      }
    }
  }

  if(chosen < 0) {
    // Replace oldtest note
    unsigned long longest = 0;
    for(unsigned int x=0; x<NVOICES; x++) {
      unsigned long dt = ms - Voice::voices[x].noteDownTimestamp;
      if(dt >= longest) {
        chosen = x;
        longest = dt;
      }
    }
  }

  Voice::Voice &chosenVoice = Voice::voices[chosen];

  if(channel == CHANNEL_ARP) {
    int arpInsert = 0;
    
    // Keep track of the note the index was pointing to
    int playingNote = chosenVoice.arpNotes[chosenVoice.arpNotesIndex];
    
    // Get rid of empty notes at the left to make space for new one
    // Want to make sure arpInsert is a valid index at the end of this loop
    for(; arpInsert<MAX_ARP_NOTES-1; arpInsert++) {
      if(chosenVoice.arpNoteEndTimestamps[arpInsert] > ms) {
        if(chosenVoice.arpNotes[arpInsert] == note) return; // Already have note playing
        if(chosenVoice.arpNotes[arpInsert] > note) break; // Notes should be in ascending order
      } else {
        bool found = false;
        for(int x=arpInsert+1; x<MAX_ARP_NOTES; x++) // If not playing, look for one that is playing to move back
        if(chosenVoice.arpNoteEndTimestamps[x] > ms && chosenVoice.arpNotes[x] <= note) {
          if(chosenVoice.arpNotes[x] == note) return; // Keep making sure the new note isn't already here
          chosenVoice.arpNotes[arpInsert] = chosenVoice.arpNotes[x]; // Move note back
          chosenVoice.arpNoteEndTimestamps[arpInsert] = chosenVoice.arpNoteEndTimestamps[x];
          chosenVoice.arpNoteEndTimestamps[x] = 0;
          found = true;
          break;
        }
        if(!found) break; // Nothing to move back, can stop here
      }
    }
    
    // Compress all the stuff to the right
    for(int x=MAX_ARP_NOTES-1; x>arpInsert; x--) {
      if(chosenVoice.arpNoteEndTimestamps[x] > ms) {
        if(chosenVoice.arpNotes[x] == note) return; // Already have note playing
      } else {
        bool found = false;
        for(int y=x-1; y>=arpInsert; y--) { // If not playing, look for one that is playing to move back
          if(chosenVoice.arpNoteEndTimestamps[y] > ms) {
            if(chosenVoice.arpNotes[y] == note) return; // Keep making sure the new note isn't already here
            chosenVoice.arpNotes[x] = chosenVoice.arpNotes[y]; // Move note forward
            chosenVoice.arpNoteEndTimestamps[x] = chosenVoice.arpNoteEndTimestamps[y];
            chosenVoice.arpNoteEndTimestamps[y] = 0;
            found = true;
            break;
          }
        }
        if(!found) break; // Nothing to move back, can stop here
      }
    }
    
    chosenVoice.arpNotes[arpInsert] = note;
    chosenVoice.arpNoteEndTimestamps[arpInsert] = ULONG_MAX;
    
    for(chosenVoice.arpNotesIndex = 0; chosenVoice.arpNotesIndex<MAX_ARP_NOTES; chosenVoice.arpNotesIndex++)
      if(chosenVoice.arpNoteEndTimestamps[chosenVoice.arpNotesIndex] > ms && chosenVoice.arpNotes[chosenVoice.arpNotesIndex] > playingNote) { // Make sure arpNotesIndex points to the current note playing so we always increase in frequency
        chosenVoice.arpNotesIndex--;
        break;
      }
    if(chosenVoice.arpNotesIndex >= MAX_ARP_NOTES) chosenVoice.arpNotesIndex = MAX_ARP_NOTES-1;
    
  } else {
    memset(chosenVoice.arpNoteEndTimestamps, 0, sizeof(chosenVoice.arpNoteEndTimestamps));
    chosenVoice.arpNotesIndex = 0;
    chosenVoice.arpTimestamp = ms;
  }

  chosenVoice.midiChannel = channel;
  chosenVoice.midiNote = note;
  chosenVoice.midiVel = vel;
  if(!chosenVoice.active) chosenVoice.midiPB = 0;
  chosenVoice.active = true;
  chosenVoice.midiNoteDown = true;
  chosenVoice.noteDownTimestamp = ms;

  chosenVoice.adsrStage = 0;
  chosenVoice.adsrTimestamp = chosenVoice.noteDownTimestamp;
  chosenVoice.lastEnv = 0;

  chosenVoice.drum = drum;

  if(Voice::voicesUpdating > 1) {
    Voice::voicesUpdating = 0;
    Synth::updateSynth();
    return;
  }
  Voice::voicesUpdating = 0;
}

void noteUp(uint8_t channel, uint8_t note) {
  Voice::voicesUpdating = 1;

  for(unsigned int x=0; x<NVOICES; x++) {
    Voice::Voice &voice = Voice::voices[x];
    if(voice.midiChannel == channel) {
      if(channel == CHANNEL_ARP) {
        unsigned long ms = millis();
        for(int y=0; y<MAX_ARP_NOTES; y++)
          if(voice.arpNotes[y] == note && voice.arpNoteEndTimestamps[y] > ms) {
            voice.arpNoteEndTimestamps[y] = ms+ARPEGGIO_LINGER;
            break;
          }
      } else if(voice.midiNote == note) voice.midiNoteDown = false;
    }
  }

  if(Voice::voicesUpdating > 1) {
    Voice::voicesUpdating = 0;
    Synth::updateSynth();
    return;
  }
  Voice::voicesUpdating = 0;
}

void aftertouch(uint8_t channel, uint8_t note, uint8_t vel) {
  for(unsigned int x=0; x<NVOICES; x++) {
    Voice::Voice &voice = Voice::voices[x];
    if(voice.midiChannel == channel) {
      bool thisVoice = (voice.midiNote == note);
      if(!thisVoice && voice.midiChannel == CHANNEL_ARP) { // Check if this command is for one of the arp notes
        unsigned long ms = millis();
        for(int y=0; y<MAX_ARP_NOTES; y++)
          if(voice.arpNoteEndTimestamps[y] > ms && voice.arpNotes[y] == note) {
            thisVoice = true;
            break;
          }
      }
      if(thisVoice) {
        voice.midiVel = vel;
        break;
      }
    }
  }
}

void pitchBend(uint8_t channel, uint8_t low7, uint8_t high7) {
  int16_t pb = (int16_t)(((int16_t)high7)<<7 | low7) - 0x2000;
  for(unsigned int x=0; x<NVOICES; x++) {
    Voice::Voice &voice = Voice::voices[x];
    if(voice.midiChannel == channel)
      voice.midiPB = pb;
  }
}

void cc(uint8_t channel, uint8_t control, uint8_t value) {
  (void)channel; // Unused

  switch(control) {
    case 120: // All sound/oscillators off
    case 123:
      for(unsigned int x=0; x<NVOICES; x++)
        Voice::voices[x].active = false;
      break;
    case 121: // Reset stuff to initial values
        tremoloDepth = TREMOLO_DEPTH_DEFAULT;
        tremoloPeriod = TREMOLO_PERIOD_DEFAULT;
        tremoloDelay = TREMOLO_DELAY_DEFAULT;
        vibratoDepth = VIBRATO_DEPTH_DEFAULT;
        vibratoPeriod = VIBRATO_PERIOD_DEFAULT;
        vibratoDelay = VIBRATO_DELAY_DEFAULT;
        attack = ATTACK_DEFAULT;
        decay = DECAY_DEFAULT;
        sustain = SUSTAIN_DEFAULT;
        release = RELEASE_DEFAULT;
        arpeggioPeriod = ARPEGGIO_PERIOD_DEFAULT;

        tremoloDepthCC = TREMOLO_DEPTH_DEFAULT_CC;
        tremoloPeriodCC = TREMOLO_PERIOD_DEFAULT_CC;
        tremoloDelayCC = TREMOLO_DELAY_DEFAULT_CC;
        vibratoDepthCC = VIBRATO_DEPTH_DEFAULT_CC;
        vibratoPeriodCC = VIBRATO_PERIOD_DEFAULT_CC;
        vibratoDelayCC = VIBRATO_DELAY_DEFAULT_CC;
        attackCC = ATTACK_DEFAULT_CC;
        decayCC = DECAY_DEFAULT_CC;
        sustainCC = SUSTAIN_DEFAULT_CC;
        releaseCC = RELEASE_DEFAULT_CC;
        arpeggioPeriodCC = ARPEGGIO_PERIOD_DEFAULT_CC;
      break;
    case TREMOLO_DEPTH_CC:
      tremoloDepth = TREMOLO_DEPTH_FROM_CC(value);
      tremoloDepthCC = value;
      break;
    case TREMOLO_PERIOD_CC:
      tremoloPeriod = TREMOLO_PERIOD_FROM_CC(value);
      tremoloPeriodCC = value;
      break;
    case TREMOLO_DELAY_CC:
      tremoloDelay = TREMOLO_DELAY_FROM_CC(value);
      tremoloDelayCC = value;
      break;
    case VIBRATO_DEPTH_CC:
      vibratoDepth = VIBRATO_DEPTH_FROM_CC(value);
      vibratoDepthCC = value;
      break;
    case VIBRATO_PERIOD_CC:
      vibratoPeriod = VIBRATO_PERIOD_FROM_CC(value);
      vibratoPeriodCC = value;
      break;
    case VIBRATO_DELAY_CC:
      vibratoDelay = VIBRATO_DELAY_FROM_CC(value);
      vibratoDelayCC = value;
      break;
    case ATTACK_CC:
      attack = ADR_FROM_CC(value);
      attackCC = value;
      break;
    case DECAY_CC:
      decay = ADR_FROM_CC(value);
      decayCC = value;
      break;
    case SUSTAIN_CC:
      sustain = S_FROM_CC(value);
      sustainCC = value;
      break;
    case RELEASE_CC:
    case 64: // Sustain pedal
      release = ADR_FROM_CC(value);
      releaseCC = value;
      break;
    case ARPEGGIO_CC:
      arpeggioPeriod = ARPEGGIO_PERIOD_FROM_CC(value);
      arpeggioPeriodCC = value;
      break;
    default:
      break;
  }
}

void handleMIDI(unsigned char byte1, unsigned char byte2, unsigned char byte3) {
  unsigned char command = byte1 >> 4;
  unsigned char channel = byte1 & 0xF;

#ifdef PRINTMIDI
  SerialUSB.print("MIDI: ");
  SerialUSB.print(byte1, HEX);
  SerialUSB.print(" ");
  SerialUSB.print(byte2, HEX);
  SerialUSB.print(" ");
  SerialUSB.println(byte3, HEX);
#endif

  // Pass MIDI though to hardware port
  if(Serial.availableForWrite() >= 3) { // Rush E protection
    Serial.write(byte1);
    Serial.write(byte2);
    Serial.write(byte3);
  }

  // Ignore MIDI channels that we don't respond to
  int16_t offsetChannel = (int16_t)channel - (int16_t)MIDIbaseChannel;
  if(offsetChannel < CHANNEL_CLEAN || offsetChannel >= CHANNEL_INVALID) {
    LCD::MIDIping(-1);
    return;
  }
  LCD::MIDIping(channel);
  channel = offsetChannel;

  switch(command) {
    case 0x9:
      if(byte3 >= VEL_THRESH) noteDown(channel, byte2, byte3);
      else noteUp(channel, byte2);
      break;
      
    case 0x8:
      noteUp(channel, byte2);
      break;
      
    case 0xA:
      aftertouch(channel, byte2, byte3);
      break;

    case 0xB:
      cc(channel, byte2, byte3);
      break;

    case 0xE:
      pitchBend(channel, byte2, byte3);
      break;
  }
}

void initMIDI() {
#ifdef PRINTMIDI
  SerialUSB.begin(115200);
#endif

  // Physical MIDI interface
  Serial.begin(31250);
}

void processMIDI() {
  static midiEventPacket_t rx;
  rx = MidiUSB.read();
  if(rx.header) handleMIDI(rx.byte1, rx.byte2, rx.byte3);
  
  while(Serial.available()) {
    unsigned char d = Serial.read();
    if(d & 0x80) hwMIDIbufInd = 0;
    hwMIDIbuf[hwMIDIbufInd++] = d;
    if(hwMIDIbufInd >= 3) {
      hwMIDIbufInd = 0;
      handleMIDI(hwMIDIbuf[0], hwMIDIbuf[1], hwMIDIbuf[2]);
    }
  }
}

void checkConnected() {
  static uint16_t lastFrameNumber;
  static int8_t missedFrameCount = -1;

  // Should increment at 1ms USB frame interval
  const uint16_t frameNumber = (UOTGHS->UOTGHS_DEVFNUM & UOTGHS_DEVFNUM_FNUM_Msk) >> UOTGHS_DEVFNUM_FNUM_Pos;

  if(frameNumber == lastFrameNumber) {
    // Only count missed frames if we have received some in the past
    if(missedFrameCount >= 0)
      missedFrameCount++;
  }

  else missedFrameCount = 0;

  lastFrameNumber = frameNumber;

  // Disable oscillators if we go more than 10ms without a USB frame
  if(missedFrameCount >= 10) {
    missedFrameCount = -1;
    for(unsigned int x=0; x<NVOICES; x++)
        Voice::voices[x].active = false;
  }
}

}
