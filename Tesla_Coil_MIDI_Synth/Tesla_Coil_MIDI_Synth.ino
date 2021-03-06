/* 
 * Standard MIDI things
    * Pitch bend
    * Aftertouch
 * Effects
    * Tremolo (volume) (rate = 102, depth = 92, delay = 103)
    * Vibrato (pitch) (rate = 76, depth = 77, delay = 78)
    * ADSR (116, 117, 118, 119)
    * Over/undertones (1/16 = 20, 1/8 = 21, 1/4 = 22, 1/2 = 23, 2 = 24, 3 = 25, 4 = 26, 5 = 27)
 * Channels
    * 1: Clean (no effects)
    * 2: Effects
    * 3: Arpeggio and effects (speed = 14)
    * 4: Drums: kick (36/C2), snare (40/E2), clap (39/D#2), tom (48/C3), and hi-hat (42/F#2)
 */

// TODO
// re-add harmonics

#include "MIDIUSB.h"
#include <limits.h>

// Uncomment to print incoming midi bytes over USB
#define PRINTMIDI

// Uncomment to automatically reduce pulse width when many voices are playing at once
//#define AUTODUCK

// Uncomment to make MIDI velocity control absolute pulse width instead of duty cycle
//#define ABSOLUTE_PULSE_WIDTH

const float midi2freq[] = {8.18,8.66,9.18,9.72,10.30,10.91,11.56,12.25,12.98,13.75,14.57,15.43,16.35,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,55.00,58.27,61.74,65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,554.37,587.33,622.25,659.26,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50,1108.73,1174.66,1244.51,1318.51,1396.91,1479.98,1567.98,1661.22,1760.00,1864.66,1975.53,2093.00,2217.46,2349.32,2489.02,2637.02,2793.83,2959.96,3135.96,3322.44,3520.00,3729.31,3951.07,4186.01,4434.92,4698.64,4978.03,5274.04,5587.65,5919.91,6271.93,6644.88,7040.00,7458.62,7902.13,8372.02,8869.84,9397.27,9956.06,10548.08,11175.30,11839.82,12543.85};

#define EXP_CRUNCH 3
uint8_t eLookup[256];
int8_t sinLookup[256];

#define MAX_WIDTH ((uint32_t)(F_CPU/2*3e-3)) // max pulse width (3ms)
#define MIN_WIDTH ((uint32_t)(F_CPU/2*10e-6))  // min pulse width (10us)
#define MIN_OFF_TIME ((int32_t)(F_CPU/2*50e-6)) // minimum time between pulses on each channel (50us)
#define VEL_THRESH 1 // minimum velocity
#define MAX_FREQ 4000 // if frequency is too high, pulses just merge together

#define NVOICES 6 // 6 timer channels broken out to pins (could use PWM unit for more but w/e)

// Buffer data coming in through hardware MIDI
unsigned char hwMIDIbuf[3], hwMIDIbufInd = 0;

// Volume control knob
uint16_t vol = 1023;

// Channels
enum {
  CHANNEL_CLEAN,
  CHANNEL_FX,
  CHANNEL_ARP,
  CHANNEL_DRUM,
  CHANNEL_INVALID
};

// Effect settings
#define TREMOLO_DEPTH_DEFAULT 0
#define TREMOLO_PERIOD_DEFAULT 1000
#define TREMOLO_DELAY_DEFAULT 1000
#define TREMOLO_PERIOD_MAX 3000
#define TREMOLO_PERIOD_MIN 10
#define TREMOLO_DELAY_MAX 10000
#define TREMOLO_DEPTH_CC 92
#define TREMOLO_PERIOD_CC 102
#define TREMOLO_DELAY_CC 103
uint8_t tremoloDepth = TREMOLO_DEPTH_DEFAULT;
uint32_t tremoloPeriod = TREMOLO_PERIOD_DEFAULT;
uint32_t tremoloDelay = TREMOLO_DELAY_DEFAULT;

#define VIBRATO_DEPTH_DEFAULT 0
#define VIBRATO_PERIOD_DEFAULT 1000
#define VIBRATO_DELAY_DEFAULT 1000
#define VIBRATO_DEPTH_MAX 200
#define VIBRATO_PERIOD_MAX 3000
#define VIBRATO_PERIOD_MIN 10
#define VIBRATO_DELAY_MAX 10000
#define VIBRATO_DEPTH_CC 77
#define VIBRATO_PERIOD_CC 76
#define VIBRATO_DELAY_CC 78
uint8_t vibratoDepth = VIBRATO_DEPTH_DEFAULT;
uint32_t vibratoPeriod = VIBRATO_PERIOD_DEFAULT;
uint32_t vibratoDelay = VIBRATO_DELAY_DEFAULT;

