#include <algorithm>
#include <cstring>

#include "Coil.h"
#include "AudioEngine.h"

Coil::Coil(uint8_t MIDIbaseChannel, AudioOutputMode aoMode): aoMode(aoMode), _millis(0), midi(this), synth(this), voicesUpdating(0) {
	midi.MIDIbaseChannel = MIDIbaseChannel;
	memset(oscillators, 0, sizeof(oscillators));
	memset(voices, 0, sizeof(voices));
}

bool Coil::getNextSample() {
	// Logical OR all oscillators like the real controller
	bool osc_state = false;
	for(int x = 0; x < NVOICES; x++) {
		// Reset counter
		if(oscillators[x].counter >= oscillators[x].period)
			oscillators[x].counter %= std::max((uint64_t)1, oscillators[x].period);
		
		// Affect state
		osc_state |= (oscillators[x].counter < oscillators[x].pulseWidth);
		
		// Increment counter
		oscillators[x].counter += F_CPU/F_SAMP;
	}

	return osc_state;
}

void Coil::handleMIDI(const unsigned char *pass) {
	midi.handleMIDI(pass[0], pass[1], pass[2]);
}

void Coil::updateSynth() {
	_millis++;
	synth.updateSynth();
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
