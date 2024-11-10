#pragma once

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
  Audio(Synth &synth);
  
  enum AudioMode {
    AM_OFF,
    AM_BINARY,
    AM_PROCESSED,
    AM_PWM,
    AM_INVALID
  };
  
  AudioMode audioMode = AM_OFF;
  static const char *audioModeNames[AM_INVALID];
  
  void init();
  void start();
  void stop();
  void process();
  
  // Called by ISR in main file
  void setDMABuffer();

private:
  Synth &synth;
  
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
  
  // Last processed sample (used for AM_PROCESSED mode)
  int32_t lastSample;
  
  // Baseline (used for AM_PWM mode)
  int32_t baseline;
  
  // Return amount of filled buffers in ring buffer of buffers
  uint8_t bufsFilled();

  // Handle a single sample
  uint16_t processSample(int32_t in);
};
