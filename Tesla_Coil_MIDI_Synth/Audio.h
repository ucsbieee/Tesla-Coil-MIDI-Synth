#pragma once

#include <inttypes.h>

namespace Audio {

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

struct Buffer {
  uint16_t len;
  uint16_t buf[BUF_SIZE];
};

enum AudioMode {
  AM_OFF,
  AM_BINARY,
  AM_PROCESSED,
  AM_PWM,
  AM_INVALID
};

extern AudioMode audioMode;
extern const char *audioModeNames[AM_INVALID];

extern int32_t lastSample;
extern int32_t baseline;

void initAudio();
void startAudio();
void stopAudio();
void processAudio();
uint16_t processSample(int32_t in);

}
