#include "AudioEngine.h"

#include <algorithm>
#include <cassert>

using namespace std;

AudioEngine::AudioEngine(list<Coil> &coils): coils(coils), sample(0), lastUpdateSample(0) {
	for(size_t chan = 0; chan < CHANNELS; chan++)
		conv.emplace_back(ir, IR_LENGTH, FRAMES_PER_BUFFER);
}

portaudio::StreamParameters AudioEngine::desiredInputParameters(portaudio::Device &d) {
	return {portaudio::DirectionSpecificStreamParameters(d, CHANNELS, portaudio::FLOAT32, true, 0, NULL),
	        portaudio::DirectionSpecificStreamParameters::null(),
	        F_SAMP,
	        FRAMES_PER_BUFFER,
	        0};
}

portaudio::StreamParameters AudioEngine::desiredOutputParameters(portaudio::Device &d) {
	return {portaudio::DirectionSpecificStreamParameters::null(),
	        portaudio::DirectionSpecificStreamParameters(d, CHANNELS, portaudio::FLOAT32, true, 0, NULL),
	        F_SAMP,
	        FRAMES_PER_BUFFER,
	        0};
}

int AudioEngine::inputCallback(const void *_input, void *output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo *timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData) {

	AudioEngine &engine = *static_cast<AudioEngine*>(userData);
	const float *input = static_cast<const float*>(_input);

	frameCount *= CHANNELS;

	for(; frameCount && engine.inputBuffer.size() < MAX_FIFO_SIZE; frameCount--)
		engine.inputBuffer.push(*input++);

	return paContinue;
}

int AudioEngine::outputCallback(const void *input, void *_output,
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

		if(inputBuffer.size() >= CHANNELS)
			for(unsigned int chan = 0; chan < CHANNELS; chan++)
				inputSamples[chan] = inputBuffer.pop();

		for(auto &coil:coils) {
			float lweight = 1, rweight = 1;
			float inputSample;

			if(coil.aoMode == Coil::LEFT) {
				rweight = STEREO_SEPARATION;
				inputSample = inputSamples[0];
			} else if(coil.aoMode == Coil::RIGHT) {
				lweight = STEREO_SEPARATION;
				inputSample = inputSamples[1];
			} else // Mono
				inputSample = (inputSamples[0] + inputSamples[1]) / 2;

			// MIDI voices
			float sample = coil.getNextSample();

			// Processed audio
			sample += min(max(coil.audio.processSample(inputSample * 0x7FFF) / (float)(F_CPU/NOM_SAMPLE_RATE), -1.0f), 1.0f);

			const float alpha = 0.0002;
			coil.lcd.averageVolume = coil.lcd.averageVolume * (1-alpha) + sample * alpha;

			sample *= VOLUME;

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
