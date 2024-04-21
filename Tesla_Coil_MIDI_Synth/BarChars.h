#ifndef BARCHARS_H
#define BARCHARS_H

#include <inttypes.h>

extern const uint8_t left_bracket_empty[8];
extern const uint8_t left_bracket_full[8];
extern const uint8_t half_full[8];
extern const uint8_t full[8];
extern const uint8_t right_bracket_full[8];
extern const uint8_t right_bracket_empty[8];

enum {
  BAR_LEFT_EMPTY,
  BAR_LEFT_FULL,
  BAR_HALF_FULL,
  BAR_FULL,
  BAR_RIGHT_FULL,
  BAR_RIGHT_EMPTY,
  BAR_EMPTY = 0xFE,
};

typedef struct {
  uint8_t value;
  const uint8_t *data;
} LCDchar;

#define NBARCHARS 6
extern const LCDchar BarChars[NBARCHARS];

#endif