#define ATTACK_DEFAULT 0
#define DECAY_DEFAULT 0
#define SUSTAIN_DEFAULT 255
#define RELEASE_DEFAULT 0
#define ADSR_MAX 2000
#define ATTACK_CC 116
#define DECAY_CC 117
#define SUSTAIN_CC 118
#define RELEASE_CC 119
uint32_t attack = ATTACK_DEFAULT;
uint32_t decay = DECAY_DEFAULT;
uint8_t sustain = SUSTAIN_DEFAULT;
uint32_t release = RELEASE_DEFAULT;

#define NHARMONICS 5
#define HARMONIC_PATTERN_LEN 512 // 2^(2*NHARMONICS-1)
#define _DEFAULT_HARMONICS {0,0,0,0,255,0,0,0,0}
#define HARMONIC_CC_START 20
const uint8_t DEFAULT_HARMONICS[] = _DEFAULT_HARMONICS;
uint8_t harmonics[] = _DEFAULT_HARMONICS;

#define ARPEGGIO_PERIOD_DEFAULT 100
#define ARPEGGIO_PERIOD_MIN 1
#define ARPEGGIO_PERIOD_MAX 100
#define ARPEGGIO_LINGER 50
#define ARPEGGIO_CC 14
uint32_t arpeggioPeriod = ARPEGGIO_PERIOD_DEFAULT;

#define PITCH_BEND_RANGE 1.1224489796 // 2 semitones

// Drum presets
typedef struct {
  // MIDI note to be triggered by
  uint8_t midiNote;

  // Frequency to apply modulation around
  float baseNote;
  
  // ADSR envelope (drums are momentary things, don't need D or S)
  uint32_t a;
  uint32_t r;

  // Amount/type of frequency modulation
  float noiseMod;
  float envMod;
} Drum;

const Drum drumPresets[] = {
  {42, 500,  0, 50,  0.5,  0.0},  // Closed hi-hat (F#2)
  {39, 1000, 0, 125, 0.75, 0.0},  // Clap (D#2)
  {48, 100,  0, 250, 0.25, 0.25}, // Tom (C3)
  {40, 800,  0, 500, 0.5, -0.5},  // Snare (E2)
  {36, 30,   0, 200, 0.0,  0.15}  // Kick (C2)
};

#define MAX_ARP_NOTES 8

typedef struct {
  // MIDI information
  uint8_t midiChannel;
  uint8_t midiNote;
  uint8_t midiVel;
  int16_t midiPB;
  bool midiNoteDown;

  // Oscillator state
  uint32_t period;
  uint32_t pulseWidth;

  // This voice is still sounding
  bool active;

  // ADSR
  unsigned char adsrStage;
  unsigned long adsrTimestamp;
  uint8_t lastEnv;

  // Drum sample
  const Drum *drum;

  // Arp notes
  uint8_t arpNotes[MAX_ARP_NOTES];
  unsigned long arpNoteEndTimestamps[MAX_ARP_NOTES];
  uint8_t arpNotesIndex;
  unsigned long arpTimestamp;
  
  // Timestamp when this note started
  unsigned long noteDownTimestamp;

  // Current harmonic pulse index
  uint8_t harmonicInd;
  uint32_t harmonicLUT[HARMONIC_PATTERN_LEN];
} Voice;

Voice voices[NVOICES];
volatile uint8_t voicesUpdating = 0;

typedef struct {
  Tc *timer;
  TcChannel *channel;
  Pio *port;
  uint8_t portN;
  bool peripheralab;
  bool timerab;
} VoiceConfig;

const VoiceConfig voiceConfigs[] = {
  {TC0, &TC0->TC_CHANNEL[0], PIOB, 25, 1, 0}, // PWM2
  {TC0, &TC0->TC_CHANNEL[1], PIOA, 2 , 0, 0}, // AD7
  {TC0, &TC0->TC_CHANNEL[2], PIOA, 6 , 0, 1}, // AD4
//  {TC1, &TC1->TC_CHANNEL[0], PIOB, 25, 1, 0}, // Not broken out on DUE
//  {TC1, &TC1->TC_CHANNEL[1], PIOB, 25, 1, 0}, // Not broken out on DUE
//  {TC1, &TC1->TC_CHANNEL[2], PIOB, 25, 1, 0}, // Not broken out on DUE
  {TC2, &TC2->TC_CHANNEL[0], PIOC, 25, 1, 0}, // PWM5
  {TC2, &TC2->TC_CHANNEL[1], PIOC, 28, 1, 0}, // PWM3
  {TC2, &TC2->TC_CHANNEL[2], PIOD,  7, 1, 0}  // PWM11
};

