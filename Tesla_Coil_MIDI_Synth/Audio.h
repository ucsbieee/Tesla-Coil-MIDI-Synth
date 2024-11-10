#pragma once

#include "AudioMode.h"
#include "AudioModePredictive.h"
#include "AudioModeBinaryDDT.h"
#include "AudioModePWMDDT.h"
#include "AudioModePulseEnergy.h"
#include "AudioModeClampedBinary.h"
#include "AudioModeBinary.h"
#include "AudioModePWM.h"

#include <inttypes.h>

// Buffer sizes
#define NBUFS 5
#define BUF_SIZE 64
#define ZERO_BUF_SIZE 16

// Dynamic sample rate
#define MIN_SAMPLE_RATE 47950
#define NOM_SAMPLE_RATE 48000
#define MAX_SAMPLE_RATE 48050

// If don't receive audio after this many ms, stop streaming
#define AUDIO_TIMEOUT 100

// Artifically increase resolution of period to help stabilize sample rate
#define PERIOD_ADJ_SPEED 6

// Output pin (PIOA21)
#define PIO PIOA
#define PIN (1 << 21)

class Synth;

class Audio {
public:
  Audio();
  
  enum AudioMode_e {
    AM_OFF,
    AM_PREDICTIVE,
    AM_PULSE_ENERGY,
    AM_CLAMPED_BINARY,
    AM_BINARY,
    AM_BINARY_DDT,
    AM_PWM,
    AM_PWM_DDT,
    AM_END
  };
  
  AudioMode_e audioMode = AM_OFF;

  // List of processing algorithm objects
  AudioMode *processors[AM_END];
  
  // Adjustable audio controls applied to all samples
  uint8_t audioGain = 64; // Max 400%
  uint8_t audioNoiseGate = 13;
  
  // Equivalent to synth.vol, but scaled to the audio sample rate
  uint16_t pulseWidthMax;

  // Equivalent to synth.vol, but scaled to the PWM pulse width
  uint16_t pwmWidthMax;
  
  void init();
  void start();
  void stop();
  void process();
  void resetProcessing();
  
  // Called by ISR in main file
  void setDMABuffer();

private:
  struct Buffer {
    uint16_t len;
    uint16_t buf[BUF_SIZE];
  };
  
  // Sample period
  uint32_t period = (F_CPU/NOM_SAMPLE_RATE) << PERIOD_ADJ_SPEED;
  
  // Ring buffer of buffers
  volatile Buffer bufs[NBUFS];
  uint8_t readBuffer, writeBuffer;
  
  // Empty buffer to insert when no data available
  uint16_t zeroBuf[ZERO_BUF_SIZE];
  
  // Flag to drop incoming buffers if we're really behind on playing
  bool purgeBufs;
  
  // DMA and stuff currently enabled
  bool audioRunning;
  
  // Last time we got new audio data
  unsigned long lastAudioTimestamp;
  
  // Processing algorithm objects
  AudioMode              AM_OFF_o;
  AudioModePredictive    AM_PREDICTIVE_o;
  AudioModePulseEnergy   AM_PULSE_ENERGY_o;
  AudioModeClampedBinary AM_CLAMPED_BINARY_o;
  AudioModeBinary        AM_BINARY_o;
  AudioModeBinaryDDT     AM_BINARY_DDT_o;
  AudioModePWM           AM_PWM_o;
  AudioModePWMDDT        AM_PWM_DDT_o;
  
  // Return amount of filled buffers in ring buffer of buffers
  uint8_t bufsFilled();

  // Handle a single sample
  uint16_t processSample(int32_t in);
};
