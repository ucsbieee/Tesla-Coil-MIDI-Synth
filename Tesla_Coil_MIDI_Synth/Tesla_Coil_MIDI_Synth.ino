/* 
 * Standard MIDI things
    * Pitch bend
    * Aftertouch
 * Effects
    * Tremolo (volume) (rate = 102, depth = 92, delay = 103)
    * Vibrato (pitch) (rate = 76, depth = 77, delay = 78)
    * ADSR (116, 117, 118, 119)
    * Over/undertones (1/16 = 20, 1/8 = 21, 1/4 = 22, 1/2 = 23, 2 = 24, 3 = 25, 4 = 26, 5 = 27)
 * Channels
    * 1: Clean (no effects)
    * 2: Effects
    * 3: Arpeggio and effects (speed = 14)
    * 4: Drums: kick (36/C2), snare (40/E2), clap (39/D#2), tom (48/C3), and hi-hat (42/F#2)
 */

#include "MIDI.h"
#include "Synth.h"
#include "Voice.h"
#include "LCD.h"
#include "Knob.h"

void setup() {
  MIDI::initMIDI();
  Synth::initSynth();
  Voice::initVoices();
  LCD::initLCD();
  Knob::initEncoder();
}

void loop() {
  MIDI::processMIDI();
  LCD::updateLCD();
  Knob::updateKnob();
}

extern "C" {
int sysTickHook() { // Runs at 1kHz
  MIDI::checkConnected();
  Synth::updateSynth();
  return 0;
}
}
