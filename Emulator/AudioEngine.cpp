#include "AudioEngine.h"

#include <ratio>
#include <functional>
#include <cstring>

using namespace std;

AudioEngine::AudioEngine(list<Coil> &coils): coils(coils) {}

AudioEngine::~AudioEngine() {
	stopStream();
}

void AudioEngine::startStream() {
	// If running, do nothing
	if(generator) return;

	// Empty buffers
	while(inputBuffer.size())
		inputBuffer.pop();
	while(outputBuffer.size())
		outputBuffer.pop();

	sample = 0;
	lastUpdateSample = 0;
	wakeTime = chrono::steady_clock::now();
	runGenerator = true;

	generator.reset(new thread{bind(&AudioEngine::generatorThread, this)});
}

void AudioEngine::stopStream() {
	if(!generator) return;
	runGenerator = false;
	generator->join();
	generator.reset();
}

int AudioEngine::genAudio(const void *input, void *_output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {

	AudioEngine &engine = *static_cast<AudioEngine*>(userData);
	float *output = static_cast<float*>(_output);

	frameCount *= 2;

	for(; frameCount && engine.outputBuffer.size(); frameCount--)
		*output++ = engine.outputBuffer.pop();

	// Buffer underrun
	for(; frameCount; frameCount--)
		*output++ = 0;

	return paContinue;
}

void AudioEngine::generatorThread() {
	auto lastTime = chrono::steady_clock::now();

	while(runGenerator) {
		auto now = chrono::steady_clock::now();
		auto dt = chrono::duration_cast<chrono::duration<long long, ratio<1, F_SAMP>>>(now - lastTime);
		lastTime = now;

		auto nsamples = dt.count();

		do {
			// If there isn't enough output samples, rush to make a bunch of input ones
			if(outputBuffer.size() < MIN_FIFO_SIZE)
				nsamples = max(nsamples, (long long)(FRAMES_PER_BUFFER - inputBuffer.size()/CHANNELS));

			for(; nsamples > 0; nsamples--, sample++) {
				genInput();

				// Update synth state every 1ms
				if(sample - lastUpdateSample >= F_SAMP/1000) {
					for(auto &coil:coils)
						coil.updateSynth();
					lastUpdateSample = sample;
				}
			}

			// Generate output samples if needed
			genOutput();
		} while(outputBuffer.size() < MIN_FIFO_SIZE);

		wakeTime += wakePeriod;
		this_thread::sleep_until(wakeTime);
	}
}

void AudioEngine::genInput() {
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

	// If buffer is too full, drop this sample
	if(inputBuffer.size() >= MAX_FIFO_SIZE)
		return;

	inputBuffer.push(lout);
	inputBuffer.push(rout);
}

void AudioEngine::genOutput() {
	// Make sure there is enough data
	if(outputBuffer.size() + FRAMES_PER_BUFFER*CHANNELS > MAX_FIFO_SIZE)
		return;

	if(inputBuffer.size() < FRAMES_PER_BUFFER*CHANNELS)
		return;

	// TODO: process
	for(size_t count = FRAMES_PER_BUFFER*CHANNELS; count; count--)
		outputBuffer.push(inputBuffer.pop());
}
