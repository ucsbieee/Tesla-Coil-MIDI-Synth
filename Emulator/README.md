# Emulated MIDI Synth

This folder contains code to patch/modify the software that runs on the microcontroller on the Tesla coil controller box so that it can run on a normal computer. There is also some glue code that uses [PortAudio](https://github.com/PortAudio/portaudio) and [libremidi](https://github.com/jcelerier/libremidi) to enable MIDI input and sound output. This allows songs to be more easily developed without needing a physical controller box plugged in (especially if you want to use the CC-controlled effects channels).

## Building

Prerequisites
 * [`portaudiocpp`](https://github.com/PortAudio/portaudio)
 * [`fftw`](https://www.fftw.org)
 * [`libremidi`](https://github.com/jcelerier/libremidi)
 * [`SDL2`](https://www.libsdl.org) (and `SDL2_ttf`)
 * GNU `make`
 * `patch`
 * C++17 compiler

You may need to modify the makefile slightly to get it to find the libraries on your system (it uses `pkg-config` by default) or to select a different backend for `libremidi` depending on your operating system.

Running `make` will take the code from the main firmware in this repo and apply some patches to remove MCU-specific code.

## Using

When the program is run, it will ask you for audio input and output devices and a MIDI input device. Set the audio input device to a guitar or loopback source, output device to your headphones, and the MIDI input to a loopback device coming out of your DAW. It should respond exactly like the real MIDI controller since it's the exact same code. The sound is approximated using an FIR filter.

If run with the `--stereo` flag, the program will emulate two Tesla coil controllers, one panned to the left and the other to the right. The right one responds to MIDI channels starting at 5 instead of 1.

Use the arrow keys and return/space or mouse wheel and click to navigate the settings.

## Modifying

Significant modifications to the MCU firmware may require the patches to be re-done. Changes to files can be propagated back to the `.patch` files by running `make patches` after modifying the files produced by running `make build/patched/x.cpp`.

## Attribution

LCD font from [here](https://fontstruct.com/fontstructions/show/476121/lcd_dot_matrix_hd44780u).
