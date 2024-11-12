# UCSB IEEE Tesla Coil MIDI Synthesizer

This is the firmware and design files for a device to convert MIDI data to pulses to drive a Tesla coil. It enumerates as a USB MIDI device and can be sent live play data or a recorded MIDI file. It also enumerates as a USB audio device and can be sent PCM audio for greater control or for live instrument use.

![photo](MIDI_Interrupter.jpg)

The circuit board is based off of the [Arduino Due](https://docs.arduino.cc/hardware/due). It uses the similar SAM3A4C MCU, and has a fiber optic output for the big DRSSTC and a BNC port for the small Tesla Coil. It also has isolated physical MIDI in/out ports for connecting other instruments or daisy-chaining more MIDI interrupters. See [ucsbieee/ArduinoCore-SAM3A4C](https://github.com/ucsbieee/ArduinoCore-SAM3A4C) for an Arduino IDE board definition.

The device has six voices, which can each produce a single square wave tone at a variable duty cycle. They are ORed together to give a polyphonic effect.

## Uploading Firmware

New firmware can be uploaded using the Arduino IDE and the aforementioned [board definition](https://github.com/ucsbieee/ArduinoCore-SAM3A4C), or the [latest release](https://github.com/ucsbieee/Tesla-Coil-MIDI-Synth/releases/latest) (`.bin` file) can be uploaded using [BOSSA](https://github.com/shumatech/BOSSA) ([available from Homebrew](https://formulae.brew.sh/formula/bossa)).

### Arduino Library Dependencies
Available from the Arduino IDE library manager
 * [MIDIUSB](https://github.com/arduino-libraries/MIDIUSB)
 * [DueFlashStorage](https://github.com/sebnil/DueFlashStorage) (only if `SAVE_BASE_MIDI` is enabled, which it is not by default)

## Emulator

There is also an emulator that can be compiled to run on a normal computer so that songs can be developed without the physical hardware.

## MIDI Information

### MIDI Channels
 1. Clean (no effects)
 2. Effects
 3. Arpeggio and effects
 4. Drums

Standard MIDI pitch bend and aftertouch messages apply to all channels.

### Effects (CC number in parentheses)
 * Tremolo (rate = 102, depth = 92, delay = 103)
 * Vibrato (rate = 76, depth = 77, delay = 78)
 * ADSR (116, 117, 118, 119)
 * Arpeggio rate (14)

### Drum Samples
 * Kick (36/C2)
 * Snare (40/E2)
 * Clap (39/D#2)
 * Tom (48/C3)
 * Closed hi-hat (42/F#2)

### Physical MIDI Ports
All MIDI messages that come in over USB or the MIDI IN port are sent out the MIDI OUT port. Additionally, all messages that come in over the MIDI IN port are sent back to the computer over USB. This allows daisy-chaining multiple controllers using the physical MIDI ports, and allows recording any inputs from the physical MIDI IN port to the computer. Be careful not to create infinite MIDI loops in your DAW because of these behaviors.

## Audio Processing Modes
 * **Predictive**: Predict how much energy will be in a pulse based on the filtered derivative near zero crossing.
 * **Pulse Energy**: Output a pulse after a delay (~5ms) with width proportional to the area of the input pulse above the noise gate level. Pulse width clamped to maximum setting.
 * **Schmitt**: Output on when audio exceeds the noise gate level, then only turn off again once audio falls below negative noise gate level (Schmitt trigger behavior). Pulse width clamped to maximum setting.
 * **Clamped Binary**: Output on when audio exceeds the noise gate level. Pulse width clamped to maximum setting.
 * **Binary**: Output on when audio exceeds the noise gate level.
 * **Binary DDT**: Output on when the derivative of the audio exceeds a certain threshold. Pulse width clamped to maximum setting.
 * **PWM**: Output 48kHz PWM with pulse width proportional to the sample value. Audio baseline level subtracted away.
 * **PWM DDT**: Output 48kHz PWM with pulse width proportional to the derivative of the audio.