void setup() {
#ifdef PRINTMIDI
  SerialUSB.begin(115200);
#endif

  // Physical MIDI interface
  Serial.begin(31250);

  memset(voices, 0, sizeof(voices));

  // Precompute exponential decay thing
  const float scale = exp(-EXP_CRUNCH);
  for(int x=0; x<256; x++)
    eLookup[x] = (exp(-x*EXP_CRUNCH/255.0)-scale)/(1-scale)*255;

  // Precompute sine
  for(int x=0; x<256; x++)
    sinLookup[x] = sin(2*PI*x/256.0)*127;

  // Enable watchdog timer so we get reset after 1s if we crash
  // Doesn't seem to work since maybe Arduino stuff writes to it first... whatever
  WDT->WDT_MR = WDT_MR_WDRSTEN | WDT_MR_WDV(255) | WDT_MR_WDD(255);

  // Allow timer interrupts
  NVIC->ISER[0] = (1<<TC0_IRQn) | (1<<TC1_IRQn) | (1<<TC2_IRQn) | (1<<TC6_IRQn) | (1<<TC7_IRQn) | (1<<TC8_IRQn);

  PMC->PMC_WPMR = 0x504D4300; // Disable write protect
  PMC->PMC_PCER0 = (1<<ID_TC0) | (1<<ID_TC1) | (1<<ID_TC2); // Enable clock to timers
  PMC->PMC_PCER1 = (1<<(ID_TC6-32)) | (1<<(ID_TC7-32)) | (1<<(ID_TC8-32));

  // Init timers
  for(int x=0; x<NVOICES; x++) {
    if(voiceConfigs[x].peripheralab) voiceConfigs[x].port->PIO_ABSR |= (1<<voiceConfigs[x].portN); // Select peripheral B
    else voiceConfigs[x].port->PIO_ABSR &= ~(1<<voiceConfigs[x].portN); // Select peripheral A
    voiceConfigs[x].port->PIO_PDR = (1<<voiceConfigs[x].portN); // Disable PIO control, allow peripheral to use it
    
    voiceConfigs[x].timer->TC_WPMR = 0x54494D00; // Disable write protect
    voiceConfigs[x].channel->TC_CCR = TC_CCR_CLKDIS; // Disable clock
    voiceConfigs[x].channel->TC_IDR = 0xFF; // Disable interrupts
    voiceConfigs[x].channel->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 // MCK/2
      | TC_CMR_EEVT_XC0 // Make sure EEVT thing is set so TIOB can be an output
      | TC_CMR_WAVSEL_UP_RC // Select up counting with reset at RC compare
      | TC_CMR_WAVE; // Waveform mode
      
    voiceConfigs[x].channel->TC_RC = F_CPU/2/100; // 100Hz
    
    if(voiceConfigs[x].timerab) {
      voiceConfigs[x].channel->TC_CMR |= TC_CMR_BCPB_CLEAR // Clear TIOB on compare with RB
        | TC_CMR_BSWTRG_CLEAR; // Clear TIOB on software trigger
      voiceConfigs[x].channel->TC_RB = 1;
    } else {
      voiceConfigs[x].channel->TC_CMR |= TC_CMR_ACPA_CLEAR // Clear TIOA on compare with RA
        | TC_CMR_ASWTRG_CLEAR; // Clear TIOA on software trigger
      voiceConfigs[x].channel->TC_RA = 1;
    }

//    voiceConfigs[x].channel->TC_IER = TC_IER_CPCS; // Enable RC compare interrupt
    voiceConfigs[x].channel->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // Enable clock
  }
  
}

