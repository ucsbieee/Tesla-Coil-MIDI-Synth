#pragma once

#include <Arduino.h>
#include <LiquidCrystal.h>

#ifdef SAVE_BASE_MIDI
#include <DueFlashStorage.h>
#endif

#include <inttypes.h>

#define LCD_UPDATE_PERIOD 30 // ms
#define MIDI_PING_LINGER 3 // frames
#define SCREEN_TIMEOUT (10000/LCD_UPDATE_PERIOD) // frames

class Synth;
class MIDI;
class Audio;

class LCD {
public:
  LCD(Synth &synth, MIDI &midi, Audio &audio);

  enum LCDScreen {
    SCREEN_PULSE_WIDTH,
    SCREEN_A,
    SCREEN_D,
    SCREEN_S,
    SCREEN_R,
    SCREEN_TREM_DEPTH,
    SCREEN_TREM_PERIOD,
    SCREEN_TREM_DELAY,
    SCREEN_VIBR_DEPTH,
    SCREEN_VIBR_PERIOD,
    SCREEN_VIBR_DELAY,
    SCREEN_ARP_PERIOD,
    SCREEN_MIDI_MIN_NOTE,
    SCREEN_MIDI_MAX_NOTE,
    SCREEN_MIDI_BASE,
    SCREEN_AUDIO_MODE,
    SCREEN_END
  };
  
  LCDScreen LCDstate = SCREEN_PULSE_WIDTH;
  bool editing = false;

  struct LCDScreenDescriptor {
    // Text to display
    const char *title;
    const char *units;
    
    // Function to convert to displayed value
    uint16_t (*convert)(void*);
    
    // Current value
    void *dataValue;
    
    // Current value (CC)
    uint8_t *dataCC;
    
    // MIDI CC number
    uint8_t cc;
    
  };
  
  const LCDScreenDescriptor screens[SCREEN_END];
  
  // Note name sequence
  static const char *noteNames[12];
  
  // Functions
  void init();
  void update();
  void MIDIping(int8_t c);

private:
  Synth &synth;
  MIDI &midi;
  Audio &audio;
  
  LiquidCrystal lcd;
  
#ifdef SAVE_BASE_MIDI
  DueFlashStorage DFS;
#endif

  unsigned long LCDframe = 0;
  unsigned long lastLCDframe = 0;
  
  LCDScreen lastLCDstate = SCREEN_END;
  bool lastEditing = true;
  uint16_t displayedValue, lastDisplayedValue = 0xFFFF;
  uint8_t volumeBar[16] = {0}, lastVolumeBar[16] = {0};
  unsigned long lastMIDIping = -MIDI_PING_LINGER;
  int8_t MIDIpingChan = -2, lastMIDIpingChan = MIDIpingChan;
  
  unsigned long framesSinceLastInput = 0;
  
  bool showingSplashScreen = true;
  
  // Functions to display value
  static uint16_t displayTypeUINT32(void *data);
  static uint16_t displayTypeUINT8scaled(void *data);
  static uint16_t displayTypeMIDIchannel(void *data);
  static uint16_t displayTypeUINT32Hz(void *data);
};
