#include "AudioEngine.h"

#include <cassert>

using namespace std;

AudioEngine::AudioEngine(list<Coil> &coils): coils(coils), sample(0), lastUpdateSample(0) {
	for(size_t chan = 0; chan < CHANNELS; chan++)
		conv.emplace_back(ir, IR_LENGTH, FRAMES_PER_BUFFER);
}

int AudioEngine::genAudio(const void *input, void *_output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {

	AudioEngine &engine = *static_cast<AudioEngine*>(userData);
	float *output = static_cast<float*>(_output);

	assert(frameCount == FRAMES_PER_BUFFER);

	engine.generate(output);

	return paContinue;
}

void AudioEngine::generate(float *output) {
	// Get input buffer locations for each channel
	float *inputs[CHANNELS];
	for(size_t chan = 0; chan < CHANNELS; chan++)
		inputs[chan] = conv[chan].getInput();

	// Fill intermediate area with samples
	for(size_t ind = 0; ind < FRAMES_PER_BUFFER; ind++, sample++) {
		float lout = 0, rout = 0;

		for(auto &coil:coils) {
			float lweight = 1, rweight = 1;

			if(coil.aoMode == Coil::LEFT)
				rweight = STEREO_SEPARATION;
			else if(coil.aoMode == Coil::RIGHT)
				lweight = STEREO_SEPARATION;

			const float sample = coil.getNextSample() * VOLUME;

			lout += lweight * sample;
			rout += rweight * sample;
		}

		inputs[0][ind] = lout;
		inputs[1][ind] = rout;

		// Update synth state every 1ms
		if(sample - lastUpdateSample >= F_SAMP/1000) {
			for(auto &coil:coils)
				coil.updateSynth();
			lastUpdateSample = sample;
		}
	}

	// Run convolutions for each channel
	const float *outputs[CHANNELS];
	for(size_t chan = 0; chan < CHANNELS; chan++)
		outputs[chan] = conv[chan].getOutput();

	// Copy to output
	for(size_t ind = 0; ind < FRAMES_PER_BUFFER; ind++)
		for(size_t chan = 0; chan < CHANNELS; chan++)
			output[ind * CHANNELS + chan] = outputs[chan][ind];
}
