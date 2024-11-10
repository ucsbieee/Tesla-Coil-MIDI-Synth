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

#include "Voice.h"
#include "Synth.h"
#include "LCD.h"
#include "MIDI.h"
#include "Audio.h"
#include "Knob.h"
#include "Connected.h"

extern Voices    voices;
extern Synth     synth;
extern LCD       lcd;
extern MIDI      midi;
extern Audio     audio;
extern Knob      knob;
extern Connected conn;

Voices    voices;
Synth     synth (voices,             midi       );
LCD       lcd   (        synth,      midi, audio);
MIDI      midi  (voices, synth, lcd             );
Audio     audio (        synth                  );
Knob      knob  (        synth, lcd, midi, audio);
Connected conn  (        synth,            audio);

void setup() {
  midi.init();
  voices.init();
  audio.init();
  lcd.init();
  knob.init();
}

void loop() {
  midi.process();
  audio.process();
  lcd.update();
  knob.update();
}

// Must be in main file to override default handler
void PWM_Handler() {
  audio.setDMABuffer();
}

extern "C" {
int sysTickHook() { // Runs at 1kHz
  conn.checkConnected();
  synth.update();
  return 0;
}
}
