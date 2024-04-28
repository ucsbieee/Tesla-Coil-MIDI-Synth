#include "Convolution.h"

#include <cstring>
#include <cmath>

Convolution::Convolution(const float *ir, size_t irLen, size_t bufLen): ir(ir), irLen(irLen), bufLen(bufLen), scale(1.0f/sqrtf(irLen)) {
	inputBuffer = (float*)fftwf_malloc(irLen * sizeof(float));
	memset(inputBuffer, 0, irLen * sizeof(float));
	inputBufferInd = 0;

	intermediate = (fftwf_complex*)fftwf_malloc((irLen/2+1) * sizeof(fftwf_complex));

	outputBuffers.resize(irLen/bufLen);
	for(float *&buffer:outputBuffers) {
		buffer = (float*)fftwf_malloc(irLen * sizeof(float));
		memset(buffer, 0, irLen * sizeof(float));
	}
	outputBufferInd = 0;

	finalBuffer = (float*)fftwf_malloc(bufLen * sizeof(float));

	forwardPlan = fftwf_plan_dft_r2c_1d(irLen, inputBuffer, intermediate, 0);
	reversePlan = fftwf_plan_dft_c2r_1d(irLen, intermediate, outputBuffers.at(0), 0);
}

Convolution::~Convolution() {
	fftwf_free(inputBuffer);
	fftwf_free(intermediate);
	for(float *buffer:outputBuffers)
		fftwf_free(buffer);
	fftwf_free(finalBuffer);
	fftwf_destroy_plan(forwardPlan);
	fftwf_destroy_plan(reversePlan);
}

void Convolution::feedSample(float sample) {
	if(inputBufferInd >= bufLen)
		return;

	// Pre-scale input to keep energy normalized
	inputBuffer[inputBufferInd++] = sample * scale;
}

const float *Convolution::getOutput() {
	inputBufferInd = 0;

	// Compute FFT of incoming data
	fftwf_execute(forwardPlan);

	// Apply impulse response
	for(size_t i = 0; i < irLen/2+1; i++) {
		const float * const a = ir + 2*i;
		const fftwf_complex &b = intermediate[i];
		fftwf_complex result;
		result[0] = a[0]*b[0] - a[1]*b[1];
		result[1] = a[0]*b[1] + a[1]*b[0];
		intermediate[i][0] = result[0];
		intermediate[i][1] = result[1];
	}

	// Compute inverse FFT
	fftwf_execute_dft_c2r(reversePlan, intermediate, outputBuffers[outputBufferInd]);

	// Compute output
	memset(finalBuffer, 0, bufLen * sizeof(float));
	ssize_t ind = outputBufferInd;
	for(size_t i = 0; i < outputBuffers.size(); i++) {
		for(size_t j = 0; j < bufLen; j++)
			finalBuffer[j] += outputBuffers[ind][j + i*bufLen];

		ind--;
		if(ind < 0)
			ind = outputBuffers.size()-1;
	}

	// Rotate output buffers
	outputBufferInd++;
	if(outputBufferInd >= outputBuffers.size())
		outputBufferInd = 0;

	return finalBuffer;
}
