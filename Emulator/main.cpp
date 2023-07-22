#include <iostream>
#include <string>
#include <vector>
#include <utility>

#include <libremidi/libremidi.hpp>
#include <portaudiocpp/PortAudioCpp.hxx>

#include "Coil.h"

#define STEREO_SEPARATION 0.25

using namespace std;

int ncoils = 1;
Coil *coils;

// Ask user to choose something from a list
int choose(const char *message, vector<pair<string, int>> choices) {
	int choice;
	do {
		printf("%s\n", message);
		for(size_t x = 0; x < choices.size(); x++)
			printf("  [%lu] %s\n", x+1, choices[x].first.c_str());
		cin >> choice;
	} while(choice <= 0 || choice > (ssize_t)choices.size());
	return choices[choice-1].second;
}

// Generate fixed stream parameters for a given PortAudio device
portaudio::StreamParameters desiredParameters(portaudio::Device &d) {
	return {portaudio::DirectionSpecificStreamParameters::null(),
	        portaudio::DirectionSpecificStreamParameters(d, 2, portaudio::FLOAT32, true, 0, NULL),
	        F_SAMP,
	        FRAMES_PER_BUFFER,
	        0};
}

// Generate audio samples (PortAudio callback)
int genaudio(const void *input, void *_output,
             unsigned long frameCount,
             const PaStreamCallbackTimeInfo* timeInfo,
             PaStreamCallbackFlags statusFlags,
             void *userData ) {
	
	float *output = (float*)_output;
	
	for(unsigned long x = 0; x < frameCount; x++) {
		float *ptr = output + 2*x;
		ptr[0] = 0;
		ptr[1] = 0;
		for(int y = 0; y < ncoils; y++) {
			float sample = coils[y].getNextSample();
			float lweight = 1, rweight = 1;
			
			if(coils[y].aoMode == Coil::LEFT)
				rweight = STEREO_SEPARATION;
			else if(coils[y].aoMode == Coil::RIGHT)
				lweight = STEREO_SEPARATION;
			
			ptr[0] += lweight * sample;
			ptr[1] += rweight * sample;
		}
	}
	
	return paContinue;
}

int main(int argc, char **argv) {
	
	// Parse args/print usage
	for(int x = 1; x < argc; x++) {
		string arg(argv[x]);
		if(arg == "--stereo") ncoils = 2;
		else {
			cout << "Usage: " << argv[0] << " [--stereo]" << endl;
			cout << "\t--stereo: Emulate two coils, one on each audio channel. The right coil responds to MIDI channels 5+." << endl;
		}
	}
	
	if(ncoils == 1)
		coils = new Coil[1]{{0, Coil::BOTH}};
	else if(ncoils == 2)
		coils = new Coil[2]{{0, Coil::LEFT}, {4, Coil::RIGHT}};
	
	// Init portaudio
	portaudio::AutoSystem autoSys;
	portaudio::System &sys = portaudio::System::instance();
	
	// Select audio output device
	vector<pair<string, int>> audioDeviceNames;
	for(portaudio::System::DeviceIterator i = sys.devicesBegin(); i != sys.devicesEnd(); i++)
		// Make sure this device supports the desired output parameters
		if(desiredParameters(*i).isSupported())
			audioDeviceNames.emplace_back(i->name(), i->index());
	
	portaudio::Device &device = sys.deviceByIndex(choose("Choose audio output device", audioDeviceNames));
	
	portaudio::CFunCallbackStream stream(desiredParameters(device), &genaudio, NULL);
	
	// Select MIDI input device
	libremidi::midi_in midi;
	vector<pair<string, int>> midiDeviceNames;
	for(unsigned int x = 0; x < midi.get_port_count(); x++)
		midiDeviceNames.emplace_back(midi.get_port_name(x), x);
	
	midi.open_port(choose("Choose MIDI input device", midiDeviceNames));
	
	midi.set_callback([](const libremidi::message& message) {
		unsigned char pass[3] = {0};
		switch(message.size()) {
			case 0:
				return;
			default:
			case 3:
				pass[2] = message[2];
			case 2:
				pass[1] = message[1];
			case 1:
				pass[0] = message[0];
				break;
		}
		for(int x = 0; x < ncoils; x++)
			coils[x].handleMIDI(pass);
	});
	
	stream.start();
	
	// Run forever...
	while(true) sys.sleep(1e6);
	
	delete coils;
	
	return 0;
}
