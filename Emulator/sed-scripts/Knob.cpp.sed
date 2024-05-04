# sed script for editing Knob.cpp

# Transform global variable declarations into initializations inside a constructor
s/^[^ 	].+[ 	]+(.+=.+;)$/\1/

# Transform references into global Synth namespace into references into associated Synth class instance
s/Synth::([a-z])/coil->synth.\1/g

# Transform references into global MIDI namespace into references into associated MIDI class instance
s/MIDI::([a-z])/coil->midi.\1/gi

# Transform references into global Voices namespace into references into parent Coil class
s/Voice::([a-z])/coil->\1/g

# Transform Arduino LiquidCrystal references to references into parent Coil class
s/lcd\./coil->lcd\./g

# Prepend class name to each member function
s/^([^ 	]+[ 	]+)([^ 	]+\()/\1Knob::\2/

# Call millis() function inside Coil class
s/millis\(\)/coil->millis\(\)/g
