# Emulated MIDI Synth

This folder contains code to patch/modify the software that runs on the microcontroller on the Tesla coil controller box so that it can run on a normal computer. There is also some glue code that uses [PortAudio](https://github.com/PortAudio/portaudio) and [libremidi](https://github.com/jcelerier/libremidi) to enable MIDI input and sound output. This allows songs to be more easily developed without needing a physical controller box plugged in (especially if you want to use the CC-controlled effects channels).

## Building

Prerequisites
 * [`portaudiocpp`](https://github.com/PortAudio/portaudio)
 * [`libremidi`](https://github.com/jcelerier/libremidi)
 * GNU `make`
 * `patch`
 * `sed`
 * C++17 compiler

You may need to modify the makefile slightly to get it to find your PortAudio install (it uses `pkg-config` by default) or to select a different backend for `libremidi` depending on your operating system.

Running `make` will take the code from the main firmware in this repo, apply some patches to remove MCU-specific code, and use `sed` to convert all global variables into C++ class members so that multiple instances of the synthesizer can be run at a time.

## Using

When the program is run, it will ask you for an audio output device and a MIDI input device. Set the audio output device to your headphones and the MIDI input to a loopback device coming out of your DAW. It should respond exactly like the real MIDI controller since it's the exact same code. The sound output is pure square waves, lacking "coloring" from the Tesla coil, but that could probably be approximated by a simple FIR filter...

If run with the `--stereo` flag, the program will emulate two Tesla coil controllers, one panned to the left and the other to the right. The right one responds to MIDI channels starting at 5 instead of 1.

## Modifying

Significant modifications to the MCU firmware may require the patches to be re-done. Changes to files can be propagated back to the .patch files by running `make patches` after modifying the files produced by running `make build/patched/x.cpp`.
