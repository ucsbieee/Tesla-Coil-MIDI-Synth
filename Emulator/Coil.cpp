#include <algorithm>

#include "Coil.h"

Coil::Coil(uint8_t MIDIbaseChannel, AudioOutputMode aoMode): aoMode(aoMode), midi(this), synth(this) {
	midi.MIDIbaseChannel = MIDIbaseChannel;
	sample = 0;
	nextSynthUpdate = 0;
	_millis = 0;
}

float Coil::getNextSample() {
	// Update synth state at 1kHz
	if(sample >= nextSynthUpdate) {
		synth.updateSynth();
		nextSynthUpdate = sample + F_SAMP/1000;
		_millis++;
	}
	
	// Logical OR all oscillators like the real controller
	bool osc_state = false;
	for(int x = 0; x < NVOICES; x++) {
		// Reset counter
		if(oscillators[x].counter >= oscillators[x].period)
			oscillators[x].counter %= std::max(1ULL, oscillators[x].period);
		
		// Affect state
		osc_state |= (oscillators[x].counter < oscillators[x].pulseWidth);
		
		// Increment counter
		oscillators[x].counter += F_CPU/F_SAMP;
	}
	
	sample++;
	
	return osc_state;
}

void Coil::handleMIDI(const unsigned char *pass) {
	midi.handleMIDI(pass[0], pass[1], pass[2]);
}

// The following functions emulate the behavior of the real timer update functions

void Coil::updateWidth(uint8_t chan, uint32_t pulseWidth) {
	uint64_t counter = oscillators[chan].counter;
	if(oscillators[chan].pulseWidth > counter && pulseWidth < counter)
		oscillators[chan].counter = 0;
	oscillators[chan].pulseWidth = pulseWidth;
}

void Coil::updatePeriod(uint8_t chan, uint32_t period) {
	oscillators[chan].period = period;
	if(oscillators[chan].counter > period)
		oscillators[chan].counter = 0;
}

unsigned long Coil::millis() {return _millis;}
