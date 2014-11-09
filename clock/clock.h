#ifndef _NIXIE_H_
#define _NIXIE_H_

#include "Arduino.h"

#define SERIAL_BUFFER_SIZE 32

typedef union {
  uint8_t buffer[6];
  struct {
    uint8_t hours10;
    uint8_t hours1;
    uint8_t minutes10;
    uint8_t minutes1;
    uint8_t seconds10;
    uint8_t seconds1;
  } splitted;
} time_u;

typedef struct 
{
  uint8_t digits[6];
  uint8_t dots[6];
  uint8_t leds[18];
} frameBuffer_t;

enum clockMode { CLOCK_MODE_COUNTER, CLOCK_MODE_CLOCK, CLOCK_MODE_BIRTHDAY };

enum dotMode { DOT_MODE_CLASSIC, DOT_MODE_PROGRESSIVE, DOT_MODE_CHASE };

#endif
