#include "Audio.h"
#include "Synth.h"
#include "USBAudio.h"

#include "AudioMode.h"

#include <Arduino.h>

// Construct list of audio mode processor objects
Audio::Audio():
processors{
  &AM_OFF_o,
  &AM_PREDICTIVE_o,
  &AM_PULSE_ENERGY_o,
  &AM_SCHMITT_o,
  &AM_CLAMPED_BINARY_o,
  &AM_BINARY_o,
  &AM_BINARY_DDT_o,
  &AM_PWM_o,
  &AM_PWM_DDT_o
},
pulseWidthMax(DEFAULT_VOL * ((uint32_t)(NOM_SAMPLE_RATE*MAX_WIDTH)) / 0x100),
pwmWidthMax(DEFAULT_VOL * (F_CPU/NOM_SAMPLE_RATE) / 0x100),
AM_OFF_o(*this),
AM_PREDICTIVE_o(*this),
AM_PULSE_ENERGY_o(*this),
AM_SCHMITT_o(*this),
AM_CLAMPED_BINARY_o(*this),
AM_BINARY_o(*this),
AM_BINARY_DDT_o(*this),
AM_PWM_o(*this),
AM_PWM_DDT_o(*this) {}

// Return amount of filled buffers in ring buffer of buffers
uint8_t Audio::bufsFilled() {
  uint8_t diff = writeBuffer - readBuffer;
  if(diff > NBUFS)
    diff += NBUFS;
  return diff;
}

// Called from ISR when one buffer has been played
void Audio::setDMABuffer() {
  // Check amount of buffers available
  uint8_t filled = bufsFilled();

  // If there is some data, play it with DMA
  if(filled) {
    PDC_PWM->PERIPH_TNPR = (uint32_t)bufs[readBuffer].buf;
    PDC_PWM->PERIPH_TNCR = bufs[readBuffer].len;

    // Increment read buffer
    readBuffer++;
    if(readBuffer >= NBUFS)
      readBuffer = 0;
    filled--;
  }

  // Otherwise insert a bit of silence
  else {
    PDC_PWM->PERIPH_TNPR = (uint32_t)zeroBuf;
    PDC_PWM->PERIPH_TNCR = ZERO_BUF_SIZE;
  }

  // Update sample rate based on current buffer fill level
  if(filled > 1)
    period = max(period-1, (F_CPU/MAX_SAMPLE_RATE) << PERIOD_ADJ_SPEED);
  else if(filled == 0)
    period = min(period+1, (F_CPU/MIN_SAMPLE_RATE) << PERIOD_ADJ_SPEED);

  PWM->PWM_CH_NUM[0].PWM_CPRDUPD = period >> PERIOD_ADJ_SPEED;
}

void Audio::init() {
  // Enable clock to PWM
  PMC->PMC_PCER1 = (1 << (ID_PWM-32));

  // Set up PWM
  PWM->PWM_SCM = PWM_SCM_SYNC0 // Enable channel 0 as a synchronous channel
   | PWM_SCM_UPDM_MODE2; // Update synchronous channels from PDC
  PWM->PWM_IDR1 = 0xFFFFFFFF; // Disable interrupts
  PWM->PWM_IDR2 = 0xFFFFFFFF;
  PWM->PWM_CH_NUM[0].PWM_CPRD = period >> PERIOD_ADJ_SPEED; // Set period to 48kHz
  PWM->PWM_CH_NUM[0].PWM_CCNT = 0; // Reset counter

  // (Prepare to) connect PWML0 to PA21B
  PIO->PIO_ABSR = PIN; // Select peripheral function B
  PIO->PIO_OER = PIN; // Fall back to low output when PWM not selected
  PIO->PIO_CODR = PIN;

  stop();

  // Enable data transmission using PWM PDC
  PDC_PWM->PERIPH_PTCR = PERIPH_PTCR_TXTEN;

  // Allow interrupts from PWM
  NVIC->ISER[1] = (1 << (PWM_IRQn-32));
}

void Audio::start() {
  // Disable PIO control, switch to peripheral function
  PIO->PIO_PDR = PIN;

  // Init buffer states
  readBuffer = 0;
  writeBuffer = 0;
  purgeBufs = false;

  resetProcessing();

  setDMABuffer();

  // Enable end of buffer interrupt
  PWM->PWM_IER2 = PWM_IER2_ENDTX;

  // Start PWM
  PWM->PWM_ENA = PWM_ENA_CHID0;

  audioRunning = true;
}

void Audio::stop() {
  // Enable PIO control, setting output low
  PIO->PIO_PER = PIN;

  // Stop PWM
  PWM->PWM_DIS = PWM_DIS_CHID0;

  // Disable interrupt
  PWM->PWM_IDR2 = 0xFFFFFFFF;

  // Clear USB audio buffer
  char dummy;
  while(USBAudio.available())
    USBAudio.read(&dummy, 1);

  audioRunning = false;
}

void Audio::process() {
  // Wait for data to be available
  if(USBAudio.available()) {
    lastAudioTimestamp = millis();

    // Make sure DMA is running
    if(!audioRunning)
      start();

    // Temporary place for unprocessed USB audio samples
    static int16_t rxBuf[BUF_SIZE];

    uint32_t len = USBAudio.read(rxBuf, BUF_SIZE*2);
    len /= 2; // Two bytes per sample

    uint8_t filled = bufsFilled();

    // Stop catching up/purging if we have done enough
    if(purgeBufs && filled < 2)
      purgeBufs = false;

    // Fill a new buffer if there is space
    if(!purgeBufs && filled < NBUFS-2) {
      // Process the new samples
      for(uint32_t x = 0; x < len; x++)
        bufs[writeBuffer].buf[x] = processSample(rxBuf[x]);
      bufs[writeBuffer].len = len;

      // Increment buffer
      writeBuffer++;
      if(writeBuffer >= NBUFS)
        writeBuffer = 0;
    }

    // Start purging if there isn't enough space
    else purgeBufs = true;
  }

  // If audio hasn't been running for a while, stop
  else {
    if(audioRunning && millis() - lastAudioTimestamp > AUDIO_TIMEOUT)
      stop();
  }
}

// Reset processing state variables
void Audio::resetProcessing() {
  for(auto *p:processors)
    p->reset();
}

// Handle a single sample
uint16_t Audio::processSample(int32_t in) {
  in = (in - (audioNoiseGate << 7)) * audioGain / 0x40;
  if(in > 0x7FFF) in = 0x7FFF;
  else if(in < -0x7FFF) in = -0x7FFF;
  
  return processors[audioMode]->processSample(in);
}