extern "C" {
int sysTickHook() { // Runs at 1kHz
  // Prevent from running while voice settings are being updated
  if(voicesUpdating) {
    voicesUpdating = 2;
    return 0;
  }
  
  // Update voices
  int totalEnv = 0;
  for(int x=0; x<NVOICES; x++) {
    Voice &voice = voices[x];
    
    if(voice.active) {
      unsigned long ms = millis();
  
      // Process ADSR/pulse width
      uint16_t env = 255;
      if(voice.midiChannel == CHANNEL_FX || voice.midiChannel == CHANNEL_ARP || voice.midiChannel == CHANNEL_DRUM) {
        uint32_t localA = attack;
        uint32_t localD = decay;
        uint8_t localS = sustain;
        uint32_t localR = release;
        if(voice.drum) {
          localA = voice.drum->a;
          localR = voice.drum->r;
        }
  
        // Increment stage if needed
        bool nextStage = false;
        unsigned long dt = ms-voice.adsrTimestamp;
        if(voice.adsrStage == 0) {
          if(dt > localA) {
            nextStage = true;
            if(voice.midiChannel == CHANNEL_DRUM) {
              voice.adsrStage = 2; // Skip D and S for drum
              voice.lastEnv = 255;
            }
          }
        } else if(voice.adsrStage == 1) {
          if(dt > localD) nextStage = true;
        } else if(voice.adsrStage == 3) {
          if(dt > localR) {
            voice.active = false;
            nextStage = true;
            env = 0;
            if(voice.midiChannel == CHANNEL_ARP) memset(voice.arpNoteEndTimestamps, 0, sizeof(voice.arpNoteEndTimestamps));
          }
        }
        if(voice.adsrStage < 3 && !voice.midiNoteDown) {
          voice.adsrStage = 2;
          nextStage = true;
        }
        if(nextStage) {
          voice.adsrStage++;
          voice.adsrTimestamp = ms;
          dt = 0;
        }
  
        // Compute envelope
        if(voice.adsrStage == 0) {
          if(localA == 0) localA = 1;
          env = 255-eLookup[(uint64_t)dt*255/localA];
        } else if(voice.adsrStage == 1) {
          if(localD == 0) localD = 1;
          env = (255-localS)*(uint32_t)eLookup[(uint64_t)dt*255/localD]/255+localS;
        } else if(voice.adsrStage == 2) {
          env = localS;
        } else if(voice.adsrStage == 3) {
          if(localR == 0) localR = 1;
          env = voice.lastEnv*(uint32_t)eLookup[(uint64_t)dt*255/localR]/255;
        }

        // Keep track of env right before release so it releases from the right point
        if(voice.adsrStage < 3) voice.lastEnv = env;
  
//        voice.pulseWidth = (uint64_t)env*voice.midiVel*MAX_WIDTH/32385;
      } else {
        if(voice.midiNoteDown) env = voice.midiVel*2; //voice.pulseWidth = (uint64_t)voice.midiVel*MAX_WIDTH/127;
        else {
          voice.active = false;
//          voice.pulseWidth = 0;
          env = 0;
        }
      }
      
      // Process drum/arp
      float note = 100;
      if(voice.midiChannel == CHANNEL_DRUM) {
        note = voice.drum->baseNote;
        note *= env/255.0*voice.drum->envMod+1;
        note *= ((float)rand()/RAND_MAX*2-1)*voice.drum->noiseMod+1;
      } else if(voice.midiChannel == CHANNEL_ARP) {
        // Check if all the notes have been released
        if(voice.midiNoteDown) {
          voice.midiNoteDown = false;
          for(int y=0; y<MAX_ARP_NOTES; y++)
            if((int64_t)voice.arpNoteEndTimestamps[y] - (int64_t)ARPEGGIO_LINGER > ms) {
              voice.midiNoteDown = true;
              break;
            }
        }
        // If all keys have been released, lock all remaining notes on
        if(!voice.midiNoteDown) {
          for(int y=0; y<MAX_ARP_NOTES; y++)
            if(voice.arpNoteEndTimestamps[y] > ms) voice.arpNoteEndTimestamps[y] = ULONG_MAX;
        }
        // Go to next note
        if(ms-voice.arpTimestamp > arpeggioPeriod) {
          for(int y=0; y<MAX_ARP_NOTES; y++) {
            voice.arpNotesIndex++;
            if(voice.arpNotesIndex >= MAX_ARP_NOTES) voice.arpNotesIndex = 0;
            if(voice.arpNoteEndTimestamps[voice.arpNotesIndex] > ms) break;
          }
          voice.arpTimestamp = ms;
        }
        note = midi2freq[voice.arpNotes[voice.arpNotesIndex]];
      } else note = midi2freq[voice.midiNote];

      // Process pitch bend
      note *= 1+(PITCH_BEND_RANGE-1)*voice.midiPB/0x2000;
  
      // Process tremolo and vibrato
      if(voice.midiChannel == CHANNEL_FX || voice.midiChannel == CHANNEL_ARP) {
        unsigned long dt = ms-voice.noteDownTimestamp;
        
        uint32_t lookupIndex = (uint64_t)dt*255/tremoloDelay;
        uint8_t tremoloAmount;
        if(lookupIndex > 255) tremoloAmount = 255;
        else tremoloAmount = lookupIndex;
  
        lookupIndex = (uint64_t)dt*255/vibratoDelay;
        uint8_t vibratoAmount;
        if(lookupIndex > 255) vibratoAmount = 255;
        else vibratoAmount = lookupIndex;
  
        int8_t tremoloOscillate = sinLookup[((uint64_t)dt*255/tremoloPeriod)&0xFF];
        int8_t vibratoOscillate = sinLookup[((uint64_t)dt*255/vibratoPeriod)&0xFF];
  
        env *= (int32_t)tremoloDepth*tremoloAmount*tremoloOscillate/8258175.0+1;
        note *= (int32_t)vibratoDepth*vibratoAmount*vibratoOscillate/8258175.0+1;
      }

      voice.period = F_CPU/2/note; // /NHARMONICS
      
      // Save env in pulseWidth; will be overwritten later to the correct pulse width
      voice.pulseWidth = env;

      // Keep track of the total amount of stuff playing
      totalEnv += env;
      
      // Generate LUT in order to quickly update harmonic pulse widths
      /*
      uint32_t lutWidth = 0;
      uint32_t lutIndex = 1;
      for(int y=(NHARMONICS-1)*2; y>=0; y++) {
        lutWidth += voice.pulseWidth*harmonics[y]/255;
        if(lutWidth > MAX_WIDTH) lutWidth = MAX_WIDTH;
        voice.harmonicLUT[lutIndex] = lutWidth;
        lutIndex = lutIndex*2+1;
      }
      */
    }
  }

  // Compute how much to reduce the pulse width based on how much stuff is currently playing to make it more intelligible
#ifdef AUTODUCK
  uint16_t duck = (255*255) / max(totalEnv-255, 255);
#else
  static const uint16_t duck = 255;
#endif

  // Iterate through voices again to update the timers
  for(int x=0; x<NVOICES; x++) {
    Voice &voice = voices[x];
    
    if(voice.active) {
      // Make duty cycle correspond to envelope
      uint16_t env = voice.pulseWidth;
#ifdef ABSOLUTE_PULSE_WIDTH
      uint64_t maxWidth = MAX_WIDTH*3/4*vol/1023;
#else
      uint64_t maxWidth = voice.period*3/4*vol/1023; // Limit to 75% duty cycle
      if(maxWidth > MAX_WIDTH) maxWidth = MAX_WIDTH;
#endif
      voice.pulseWidth = maxWidth*env*duck/65025;

      if((int32_t)voice.period-(int32_t)voice.pulseWidth < MIN_OFF_TIME) voice.pulseWidth = voice.period - MIN_OFF_TIME;

      // Update timer
      updatePeriod(x, voice.period);
      updateWidth(x, voice.pulseWidth);
    } else { // Note is not active
      updateWidth(x, 0);
    }
  }

  // Reset WDT
  WDT->WDT_CR = WDT_CR_KEY(0xA5) | WDT_CR_WDRSTT;

  return 0;
}
}

