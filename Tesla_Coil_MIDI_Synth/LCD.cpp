#include "LCD.h"
#include "BarChars.h"
#include "Synth.h"
#include "MIDI.h"
#include "Audio.h"
#include "Version.h"

// Note name sequence
const char *LCD::noteNames[] = {
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

LCD::LCD(Synth &synth, MIDI &midi, Audio &audio): screens{
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
    (void*)&midi.attack,
    &midi.attackCC,
    ATTACK_CC
  },
  { // SCREEN_D
    "Decay",
    "ms",
    &displayTypeUINT32,
    (void*)&midi.decay,
    &midi.decayCC,
    DECAY_CC
  },
  { // SCREEN_S
    "Sustain",
    "%",
    &displayTypeUINT8scaled,
    (void*)&midi.sustain,
    &midi.sustainCC,
    SUSTAIN_CC
  },
  { // SCREEN_R
    "Release",
    "ms",
    &displayTypeUINT32,
    (void*)&midi.release,
    &midi.releaseCC,
    RELEASE_CC
  },
  { // SCREEN_TREM_DEPTH
    "Trem Depth",
    "%",
    &displayTypeUINT8scaled,
    (void*)&midi.tremoloDepth,
    &midi.tremoloDepthCC,
    TREMOLO_DEPTH_CC
  },
  { // SCREEN_TREM_PERIOD
    "Trem Speed",
    "Hz",
    &displayTypeUINT32Hz,
    (void*)&midi.tremoloPeriod,
    &midi.tremoloPeriodCC,
    TREMOLO_PERIOD_CC
  },
  { // SCREEN_TREM_DELAY
    "Trem Delay",
    "ms",
    &displayTypeUINT32,
    (void*)&midi.tremoloDelay,
    &midi.tremoloDelayCC,
    TREMOLO_DELAY_CC
  },
  { // SCREEN_VIBR_DEPTH
    "Vibr Depth",
    "%",
    &displayTypeUINT8scaled,
    (void*)&midi.vibratoDepth,
    &midi.vibratoDepthCC,
    VIBRATO_DEPTH_CC
  },
  { // SCREEN_VIBR_PERIOD
    "Vibr Speed",
    "Hz",
    &displayTypeUINT32Hz,
    (void*)&midi.vibratoPeriod,
    &midi.vibratoPeriodCC,
    VIBRATO_PERIOD_CC
  },
  { // SCREEN_VIBR_DELAY
    "Vibr Delay",
    "ms",
    &displayTypeUINT32,
    (void*)&midi.vibratoDelay,
    &midi.vibratoDelayCC,
    VIBRATO_DELAY_CC
  },
  { // SCREEN_ARP_PERIOD
    "Arp Speed",
    "Hz",
    &displayTypeUINT32Hz,
    (void*)&midi.arpeggioPeriod,
    &midi.arpeggioPeriodCC,
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
    &midi.MIDIbaseChannel,
    NULL,
    0
  },
  { // SCREEN_AUDIO_MODE
    "Audio Mode",
    "",
    NULL,
    NULL,
    NULL,
    0
  },
  { // SCREEN_AUDIO_GAIN
    "Audio Gain",
    "%",
    &displayTypeUINT8scaled4x,
    &audio.audioGain,
    NULL,
    0
  },
  { // SCREEN_AUDIO_NOISE_GATE
    "Noise Gate",
    "%",
    &displayTypeUINT8scaled,
    &audio.audioNoiseGate,
    NULL,
    0
  }
}, synth(synth), midi(midi), audio(audio), lcd(19, 18, 17, 16, 23, 24), displayedValue(synth.vol) {}

void LCD::init() {
  lcd.begin(16, 2);

  for(uint8_t x=0; x<NBARCHARS; x++) {
    uint8_t temp[8];
    memcpy(temp, BarChars[x].data, 8); // Un-const
    lcd.createChar(BarChars[x].value, temp);
  }

  memset(volumeBar, BAR_EMPTY, 16);
  memset(lastVolumeBar, BAR_EMPTY, 16);

#ifdef SAVE_BASE_MIDI
  midi.MIDIbaseChannel = DFS.read(0); // Store base channel in flash (may cause issues if program space over 50% full?)
  if(midi.MIDIbaseChannel > 15) {
    midi.MIDIbaseChannel = 0;
    DFS.write(0, 0);
  }
#endif

  // Show firmware version
  lcd.setCursor(0, 0);
  lcd.print("FW " VERSION
#ifndef RELEASE_BUILD
  "-dev"
#endif
  );
  lcd.setCursor(0, 1);
  lcd.print(__DATE__);
}

void LCD::update() {
  unsigned long ms = millis();
  if(ms-lastLCDframe < LCD_UPDATE_PERIOD) return;
  lastLCDframe = ms;

  if(showingSplashScreen) {
    if(ms > 1000) {
      showingSplashScreen = false;
      lcd.clear();
    } else return;
  }

  const auto &screen = screens[LCDstate];

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
        displayedValue = displayTypeUINT8scaled((void*)&synth.vol);
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
      displayedValue = midi.minNote;
      goto SCREEN_MIDI_NOTE;
    case SCREEN_MIDI_MAX_NOTE:
      displayedValue = midi.maxNote;
    SCREEN_MIDI_NOTE:
      if(LCDstate != lastLCDstate || displayedValue != lastDisplayedValue || editing != lastEditing) {
        char buf[17];
        snprintf(buf, 17, "%c%s%i                ", editing ? '>' : ' ', noteNames[displayedValue%12], displayedValue/12-1);
        lcd.setCursor(0, 1);
        lcd.print(buf);
      }
      break;
    case SCREEN_AUDIO_MODE:
      displayedValue = (uint16_t)audio.audioMode;
      if(LCDstate != lastLCDstate || displayedValue != lastDisplayedValue || editing != lastEditing) {
        char buf[17];
        snprintf(buf, 17, "%c%s                ", editing ? '>' : ' ', audio.processors[audio.audioMode]->name());
        lcd.setCursor(0, 1);
        lcd.print(buf);
      }
      break;
#ifdef SAVE_BASE_MIDI
    case SCREEN_MIDI_BASE:
      if(!editing && lastEditing) DFS.write(0, midi.MIDIbaseChannel);
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

void LCD::MIDIping(int8_t c) {
  // Don't cover display of playing channels with non-playing channels
  if(c < 0 && MIDIpingChan >= 0) return;
  lastMIDIping = LCDframe;
  MIDIpingChan = c;
}

uint16_t LCD::displayTypeUINT32(void *data) {
  return *(uint32_t*)data;
}

uint16_t LCD::displayTypeUINT8scaled(void *data) {
  return ((uint16_t)*(uint8_t*)data)*100/255;
}

uint16_t LCD::displayTypeUINT8scaled4x(void *data) {
  return ((uint16_t)*(uint8_t*)data)*400/255;
}

uint16_t LCD::displayTypeMIDIchannel(void *data) {
  return *(uint8_t*)data+1;
}

uint16_t LCD::displayTypeUINT32Hz(void *data) {
  return 100000 / *(uint32_t*)data;
}
