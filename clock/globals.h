#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <stdint.h>

#define DEBUG_LEVEL_1 1
#define DEBUG_LEVEL_2 2
#define DEBUG_LEVEL_3 3

#define DEBUG DEBUG_LEVEL_1

#if DEBUG >= DEBUG_LEVEL_1
# define dbg1(s) { Serial1.print("Debug1: "); Serial1.println(s); };
#else
# define dbg1(s) ;
#endif
#if DEBUG >= DEBUG_LEVEL_2
# define dbg2(s) { Serial1.print("Debug2: "); Serial1.println(s); };
#else
# define dbg2(s) ;
#endif
#if DEBUG >= DEBUG_LEVEL_3
# define dbg3(s) { Serial1.print("Debug3: "); Serial1.println(s); };
#else
# define dbg3(s) ;
#endif
#define dbg(s) dbg1(s)

typedef struct 
{
  uint8_t digits[6];
  uint8_t dots[6];
  uint8_t leds[18];
} frameBuffer_t;

enum dotMode {
	DOT_MODE_CLASSIC,
	DOT_MODE_PROGRESSIVE,
	DOT_MODE_CHASE
};
enum splitMode {
	SPLIT_SEC_TO_HOUR_MIN_SEC,
	SPLIT_MS_TO_MIN_SEC_CENTISEC,
};

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

