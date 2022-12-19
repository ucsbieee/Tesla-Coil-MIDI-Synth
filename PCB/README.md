# PCB

## Hacking
There are twelve extra pins on the board marked "Expansion," and they're there so you can add extra inputs or whatever you want to the board. Three are power (5V, 3.3V, GND), and seven go to digital/ADC pins on the MCU. The Arduino board definition is based off of the DUE, so take a look at the [DUE schematic](https://www.arduino.cc/en/uploads/Main/arduino-Due-schematic.pdf) or [variant.cpp](https://github.com/ucsbieee/ArduinoCore-SAM3A4C/blob/master/variants/arduino_due_x/variant.cpp) to see which Arduino pins they map to. These could be good for adding physical controls/music input of some kind.

There's also an input that is ORed with the voice signals before being sent to the outputs, and a 3.3V logic level copy of that combined output signal available. You can use these to make use of the output circuitry to control the Tesla coil with your own signals, combine voices from multiple interrupter boards (chaos), or whatever else you can think of.

## Erase and Reset Buttons
Pressing the "Erase" button on the PCB immediately deletes the firmware. Don't push it unless you intend to. This microcontroller has the SAM-BA bootloader included in ROM which is activated when the chip is blank, causing it to enumerate as a USB serial device and accept new firmware. If you ever get the board into a state where you can't program it, hit the erase button then reset and that should fix it. After uploading new firmware to a blank chip, you may need to re-power (instead of reset) the board. If it wasn't blank, you usually can just hit the reset button to make it run the new firmware.

If you accidentally nuked the chip, there's a firmware binary file in this repo and you can upload it using [bossa](https://github.com/shumatech/BOSSA/releases).

## If you want to make more

There should be a few extra unpopulated boards in the lab somewhere. See [BOM.csv](https://github.com/ucsbieee/Tesla-Coil-MIDI-Synth/blob/master/PCB/BOM.csv) for a list of parts you'll need. Parts without a DigiKey part number are generic stuff. Most resistors and caps are 0603, with a few 0805 (see footprint column).

You'll also likely want to use a solder paste stencil, especially for the BGA microcontroller. A file for the stencil is in the Gerber folder.

If you guys ever build a Tesla Coil symphony with a bunch of these things, please invite me back, I want to see :)
