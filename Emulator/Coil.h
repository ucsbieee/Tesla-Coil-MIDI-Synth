#pragma once

#include "MIDI.h"
#include "Synth.h"
#include "Audio.h"
#include "Knob.h"
#include "LCD.h"
#include "LiquidCrystal.h"
#include "Voice.h"

// Class representing a single MIDI controller/Tesla coil pair
class Coil {
public:
	enum AudioOutputMode {
		BOTH = 0b11,
		LEFT = 0b01,
		RIGHT = 0b10
	};

	Coil(const char *name, uint8_t MIDIbaseChannel = 0, AudioOutputMode aoMode = BOTH);
	
	// Call once for each subsequent sample
	bool getNextSample();

	// What audio mode has been assigned to this coil
	AudioOutputMode aoMode;
	
	// Pass MIDI into internal MIDI class
	void handleMIDI(const unsigned char *pass);

	// Update synth state
	void updateSynth();
	
	// Prevent copying (will break references of subclasses to us)
	Coil(const Coil&) = delete;

private:
	// Period, pulse width, and counter are in a timer
	typedef struct {
		uint64_t period;
		uint64_t pulseWidth;
		uint64_t counter;
	} Oscillator;

	// Emulate hardware oscillators in MCU
	Oscillator oscillators[NVOICES];
	
	// millis() time
	unsigned long _millis;

public:
	// Instances of synth components
	LiquidCrystal lc;
	Voices voices;
	MIDI midi;
	Synth synth;
	Audio audio;
	Knob knob;
	LCD lcd;
	volatile uint8_t voicesUpdating;
	
	// Functions to affect oscillator behavior
	void updateWidth(uint8_t chan, uint32_t pulseWidth);
	void updatePeriod(uint8_t chan, uint32_t period);
	
	// Replacement for Arduino millis() function
	unsigned long millis(void);
};
