#pragma once

#include <inttypes.h>

#define BUTTON_DEBOUNCE 100 // ms
#define ENCODER_ACCELERATION 60 // ms per decrement

class Synth;
class LCD;
class MIDI;
class Audio;

class Knob {
public:
  Knob(Synth &synth, LCD &lcd, MIDI &midi, Audio &audio);
  
  void init();
  void update();
  
private:
  Synth &synth;
  LCD &lcd;
  MIDI &midi;
  Audio &audio;
  
  // Button
  unsigned long lastButton = 0;
  bool lastButtonState = true;
  
  void pollButton();
  void updateEncoderAcceleration();
  void applyChange();
  
  // Encoder tracking and acceleration
  static int8_t movement;
  static int32_t recentMovement;
  static unsigned long lastDecrement;
  static int32_t globalPos;
  int32_t localPos;
  
  // Encoder LUT
  static const int8_t encLut[32];
  static uint8_t encDir, encLutInd;
  
  // Static interrupt handler
  static void enc();
};
