#pragma once

#include <vector>
#include <stddef.h>

#include <fftw3.h>

class Convolution {
public:
	Convolution(const float *ir, size_t irLen, size_t bufLen);
	~Convolution();

	// Return input buffer
	float *getInput() const;

	// Return output buffer
	const float *getOutput();

protected:
	const float * const ir;
	const size_t irLen, bufLen;

	const float scale;

	// Contiguous and aligned input data
	float *inputBuffer;

	// Intermediate FFT results for convolving
	fftwf_complex *intermediate;

	// Output buffers to be summed to create continuous impulse response
	std::vector<float*> outputBuffers;
	size_t outputBufferInd;

	// Final summed output buffer
	float *finalBuffer;

	fftwf_plan forwardPlan, reversePlan;
};
