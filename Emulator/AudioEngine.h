#pragma once

#include <list>
#include <deque>

#include <portaudiocpp/PortAudioCpp.hxx>

#include "Coil.h"
#include "Convolution.h"
#include "RingBuffer.h"

#define F_SAMP 48000
#define FRAMES_PER_BUFFER 128
#define CHANNELS 2

#define MAX_FIFO_SIZE (FRAMES_PER_BUFFER*CHANNELS*4)

#define VOLUME 0.8f
#define STEREO_SEPARATION 0.4f

// Impulse response data loaded from ir.bin
// Data has been pre-FFTed, so length is actually 2*(IR_LENGTH/2+1) = 4098 floats in RE-IM-RE-IM format
#define IR_LENGTH 4096
extern const float ir[IR_LENGTH/2+1];

class AudioEngine {
public:
	AudioEngine(std::list<Coil> &coils);

	// portaudio callback
	static int genAudio(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

protected:
	std::list<Coil> &coils;

	// Unprocessed audio generation state
	unsigned long sample; // Index of last sample generated
	unsigned long lastUpdateSample; // Index of last sample when synth was updated

	// Convolution objects to apply IR filter to each channel
	std::deque<Convolution> conv;

	// Generate some samples
	void generate(float *output);
};
