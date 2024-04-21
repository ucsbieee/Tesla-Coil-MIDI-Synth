#include "LCD.h"
#include "BarChars.h"
#include "Knob.h"
#include "MIDI.h"

#include <Arduino.h>
#include <LiquidCrystal.h>

#ifdef SAVE_BASE_MIDI
#include <DueFlashStorage.h>
#endif

namespace LCD {

LiquidCrystal lcd(19, 18, 17, 16, 23, 24);

#ifdef SAVE_BASE_MIDI
DueFlashStorage DFS;
#endif

const LCD_screen_descriptor screens[] = {
  { // SCREEN_PULSE_WIDTH
    "P Width: ",
    "%",
    NULL,
    NULL, // Special
    NULL,
    0
  },
  { // SCREEN_A
    "Attack",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::attack,
    &MIDI::attackCC,
    ATTACK_CC
  },
  { // SCREEN_D
    "Decay",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::decay,
    &MIDI::decayCC,
    DECAY_CC
  },
  { // SCREEN_S
    "Sustain",
    "%",
    &displayTypeUINT8scaled,
    (void*)&MIDI::sustain,
    &MIDI::sustainCC,
    SUSTAIN_CC
  },
  { // SCREEN_R
    "Release",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::release,
    &MIDI::releaseCC,
    RELEASE_CC
  },
  { // SCREEN_TREM_DEPTH
    "Trem Depth",
    "%",
    &displayTypeUINT8scaled,
    (void*)&MIDI::tremoloDepth,
    &MIDI::tremoloDepthCC,
    TREMOLO_DEPTH_CC
  },
  { // SCREEN_TREM_PERIOD
    "Trem Speed",
    "Hz",
    &displayTypeUINT32Hz,
    (void*)&MIDI::tremoloPeriod,
    &MIDI::tremoloPeriodCC,
    TREMOLO_PERIOD_CC
  },
  { // SCREEN_TREM_DELAY
    "Trem Delay",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::tremoloDelay,
    &MIDI::tremoloDelayCC,
    TREMOLO_DELAY_CC
  },
  { // SCREEN_VIBR_DEPTH
    "Vibr Depth",
    "%",
    &displayTypeUINT8scaled,
    (void*)&MIDI::vibratoDepth,
    &MIDI::vibratoDepthCC,
    VIBRATO_DEPTH_CC
  },
  { // SCREEN_VIBR_PERIOD
    "Vibr Speed",
    "Hz",
    &displayTypeUINT32Hz,
    (void*)&MIDI::vibratoPeriod,
    &MIDI::vibratoPeriodCC,
    VIBRATO_PERIOD_CC
  },
  { // SCREEN_VIBR_DELAY
    "Vibr Delay",
    "ms",
    &displayTypeUINT32,
    (void*)&MIDI::vibratoDelay,
    &MIDI::vibratoDelayCC,
    VIBRATO_DELAY_CC
  },
  { // SCREEN_ARP_PERIOD
    "Arp Speed",
    "Hz",
    &displayTypeUINT32Hz,
    (void*)&MIDI::arpeggioPeriod,
    &MIDI::arpeggioPeriodCC,
    ARPEGGIO_CC
  },
  { // SCREEN_MIDI_MIN_NOTE
    "Min Note",
    "",
    NULL,
    NULL,
    NULL,
    0
  },
  { // SCREEN_MIDI_MAX_NOTE
    "Max Note",
    "",
    NULL,
    NULL,
    NULL,
    0
  },
  { // SCREEN_MIDI_BASE
    "MIDI Base CH",
    "",
    &displayTypeMIDIchannel,
    &MIDI::MIDIbaseChannel,
    NULL,
    0
  }
};

// Note name sequence
const char *noteNames[] = {
  "C",
  "C#",
  "D",
  "D#",
  "E",
  "F",
  "F#",
  "G",
  "G#",
  "A",
  "A#",
  "B",
};

unsigned long LCDframe = 0;
unsigned long lastLCDframe = 0;
unsigned long lastMIDIping = -MIDI_PING_LINGER;
int8_t MIDIpingChan = -2, lastMIDIpingChan = MIDIpingChan;

uint8_t LCDstate = SCREEN_PULSE_WIDTH, lastLCDstate = 0xFF;
uint16_t displayedValue = Synth::vol, lastDisplayedValue = 0xFFFF;
bool editing = false, lastEditing = true;
uint8_t volumeBar[16], lastVolumeBar[16];

unsigned long framesSinceLastInput = 0;

void initLCD() {
  lcd.begin(16, 2);

  for(uint8_t x=0; x<NBARCHARS; x++) {
    uint8_t temp[8];
    memcpy(temp, BarChars[x].data, 8); // Un-const
    lcd.createChar(BarChars[x].value, temp);
  }

  memset(volumeBar, BAR_EMPTY, 16);
  memset(lastVolumeBar, BAR_EMPTY, 16);

#ifdef SAVE_BASE_MIDI
  MIDI::MIDIbaseChannel = DFS.read(0); // Store base channel in flash (may cause issues if program space over 50% full?)
  if(MIDI::MIDIbaseChannel > 15) {
    MIDI::MIDIbaseChannel = 0;
    DFS.write(0, 0);
  }
#endif
}

void updateLCD() {
  unsigned long ms = millis();
  if(ms-lastLCDframe < LCD_UPDATE_PERIOD) return;
  lastLCDframe = ms;

  const LCD_screen_descriptor &screen = screens[LCDstate];

  // Print title
  if(LCDstate != lastLCDstate) {
    lcd.setCursor(0, 0);
    lcd.print("              ");
    lcd.setCursor(0, 0);
    lcd.print(screen.title);
  }

  switch(LCDstate) {
    case SCREEN_PULSE_WIDTH: {
        // Print pulse width value
        displayedValue = displayTypeUINT8scaled((void*)&Synth::vol);
        if(LCDstate != lastLCDstate || displayedValue != lastDisplayedValue || editing != lastEditing) {
          char buf[17];
          snprintf(buf, 17, "%s%i%s                ", editing ? ">" : "", displayedValue, screen.units);
          uint8_t titleLen = strlen(screen.title);
          buf[14-titleLen] = 0; // Only print 14 chars
          lcd.setCursor(titleLen, 0);
          lcd.print(buf);
        }
  
        // Draw volume bar
        uint16_t volume = analogRead(6)*30/1023;
        volumeBar[0] = volume > 0 ? BAR_LEFT_FULL : BAR_LEFT_EMPTY;
        volumeBar[15] = volume >= 30 ? BAR_RIGHT_FULL : BAR_RIGHT_EMPTY;
        for(uint8_t x=1; x<15; x++) {
          if(volume >= x*2+1) volumeBar[x] = BAR_FULL;
          else if(volume >= x*2) volumeBar[x] = BAR_HALF_FULL;
          else volumeBar[x] = BAR_EMPTY;
        }
        for(uint8_t x=0; x<16; x++)
          if(volumeBar[x] != lastVolumeBar[x] || LCDstate != lastLCDstate) {
            lcd.setCursor(x, 1);
            lcd.write(volumeBar[x]);
            lastVolumeBar[x] = volumeBar[x];
          }
      }
      break;
    case SCREEN_MIDI_MIN_NOTE:
      displayedValue = MIDI::minNote;
      goto SCREEN_MIDI_NOTE;
    case SCREEN_MIDI_MAX_NOTE:
      displayedValue = MIDI::maxNote;
    SCREEN_MIDI_NOTE:
      if(LCDstate != lastLCDstate || displayedValue != lastDisplayedValue || editing != lastEditing) {
        char buf[17];
        snprintf(buf, 17, "%c%s%i                ", editing ? '>' : ' ', noteNames[displayedValue%12], displayedValue/12-1);
        lcd.setCursor(0, 1);
        lcd.print(buf);
      }
      break;
#ifdef SAVE_BASE_MIDI
    case SCREEN_MIDI_BASE:
      if(!editing && lastEditing) DFS.write(0, MIDI::MIDIbaseChannel);
#endif
    default:
      // Print value
      displayedValue = screen.convert(screen.dataValue);
      if(LCDstate != lastLCDstate || displayedValue != lastDisplayedValue || editing != lastEditing) {
        char buf[17];
        if(screen.convert == &displayTypeUINT32Hz) // Show decimal places
          snprintf(buf, 17, "%c%i.%.2i%s                ", editing ? '>' : ' ', displayedValue/100, displayedValue%100, screen.units);
        else
          snprintf(buf, 17, "%c%i%s                ", editing ? '>' : ' ', displayedValue, screen.units);
        lcd.setCursor(0, 1);
        lcd.print(buf);
      }
      break;
  }

  // Display when we receive MIDI data
  if(LCDframe-lastMIDIping > MIDI_PING_LINGER) MIDIpingChan = -2;
  if(MIDIpingChan != lastMIDIpingChan) {
    lcd.setCursor(14, 0);
    if(MIDIpingChan < -1) lcd.print("  ");
    else if(MIDIpingChan == -1) lcd.print(" *");
    else {
      if(MIDIpingChan < 10) lcd.print(' ');
      lcd.print(MIDIpingChan + 1);
    }
    lastMIDIpingChan = MIDIpingChan;
  }

  if(displayedValue != lastDisplayedValue || LCDstate != lastLCDstate)
    framesSinceLastInput = 0;

  lastLCDstate = LCDstate;
  lastDisplayedValue = displayedValue;
  lastEditing = editing;

  if(!editing) {
    framesSinceLastInput++;
    if(framesSinceLastInput > SCREEN_TIMEOUT) {
      framesSinceLastInput = 0;
      LCDstate = SCREEN_PULSE_WIDTH;
    }
  } else framesSinceLastInput = 0;
  
  LCDframe++;
}

void MIDIping(int8_t c) {
  // Don't cover display of playing channels with non-playing channels
  if(c < 0 && MIDIpingChan >= 0) return;
  lastMIDIping = LCDframe;
  MIDIpingChan = c;
}

uint16_t displayTypeUINT32(void *data) {
  return *(uint32_t*)data;
}

uint16_t displayTypeUINT8scaled(void *data) {
  return ((uint16_t)*(uint8_t*)data)*100/255;
}

uint16_t displayTypeMIDIchannel(void *data) {
  return *(uint8_t*)data+1;
}

uint16_t displayTypeUINT32Hz(void *data) {
  return 100000 / *(uint32_t*)data;
}

}
