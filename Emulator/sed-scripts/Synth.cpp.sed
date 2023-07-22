# sed script for editing Synth.cpp

# Transform global variable declarations into initializations inside a constructor
s/^[^ 	].+[ 	]+(.+=.+;)$/\1/

# Transform references into global MIDI namespace into references into associated MIDI class instance
s/MIDI::([a-z])/coil->midi.\1/g

# Transform references into global Voices namespace into references into parent Coil class
s/Voice::([a-z])/coil->\1/g

# Prepend class name to each member function
s/^([^ 	]+[ 	]+)([^ 	]+\()/\1Synth::\2/

# Replace calls to updateWidth and updatePeriod with calls into Coil class
s/updateWidth/coil->updateWidth/g
s/updatePeriod/coil->updatePeriod/g

# Call millis() function inside Coil class
s/millis\(\)/coil->millis\(\)/g
