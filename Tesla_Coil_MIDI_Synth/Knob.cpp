#include "Knob.h"
#include "LCD.h"
#include "MIDI.h"

#include <Arduino.h>

namespace Knob {

// Button
unsigned long lastButton = 0;
bool lastButtonState = true;

// Encoder tracking and acceleration
int8_t movement = 0;
int32_t recentMovement = 0;
unsigned long lastDecrement = 0;

// Encoder LUT (stolen from https://electronics.stackexchange.com/questions/360637/quadrature-encoder-most-efficient-software-implementation)
const int8_t encLut[32] = {0,-1,+1,+2,+1,0,+2,-1,-1,+2,0,+1,+2,+1,-1,0,0,-1,+1,-2,+1,0,-2,-1,-1,-2,0,+1,-2,+1,-1,0};
uint8_t encDir = 0, encLutInd = 0;

void pollButton() {
  unsigned long ms = millis();
  bool buttonState = digitalRead(22);
  if(!buttonState) {
    if(lastButtonState && ms - lastButton >= BUTTON_DEBOUNCE) LCD::editing = !LCD::editing;
    lastButton = ms;
  }
}

void updateEncoder() {
  unsigned long ms = millis();
  while(ms - lastDecrement > ENCODER_ACCELERATION) {
    lastDecrement += ENCODER_ACCELERATION;
    if(recentMovement > 0) recentMovement--;
    else if(recentMovement < 0) recentMovement++;
  }
}

// Called when encoder moves
void enc() {
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

    if(LCD::editing) { // Change value
      const LCD::LCD_screen_descriptor &screen = LCD::screens[LCD::LCDstate];
      if(screen.dataCC) {
        change += (int32_t)*screen.dataCC;
        if(change < 0) change = 0;
        else if(change > 127) change = 127;
        *screen.dataCC = change;
        MIDI::handleMIDI(0xB0 | MIDI::MIDIbaseChannel, screen.cc, change);
      } else {
        switch(LCD::LCDstate) {
          case LCD::SCREEN_PULSE_WIDTH:
            change += (int32_t)Synth::vol;
            if(change < 0) change = 0;
            else if(change > 255) change = 255;
            Synth::vol = change;
            break;
          case LCD::SCREEN_MIDI_MIN_NOTE:
            change += (int32_t)MIDI::minNote;
            if(change < 0) change = 0;
            else if(change > MIDI::maxNote) change = MIDI::maxNote;
            MIDI::minNote = change;
            break;
          case LCD::SCREEN_MIDI_MAX_NOTE:
            change += (int32_t)MIDI::maxNote;
            if(change < MIDI::minNote) change = MIDI::minNote;
            else if(change > MIDI_MAX_NOTE) change = MIDI_MAX_NOTE;
            MIDI::maxNote = change;
            break;
          case LCD::SCREEN_MIDI_BASE:
            change += (int32_t)MIDI::MIDIbaseChannel;
            if(change < 0) change = 0;
            else if(change > 15) change = 15;
            MIDI::MIDIbaseChannel = change;
          default:
            break;
        }
      }
    } else { // Change screen
      change += (int32_t)LCD::LCDstate;
      if(change >= LCD::SCREEN_END) change = change % LCD::SCREEN_END;
      else while(change < 0) change += LCD::SCREEN_END;
      LCD::LCDstate = change;
    }
  }
  
  updateEncoder();
}

void updateKnob() {
  updateEncoder();
  pollButton();
}

void initEncoder() {
  pinMode(22, INPUT_PULLUP);
  pinMode(64, INPUT_PULLUP);
  pinMode(65, INPUT_PULLUP);
  attachInterrupt(64, enc, CHANGE);
  attachInterrupt(65, enc, CHANGE);
}

}
