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
int32_t Knob::globalPos = 0;

// Encoder LUT (stolen from https://electronics.stackexchange.com/questions/360637/quadrature-encoder-most-efficient-software-implementation)
const int8_t Knob::encLut[32] = {0,-1,+1,+2,+1,0,+2,-1,-1,+2,0,+1,+2,+1,-1,0,0,-1,+1,-2,+1,0,-2,-1,-1,-2,0,+1,-2,+1,-1,0};
uint8_t Knob::encDir = 0, Knob::encLutInd = 0;

Knob::Knob(Synth &synth, LCD &lcd, MIDI &midi, Audio &audio): synth(synth), lcd(lcd), midi(midi), audio(audio) {}

void Knob::pollButton() {
  unsigned long ms = millis();
  bool buttonState = digitalRead(22);
  if(!buttonState) {
    if(lastButtonState && ms - lastButton >= BUTTON_DEBOUNCE) lcd.editing = !lcd.editing;
    lastButton = ms;
  }
}

void Knob::updateEncoderAcceleration() {
  unsigned long ms = millis();
  while(ms - lastDecrement > ENCODER_ACCELERATION) {
    lastDecrement += ENCODER_ACCELERATION;
    if(recentMovement > 0) recentMovement--;
    else if(recentMovement < 0) recentMovement++;
  }
}

// Called when encoder moves (interrupt)
void Knob::enc() {
  encLutInd |= (!digitalRead(64))<<1 | !digitalRead(65); // Add current state to index
  movement += encLut[encLutInd]; // Increment position
  if(encLut[encLutInd]) encDir = (encLut[encLutInd] > 0) ? 1:0; // Update instantaneous direction
  encLutInd = ((encLutInd<<2)&0b1100) | (encDir<<4); // Shift bits and stuff for next iteration
  
  if(abs(movement) >= 4) { // Four pulses per tick
    recentMovement += movement/4;
    movement = 0;
    
    if(recentMovement > 0) globalPos += recentMovement*recentMovement;
    else globalPos -= recentMovement*recentMovement;
  }
}

// Update values based on movement
void Knob::applyChange() {
  int32_t change = globalPos - localPos;
  localPos = globalPos;

  if(change) {
    if(lcd.editing) { // Change value
      const auto &screen = lcd.screens[lcd.LCDstate];
      if(screen.dataCC) {
        change += (int32_t)*screen.dataCC;
        if(change < 0) change = 0;
        else if(change > 127) change = 127;
        *screen.dataCC = change;
        midi.handleMIDI(0xB0 | midi.MIDIbaseChannel, screen.cc, change);
      } else {
        switch(lcd.LCDstate) {
          case LCD::SCREEN_PULSE_WIDTH:
            change += (int32_t)synth.vol;
            if(change < 0) change = 0;
            else if(change > 255) change = 255;
            synth.vol = change;
            audio.pulseWidthMax = change * ((uint32_t)(NOM_SAMPLE_RATE*MAX_WIDTH)) / 0x100;
            audio.pwmWidthMax = change * (F_CPU/NOM_SAMPLE_RATE) / 0x100;
            break;
          case LCD::SCREEN_MIDI_MIN_NOTE:
            change += (int32_t)midi.minNote;
            if(change < 0) change = 0;
            else if(change > midi.maxNote) change = midi.maxNote;
            midi.minNote = change;
            break;
          case LCD::SCREEN_MIDI_MAX_NOTE:
            change += (int32_t)midi.maxNote;
            if(change < midi.minNote) change = midi.minNote;
            else if(change > MIDI_MAX_NOTE) change = MIDI_MAX_NOTE;
            midi.maxNote = change;
            break;
          case LCD::SCREEN_MIDI_BASE:
            change += (int32_t)midi.MIDIbaseChannel;
            if(change < 0) change = 0;
            else if(change > 15) change = 15;
            midi.MIDIbaseChannel = change;
            break;
          case LCD::SCREEN_AUDIO_MODE:
            change += (int32_t)audio.audioMode;
            if(change < 0) change = Audio::AM_END-1;
            else if(change >= Audio::AM_END) change = Audio::AM_OFF;
            audio.audioMode = (Audio::AudioMode_e)change;
            audio.processors[change]->reset();
            break;
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
      change += (int32_t)lcd.LCDstate;
      if(change >= LCD::SCREEN_END) change = change % LCD::SCREEN_END;
      else while(change < 0) change += LCD::SCREEN_END;
      lcd.LCDstate = (LCD::LCDScreen)change;
    }
  }
}

void Knob::update() {
  updateEncoderAcceleration();
  pollButton();
  applyChange();
}

void Knob::init() {
  pinMode(22, INPUT_PULLUP);
  pinMode(64, INPUT_PULLUP);
  pinMode(65, INPUT_PULLUP);
  attachInterrupt(64, enc, CHANGE);
  attachInterrupt(65, enc, CHANGE);
}
