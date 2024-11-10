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
  void updateEncoder();
  
  // Encoder tracking and acceleration
  static int8_t movement;
  static int32_t recentMovement;
  static unsigned long lastDecrement;
  
  // Encoder LUT
  static const int8_t encLut[32];
  static uint8_t encDir, encLutInd;
  
  // Static callback function
  // plus cursed static reference to the latest instantiation of this class for the static function to use
  static void enc();
  static Knob *knob;
};
