#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <utility>
#include <memory>

#include <libremidi/libremidi.hpp>
#include <portaudiocpp/PortAudioCpp.hxx>

#include "Coil.h"
#include "AudioEngine.h"
#include "LiquidCrystal.h"
#include "Version.h"

using namespace std;

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

int main(int argc, char **argv) {

	int ncoils = 1;

	// Parse args/print usage
	for(int x = 1; x < argc; x++) {
		string arg(argv[x]);
		if(arg == "--stereo") ncoils = 2;
		else {
			cout << "UCSB IEEE Tesla Coil Interrupter Emulator " VERSION
#ifndef RELEASE_BUILD
			"-dev"
#endif
			<< endl;
			cout << "Usage: " << argv[0] << " [--stereo]" << endl;
			cout << "\t--stereo: Emulate two coils, one on each audio channel. The right coil responds to MIDI channels 5+." << endl;
			return 1;
		}
	}

	// Initialize SDL
	LiquidCrystal::initialize();

	// Create Coil instances
	list<Coil> coils;
	if(ncoils == 1)
		coils.emplace_back("Tesla Coil Controller", 0, Coil::BOTH);
	else if(ncoils == 2) {
		coils.emplace_back("Tesla Coil Controller (Left)", 0, Coil::LEFT);
		coils.emplace_back("Tesla Coil Controller (Right)", 4, Coil::RIGHT);
	}

	// Create audio engine
	AudioEngine engine(coils);

	// Init portaudio
	portaudio::AutoSystem autoSys;
	portaudio::System &sys = portaudio::System::instance();

	// Select audio output device
	vector<pair<string, int>> deviceNames;
	auto filterDevices = [&](auto desiredParameters) {
		for(portaudio::System::DeviceIterator i = sys.devicesBegin(); i != sys.devicesEnd(); i++)
			// Make sure this device supports the desired parameters
			if(desiredParameters(*i).isSupported())
				deviceNames.emplace_back(i->name(), i->index());
	};

	filterDevices(AudioEngine::desiredOutputParameters);
	portaudio::Device &outputDevice = sys.deviceByIndex(choose("Choose audio output device", deviceNames));
	portaudio::CFunCallbackStream outputStream(AudioEngine::desiredOutputParameters(outputDevice), &AudioEngine::outputCallback, &engine);

	deviceNames.clear();
	deviceNames.emplace_back("None", -1);
	filterDevices(AudioEngine::desiredInputParameters);
	const int inputDeviceInd = choose("Choose audio input device", deviceNames);
	unique_ptr<portaudio::CFunCallbackStream> inputStream;
	if(inputDeviceInd >= 0) {
		portaudio::Device &inputDevice = sys.deviceByIndex(inputDeviceInd);
		inputStream.reset(new portaudio::CFunCallbackStream(AudioEngine::desiredInputParameters(inputDevice), &AudioEngine::inputCallback, &engine));
	}

	// Select MIDI input device
	libremidi::midi_in midi;
	deviceNames.clear();
	deviceNames.emplace_back("None", -1);
	for(unsigned int x = 0; x < midi.get_port_count(); x++)
		deviceNames.emplace_back(midi.get_port_name(x), x);

	const int chosenMidiDevice = choose("Choose MIDI input device", deviceNames);
	if(chosenMidiDevice >= 0) {
		midi.open_port(chosenMidiDevice);

		midi.set_callback([&](const libremidi::message& message) {
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
			for(auto &coil:coils)
				coil.handleMIDI(pass);
		});
	}

	outputStream.start();
	if(inputStream)
		inputStream->start();

	// Run until user quits
	while(LiquidCrystal::pollSDL())
		for(auto &coil:coils) {
			coil.lcd.update();
			coil.knob.update();
		}

	outputStream.stop();
	if(inputStream)
		inputStream->stop();

	coils.clear();

	LiquidCrystal::destroy();

	return 0;
}
