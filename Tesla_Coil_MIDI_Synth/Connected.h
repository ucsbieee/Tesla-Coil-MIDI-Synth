#pragma once

#include <inttypes.h>

namespace Connected {

extern uint16_t lastFrameNumber;
extern int8_t missedFrameCount;

void checkConnected();

}
