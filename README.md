# UCSB IEEE Tesla Coil MIDI Synthesizer

This is an Arduino program to convert MIDI data to pulses to drive the Tesla Coil. The Arduino enumerates as a USB MIDI device and can be sent live play data or a recorded MIDI file.

This program was originally intended for the [Arduino Due](https://docs.arduino.cc/hardware/due) (see the old version in `Tesla_Coil_MIDI_Synth_DUE`). A circuit board has since been designed for it using the similar SAM3A4C MCU (see [this repo](https://github.com/ucsbieee/ArduinoCore-SAM3A4C) for an Arduino IDE board definition), which has a fiber optic output for the big DRSSTC and a BNC port for the small Tesla Coil. It also has isolated physical MIDI in/out ports for connecting other instruments or daisy-chaining more MIDI interrupters.

The device has six voices, which can each produce a single square wave tone at a variable duty cycle. They are ORed together to give a polyphonic effect.

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

### Drum samples
 * Kick (36/C2)
 * Snare (40/E2)
 * Clap (39/D#2)
 * Tom (48/C3)
 * Closed hi-hat (42/F#2)