// Update pulse width of a timer
inline __attribute__((always_inline)) void updateWidth(uint8_t chan, uint32_t pulseWidth) {
  const VoiceConfig &vc = voiceConfigs[chan];
  if(vc.timerab) {
    // If we decrease the compare value below the counter value, it will never equal it to set the pin low
    //   and the pin will stay high for an entire period (bad).
    // Include some margin since counter will keep going as the code runs
    if(vc.channel->TC_RB > vc.channel->TC_CV && pulseWidth < vc.channel->TC_CV+10) {
      uint32_t temp = vc.channel->TC_CV; // save count value
      vc.channel->TC_CCR = TC_CCR_SWTRG; // trigger the timer to reset it back to 0 and set pin low
      vc.channel->TC_CV = temp; // put counter value back
    }
    vc.channel->TC_RB = pulseWidth; // update pulse width
    if(pulseWidth < MIN_WIDTH) {
      vc.channel->TC_CMR &= ~TC_CMR_BCPC_SET; // disable output if pulse width is too small
      vc.channel->TC_CCR = TC_CCR_SWTRG; // re-trigger the timer
    } else vc.channel->TC_CMR |= TC_CMR_BCPC_SET;
  } else {
    if(vc.channel->TC_RA > vc.channel->TC_CV && pulseWidth < vc.channel->TC_CV+10) {
      uint32_t temp = vc.channel->TC_CV;
      vc.channel->TC_CCR = TC_CCR_SWTRG;
      vc.channel->TC_CV = temp;
    }
    vc.channel->TC_RA = pulseWidth;
    if(pulseWidth < MIN_WIDTH) {
      vc.channel->TC_CMR &= ~TC_CMR_ACPC_SET;
      vc.channel->TC_CCR = TC_CCR_SWTRG;
    } else vc.channel->TC_CMR |= TC_CMR_ACPC_SET;
  }
}

// Update frequency of a timer
inline __attribute__((always_inline)) void updatePeriod(uint8_t chan, uint32_t period) {
  const VoiceConfig &vc = voiceConfigs[chan];
  vc.channel->TC_RC = period;
  if(vc.channel->TC_CV > period) { // Reset so counter stays below the period (otherwise would get long pulses)
    if(vc.timerab) {
      // Temporarily make software trigger set the pin instead of reset in order to behave the same as an RC compare
      if(vc.channel->TC_CMR & TC_CMR_BCPC_SET) { // Only do this if the output is enabled
        vc.channel->TC_CMR = (vc.channel->TC_CMR & ~(0b11<<TC_CMR_BSWTRG_Pos)) | TC_CMR_BSWTRG_SET;
        vc.channel->TC_CCR = TC_CCR_SWTRG;
        vc.channel->TC_CMR = (vc.channel->TC_CMR & ~(0b11<<TC_CMR_BSWTRG_Pos)) | TC_CMR_BSWTRG_CLEAR;
      }
    } else {
      if(vc.channel->TC_CMR & TC_CMR_ACPC_SET) {
        vc.channel->TC_CMR = (vc.channel->TC_CMR & ~(0b11<<TC_CMR_ASWTRG_Pos)) | TC_CMR_ASWTRG_SET;
        vc.channel->TC_CCR = TC_CCR_SWTRG;
        vc.channel->TC_CMR = (vc.channel->TC_CMR & ~(0b11<<TC_CMR_ASWTRG_Pos)) | TC_CMR_ASWTRG_CLEAR;
      }
    }
  }
}

