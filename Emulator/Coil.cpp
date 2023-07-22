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
	for(int x = 0; x < NVOICES; x++)
		osc_state |= (((sample << 16) % std::max(1ULL, oscillators[x].period)) < oscillators[x].pulseWidth);
	
	sample++;
	
	return osc_state;
}

void Coil::handleMIDI(const unsigned char *pass) {
	midi.handleMIDI(pass[0], pass[1], pass[2]);
}

// Multiply sample rate by 2^16 for better period resolution

void Coil::updateWidth(uint8_t chan, uint32_t pulseWidth) {
	oscillators[chan].pulseWidth = (uint64_t)pulseWidth * (F_SAMP << 16) / F_CPU;
}

void Coil::updatePeriod(uint8_t chan, uint32_t period) {
	oscillators[chan].period = (uint64_t)period * (F_SAMP << 16) / F_CPU;
}

unsigned long Coil::millis() {return _millis;}
