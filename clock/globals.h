#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <stdint.h>

#define DEBUG 1
#ifdef DEBUG
#define dbg(s) { Serial1.print("Debug: "); Serial1.println(s); };
#else
#define dbg(s) ;
#endif

typedef struct 
{
  uint8_t digits[6];
  uint8_t dots[6];
  uint8_t leds[18];
} frameBuffer_t;

enum dotMode { DOT_MODE_CLASSIC, DOT_MODE_PROGRESSIVE, DOT_MODE_CHASE };
enum splitMode { SPLIT_HMS, SPLIT_MSC };

extern frameBuffer_t frameBuffer;

typedef struct
{
  // pointer to the generator function that will be used
  void (*generator)(void);
  // config for generator_countdown:
  unsigned long countdown_ms;
  // config for generator_newyear:
  unsigned int newyear_target;
  // config for generator_clock:
  dotMode dot_mode;
  int want_transition_now;
  // other stuff
  int show_fps; 
} config_t;

extern config_t cfg;

#endif