// Update pulse width to support different widths at harmonics and subharmonics
inline __attribute__((always_inline)) void processHarmonics(uint8_t chan) {
  Voice &voice = voices[chan];
  uint8_t lastIndex = voice.harmonicInd;
  voice.harmonicInd++;
  updateWidth(chan, voice.harmonicLUT[lastIndex^voice.harmonicInd]);
}

void TC0_Handler() {
  if(TC0->TC_CHANNEL[0].TC_SR & TC_SR_CPCS) processHarmonics(0);
}

void TC1_Handler() {
  if(TC0->TC_CHANNEL[1].TC_SR & TC_SR_CPCS) processHarmonics(1);
}

void TC2_Handler() {
  if(TC0->TC_CHANNEL[2].TC_SR & TC_SR_CPCS) processHarmonics(2);
}

void TC6_Handler() {
  if(TC2->TC_CHANNEL[0].TC_SR & TC_SR_CPCS) processHarmonics(3);
}

void TC7_Handler() {
  if(TC2->TC_CHANNEL[1].TC_SR & TC_SR_CPCS) processHarmonics(4);
}

void TC8_Handler() {
  if(TC2->TC_CHANNEL[2].TC_SR & TC_SR_CPCS) processHarmonics(5);
}

void noteDown(uint8_t channel, uint8_t note, uint8_t vel) {
  if(channel >= CHANNEL_INVALID) return;

  unsigned long ms = millis();

  const Drum *drum = NULL;
  if(channel == CHANNEL_DRUM) {
    for(int x=0; x<sizeof(drumPresets)/sizeof(Drum); x++)
      if(drumPresets[x].midiNote == note) {
        drum = drumPresets+x;
        break;
      }
    if(!drum) return;
  }
  
  voicesUpdating = 1;
  
  int chosen = -1;

  // Arp notes should be combined
  if(channel == CHANNEL_ARP)
    for(int x=0; x<NVOICES; x++)
      if(voices[x].midiChannel == CHANNEL_ARP && (voices[x].midiNoteDown == voices[x].active)) {
        chosen = x;
        break;
      }

  // Use any unused voice
  for(int x=0; chosen<0 && x<NVOICES; x++)
    if(!voices[x].active)
      chosen = x;

  // If all voices are in use...
  if(chosen < 0 && channel == CHANNEL_DRUM) {
    // Drum prefers to replace another drum
    unsigned long longest = 0;
    for(int x=0; x<NVOICES; x++) {
      if(voices[x].midiChannel == CHANNEL_DRUM) {
        unsigned long dt = ms - voices[x].noteDownTimestamp;
        if(dt >= longest) {
          chosen = x;
          longest = dt;
        }
      }
    }
  }

  if(chosen < 0) {
    // Replace oldtest note
    unsigned long longest = 0;
    for(int x=0; x<NVOICES; x++) {
      unsigned long dt = ms - voices[x].noteDownTimestamp;
      if(dt >= longest) {
        chosen = x;
        longest = dt;
      }
    }
  }

  Voice &chosenVoice = voices[chosen];

  if(channel == CHANNEL_ARP) {
    int arpInsert = 0;
    
    // Keep track of the note the index was pointing to
    int playingNote = chosenVoice.arpNotes[chosenVoice.arpNotesIndex];
    
    // Get rid of empty notes at the left to make space for new one
    // Want to make sure arpInsert is a valid index at the end of this loop
    for(; arpInsert<MAX_ARP_NOTES-1; arpInsert++) {
      if(chosenVoice.arpNoteEndTimestamps[arpInsert] > ms) {
        if(chosenVoice.arpNotes[arpInsert] == note) return; // Already have note playing
        if(chosenVoice.arpNotes[arpInsert] > note) break; // Notes should be in ascending order
      } else {
        bool found = false;
        for(int x=arpInsert+1; x<MAX_ARP_NOTES; x++) // If not playing, look for one that is playing to move back
        if(chosenVoice.arpNoteEndTimestamps[x] > ms && chosenVoice.arpNotes[x] <= note) {
          if(chosenVoice.arpNotes[x] == note) return; // Keep making sure the new note isn't already here
          chosenVoice.arpNotes[arpInsert] = chosenVoice.arpNotes[x]; // Move note back
          chosenVoice.arpNoteEndTimestamps[arpInsert] = chosenVoice.arpNoteEndTimestamps[x];
          chosenVoice.arpNoteEndTimestamps[x] = 0;
          found = true;
          break;
        }
        if(!found) break; // Nothing to move back, can stop here
      }
    }
    
    // Compress all the stuff to the right
    for(int x=MAX_ARP_NOTES-1; x>arpInsert; x--) {
      if(chosenVoice.arpNoteEndTimestamps[x] > ms) {
        if(chosenVoice.arpNotes[x] == note) return; // Already have note playing
      } else {
        bool found = false;
        for(int y=x-1; y>=arpInsert; y--) { // If not playing, look for one that is playing to move back
          if(chosenVoice.arpNoteEndTimestamps[y] > ms) {
            if(chosenVoice.arpNotes[y] == note) return; // Keep making sure the new note isn't already here
            chosenVoice.arpNotes[x] = chosenVoice.arpNotes[y]; // Move note forward
            chosenVoice.arpNoteEndTimestamps[x] = chosenVoice.arpNoteEndTimestamps[y];
            chosenVoice.arpNoteEndTimestamps[y] = 0;
            found = true;
            break;
          }
        }
        if(!found) break; // Nothing to move back, can stop here
      }
    }
    
    chosenVoice.arpNotes[arpInsert] = note;
    chosenVoice.arpNoteEndTimestamps[arpInsert] = ULONG_MAX;
    
    for(chosenVoice.arpNotesIndex = 0; chosenVoice.arpNotesIndex<MAX_ARP_NOTES; chosenVoice.arpNotesIndex++)
      if(chosenVoice.arpNoteEndTimestamps[chosenVoice.arpNotesIndex] > ms && chosenVoice.arpNotes[chosenVoice.arpNotesIndex] > playingNote) { // Make sure arpNotesIndex points to the current note playing so we always increase in frequency
        chosenVoice.arpNotesIndex--;
        break;
      }
    if(chosenVoice.arpNotesIndex < 0 || chosenVoice.arpNotesIndex >= MAX_ARP_NOTES) chosenVoice.arpNotesIndex = MAX_ARP_NOTES-1;
    
  } else {
    memset(chosenVoice.arpNoteEndTimestamps, 0, sizeof(chosenVoice.arpNoteEndTimestamps));
    chosenVoice.arpNotesIndex = 0;
    chosenVoice.arpTimestamp = ms;
  }

noteDownComplete:
  chosenVoice.midiChannel = channel;
  chosenVoice.midiNote = note;
  chosenVoice.midiVel = vel;
  if(!chosenVoice.active) chosenVoice.midiPB = 0;
  chosenVoice.active = true;
  chosenVoice.midiNoteDown = true;
  chosenVoice.noteDownTimestamp = ms;

  chosenVoice.adsrStage = 0;
  chosenVoice.adsrTimestamp = chosenVoice.noteDownTimestamp;
  chosenVoice.lastEnv = 0;

  chosenVoice.drum = drum;

  if(voicesUpdating > 1) {
    voicesUpdating = 0;
    sysTickHook();
    return;
  }
  voicesUpdating = 0;
}

