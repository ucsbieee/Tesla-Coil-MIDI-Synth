#include "BarChars.h"

const uint8_t left_bracket_empty[8] = {
  0b11111,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b11111
};

const uint8_t left_bracket_full[8] = {
  0b11111,
  0b10000,
  0b10111,
  0b10111,
  0b10111,
  0b10111,
  0b10000,
  0b11111
};

const uint8_t half_full[8] = {
  0b00000,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b00000
};

const uint8_t full[8] = {
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b00000
};

const uint8_t right_bracket_full[8] = {
  0b11111,
  0b00001,
  0b11101,
  0b11101,
  0b11101,
  0b11101,
  0b00001,
  0b11111
};

const uint8_t right_bracket_empty[8] = {
  0b11111,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b11111
};

const LCDchar BarChars[] = {
  {BAR_LEFT_EMPTY, left_bracket_empty},
  {BAR_LEFT_FULL, left_bracket_full},
  {BAR_HALF_FULL, half_full},
  {BAR_FULL, full},
  {BAR_RIGHT_FULL, right_bracket_full},
  {BAR_RIGHT_EMPTY, right_bracket_empty}
};
