#ifndef KNOB_H
#define KNOB_H

#include <inttypes.h>

namespace Knob {

#define BUTTON_DEBOUNCE 100 // ms
#define ENCODER_ACCELERATION 60 // ms per decrement

void initEncoder();
void updateKnob();

}

#endif