void noteUp(uint8_t channel, uint8_t note) {
  voicesUpdating = 1;

  for(int x=0; x<NVOICES; x++) {
    Voice &voice = voices[x];
    if(voice.midiChannel == channel) {
      if(channel == CHANNEL_ARP) {
        unsigned long ms = millis();
        for(int y=0; y<MAX_ARP_NOTES; y++)
          if(voice.arpNotes[y] == note && voice.arpNoteEndTimestamps[y] > ms) {
            voice.arpNoteEndTimestamps[y] = ms+ARPEGGIO_LINGER;
            break;
          }
      } else if(voice.midiNote == note) voice.midiNoteDown = false;
    }
  }

  if(voicesUpdating > 1) {
    voicesUpdating = 0;
    sysTickHook();
    return;
  }
  voicesUpdating = 0;
}

void aftertouch(uint8_t channel, uint8_t note, uint8_t vel) {
  for(int x=0; x<NVOICES; x++) {
    Voice &voice = voices[x];
    if(voice.midiChannel == channel) {
      bool thisVoice = (voice.midiNote == note);
      if(!thisVoice && voice.midiChannel == CHANNEL_ARP) { // Check if this command is for one of the arp notes
        unsigned long ms = millis();
        for(int y=0; y<MAX_ARP_NOTES; y++)
          if(voice.arpNoteEndTimestamps[y] > ms && voice.arpNotes[y] == note) {
            thisVoice = true;
            break;
          }
      }
      if(thisVoice) {
        voice.midiVel = vel;
        break;
      }
    }
  }
}

void pitchBend(uint8_t channel, uint8_t low7, uint8_t high7) {
  int16_t pb = (int16_t)(((int16_t)high7)<<7 | low7) - 0x2000;
  for(int x=0; x<NVOICES; x++) {
    Voice &voice = voices[x];
    if(voice.midiChannel == channel)
      voice.midiPB = pb;
  }
}

