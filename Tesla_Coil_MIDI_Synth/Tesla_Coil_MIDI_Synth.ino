/* 
 * Standard MIDI things
    * Pitch bend
    * Aftertouch
 * Effects
    * Tremolo (volume) (rate = 102, depth = 92, delay = 103)
    * Vibrato (pitch) (rate = 76, depth = 77, delay = 78)
    * ADSR (116, 117, 118, 119)
    * Arpeggio rate (14)
 * Channels
    * 1: Clean (no effects)
    * 2: Effects
    * 3: Arpeggio and effects
    * 4: Drums: kick (36/C2), snare (40/E2), clap (39/D#2), tom (48/C3), and hi-hat (42/F#2)
 */

#include "MIDI.h"
#include "Synth.h"
#include "Voice.h"
#include "Audio.h"
#include "LCD.h"
#include "Knob.h"
#include "Connected.h"

void setup() {
  MIDI::initMIDI();
  Synth::initSynth();
  Voice::initVoices();
  Audio::initAudio();
  LCD::initLCD();
  Knob::initEncoder();
}

void loop() {
  MIDI::processMIDI();
  Audio::processAudio();
  LCD::updateLCD();
  Knob::updateKnob();
}

extern "C" {
int sysTickHook() { // Runs at 1kHz
  Connected::checkConnected();
  Synth::updateSynth();
  return 0;
}
}
