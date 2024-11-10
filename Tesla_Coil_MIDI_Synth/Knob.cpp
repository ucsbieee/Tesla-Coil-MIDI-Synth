#include "Knob.h"
#include "Synth.h"
#include "LCD.h"
#include "MIDI.h"
#include "Audio.h"

#include <Arduino.h>

// Encoder tracking and acceleration
int8_t Knob::movement = 0;
int32_t Knob::recentMovement = 0;
unsigned long Knob::lastDecrement = 0;

// Encoder LUT (stolen from https://electronics.stackexchange.com/questions/360637/quadrature-encoder-most-efficient-software-implementation)
const int8_t Knob::encLut[32] = {0,-1,+1,+2,+1,0,+2,-1,-1,+2,0,+1,+2,+1,-1,0,0,-1,+1,-2,+1,0,-2,-1,-1,-2,0,+1,-2,+1,-1,0};
uint8_t Knob::encDir = 0, Knob::encLutInd = 0;

// Cursed reference to the latest instantiation of this class for the static interrupt handler
// (class is for sematic purposes only; instantiating multiple is pointless)
Knob *Knob::knob;

Knob::Knob(Synth &synth, LCD &lcd, MIDI &midi, Audio &audio): synth(synth), lcd(lcd), midi(midi), audio(audio) {
  knob = this;
}

void Knob::pollButton() {
  unsigned long ms = millis();
  bool buttonState = digitalRead(22);
  if(!buttonState) {
    if(lastButtonState && ms - lastButton >= BUTTON_DEBOUNCE) lcd.editing = !lcd.editing;
    lastButton = ms;
  }
}

void Knob::updateEncoder() {
  unsigned long ms = millis();
  while(ms - lastDecrement > ENCODER_ACCELERATION) {
    lastDecrement += ENCODER_ACCELERATION;
    if(recentMovement > 0) recentMovement--;
    else if(recentMovement < 0) recentMovement++;
  }
}

// Called when encoder moves
void Knob::enc() {
  Knob &k = *knob;
  
  encLutInd |= (!digitalRead(64))<<1 | !digitalRead(65); // Add current state to index
  movement += encLut[encLutInd]; // Increment position
  if(encLut[encLutInd]) encDir = (encLut[encLutInd] > 0) ? 1:0; // Update instantaneous direction
  encLutInd = ((encLutInd<<2)&0b1100) | (encDir<<4); // Shift bits and stuff for next iteration
  
  if(abs(movement) >= 4) { // Four pulses per tick
    recentMovement += movement/4;
    movement = 0;
    
    int32_t change = 0;
    if(recentMovement > 0) change = recentMovement*recentMovement;
    else change = -recentMovement*recentMovement;

    if(k.lcd.editing) { // Change value
      const auto &screen = k.lcd.screens[k.lcd.LCDstate];
      if(screen.dataCC) {
        change += (int32_t)*screen.dataCC;
        if(change < 0) change = 0;
        else if(change > 127) change = 127;
        *screen.dataCC = change;
        k.midi.handleMIDI(0xB0 | k.midi.MIDIbaseChannel, screen.cc, change);
      } else {
        switch(k.lcd.LCDstate) {
          case LCD::SCREEN_PULSE_WIDTH:
            change += (int32_t)k.synth.vol;
            if(change < 0) change = 0;
            else if(change > 255) change = 255;
            k.synth.vol = change;
            k.audio.pulseWidthMax = change * ((uint32_t)(NOM_SAMPLE_RATE*MAX_WIDTH)) / 0x100;
            k.audio.pwmWidthMax = change * (F_CPU/NOM_SAMPLE_RATE) / 0x100;
            break;
          case LCD::SCREEN_MIDI_MIN_NOTE:
            change += (int32_t)k.midi.minNote;
            if(change < 0) change = 0;
            else if(change > k.midi.maxNote) change = k.midi.maxNote;
            k.midi.minNote = change;
            break;
          case LCD::SCREEN_MIDI_MAX_NOTE:
            change += (int32_t)k.midi.maxNote;
            if(change < k.midi.minNote) change = k.midi.minNote;
            else if(change > MIDI_MAX_NOTE) change = MIDI_MAX_NOTE;
            k.midi.maxNote = change;
            break;
          case LCD::SCREEN_MIDI_BASE:
            change += (int32_t)k.midi.MIDIbaseChannel;
            if(change < 0) change = 0;
            else if(change > 15) change = 15;
            k.midi.MIDIbaseChannel = change;
          case LCD::SCREEN_AUDIO_MODE:
            change += (int32_t)k.audio.audioMode;
            if(change < 0) change = Audio::AM_END-1;
            else if(change >= Audio::AM_END) change = Audio::AM_OFF;
            k.audio.audioMode = (Audio::AudioMode_e)change;
            k.audio.processors[change]->reset();
          case LCD::SCREEN_AUDIO_GAIN:
          case LCD::SCREEN_AUDIO_NOISE_GATE:
            change += *(uint8_t*)screen.dataValue;
            if(change > 0xFF) change = 0xFF;
            else if(change < 0) change = 0;
            *(uint8_t*)screen.dataValue = change;
            break;
          default:
            break;
        }
      }
    } else { // Change screen
      change += (int32_t)k.lcd.LCDstate;
      if(change >= LCD::SCREEN_END) change = change % LCD::SCREEN_END;
      else while(change < 0) change += LCD::SCREEN_END;
      k.lcd.LCDstate = (LCD::LCDScreen)change;
    }
  }
  
  k.updateEncoder();
}

void Knob::update() {
  updateEncoder();
  pollButton();
}

void Knob::init() {
  pinMode(22, INPUT_PULLUP);
  pinMode(64, INPUT_PULLUP);
  pinMode(65, INPUT_PULLUP);
  attachInterrupt(64, enc, CHANGE);
  attachInterrupt(65, enc, CHANGE);
}