void cc(uint8_t channel, uint8_t control, uint8_t value) {
  switch(control) {
    case 120: // All sound/oscillators off
    case 123:
      for(int x=0; x<NVOICES; x++)
        voices[x].active = false;
      break;
    case 121: // Reset stuff to initial values
        tremoloDepth = TREMOLO_DEPTH_DEFAULT;
        tremoloPeriod = TREMOLO_PERIOD_DEFAULT;
        tremoloDelay = TREMOLO_DELAY_DEFAULT;
        vibratoDepth = VIBRATO_DEPTH_DEFAULT;
        vibratoPeriod = VIBRATO_PERIOD_DEFAULT;
        vibratoDelay = VIBRATO_DELAY_DEFAULT;
        attack = ATTACK_DEFAULT;
        decay = DECAY_DEFAULT;
        sustain = SUSTAIN_DEFAULT;
        release = RELEASE_DEFAULT;
        arpeggioPeriod = ARPEGGIO_PERIOD_DEFAULT;
        memcpy(harmonics, DEFAULT_HARMONICS, sizeof(harmonics));
      break;
    case TREMOLO_DEPTH_CC:
      tremoloDepth = value*2;
      break;
    case TREMOLO_PERIOD_CC:
      tremoloPeriod = ((uint32_t)TREMOLO_PERIOD_MAX-TREMOLO_PERIOD_MIN)*(0x7F-value)/0x7F+TREMOLO_PERIOD_MIN;
      break;
    case TREMOLO_DELAY_CC:
      tremoloDelay = (uint32_t)TREMOLO_DELAY_MAX*value/0x7F;
      break;
    case VIBRATO_DEPTH_CC:
      vibratoDepth = (uint32_t)VIBRATO_DEPTH_MAX*value/0x7F;
      break;
    case VIBRATO_PERIOD_CC:
      vibratoPeriod = ((uint32_t)VIBRATO_PERIOD_MAX-VIBRATO_PERIOD_MIN)*(0x7F-value)/0x7F+VIBRATO_PERIOD_MIN;
      break;
    case VIBRATO_DELAY_CC:
      vibratoDelay = (uint32_t)VIBRATO_DELAY_MAX*value/0x7F;
      break;
    case ATTACK_CC:
      attack = (uint32_t)ADSR_MAX*value/0x7F;
      break;
    case DECAY_CC:
      decay = (uint32_t)ADSR_MAX*value/0x7F;
      break;
    case SUSTAIN_CC:
      sustain = value*2;
      break;
    case RELEASE_CC:
      release = (uint32_t)ADSR_MAX*value/0x7F;
      break;
    case ARPEGGIO_CC:
      arpeggioPeriod = ((uint32_t)ARPEGGIO_PERIOD_MAX-ARPEGGIO_PERIOD_MIN)*(0x7F-value)/0x7F+ARPEGGIO_PERIOD_MIN;
      break;
    default:
      if(control >= HARMONIC_CC_START && control < HARMONIC_CC_START+(NHARMONICS-1)*2)
        harmonics[control-HARMONIC_CC_START] = value*2;
      break;
  }
}

void handleMIDI(unsigned char byte1, unsigned char byte2, unsigned char byte3) {
  unsigned char command = byte1 >> 4;
    unsigned char channel = byte1 & 0xF;

#ifdef PRINTMIDI
    SerialUSB.print("MIDI: ");
    SerialUSB.print(byte1, HEX);
    SerialUSB.print(" ");
    SerialUSB.print(byte2, HEX);
    SerialUSB.print(" ");
    SerialUSB.println(byte3, HEX);
#endif
    
    switch(command) {
      case 0x9:
        if(byte3 >= VEL_THRESH) noteDown(channel, byte2, byte3);
        else noteUp(channel, byte2);
        break;
        
      case 0x8:
        noteUp(channel, byte2);
        break;
        
      case 0xA:
        aftertouch(channel, byte2, byte3);
        break;

      case 0xB:
        cc(channel, byte2, byte3);
        break;

      case 0xE:
        pitchBend(channel, byte2, byte3);
        break;
    }
}

void loop() {
  static midiEventPacket_t rx;
  rx = MidiUSB.read();
  if(rx.header) handleMIDI(rx.byte1, rx.byte2, rx.byte3);
  
  while(Serial.available()) {
    unsigned char d = Serial.read();
    if(d & 0x80) hwMIDIbufInd = 0;
    hwMIDIbuf[hwMIDIbufInd++] = d;
    if(hwMIDIbufInd >= 3) {
      hwMIDIbufInd = 0;
      handleMIDI(hwMIDIbuf[0], hwMIDIbuf[1], hwMIDIbuf[2]);
    }
  }

  vol = analogRead(0);
}
