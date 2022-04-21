# UCSB IEEE Tesla Coil MIDI Synthesizer

This is an Arduino program to convert MIDI data to pulses to drive the Tesla Coil. The Arduino enumerates as a USB MIDI device and can be sent live play data or a recorded MIDI file.

This program is intended for the [Arduino Due](https://docs.arduino.cc/hardware/due), though it could probably be ported to other MCUs without too much effort (hard part is timers and USB stuff).

### Pins
The following pins should be ORed together to drive the fiber optic transmitter.
 1. PWM2
 2. A7
 3. A4
 4. PWM5
 5. PWM3
 6. PWM11

These pins are the six voices (could have nine with this MCU but they aren't broken out on the Arduino). These voices will respond to the following MIDI channels.

### MIDI Channels
 1. Clean (no effects)
 2. Effects
 3. Arpeggio and effects
 4. Drums

Standard MIDI pitch bend and aftertouch messages apply to the first three channels.

### Effects (CC number in parentheses)
 * Tremolo (rate = 102, depth = 92, delay = 103)
 * Vibrato (rate = 76, depth = 77, delay = 78)
 * ADSR (116, 117, 118, 119)

### Drum samples
 * Kick (36/C2)
 * Snare (40/E2)
 * Clap (39/D#2)
 * Tom (48/C3)
 * Closed hi-hat (42/F#2)
