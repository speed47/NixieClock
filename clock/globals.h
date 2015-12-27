#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <stdint.h>
#include <time.h>
#include "core_pins.h"

#ifndef GIT_REVISION
# define GIT_REVISION none
#endif
#ifndef GIT_DIRTY
# define GIT_DIRTY unknown
#endif
#if !defined(BUILD_TIME) && defined(TIME_T)
# define BUILD_TIME TIME_T
#endif

#define _EXPAND2STR( str ) #str
#define EXPAND2STR( str ) _EXPAND2STR( str )

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

unsigned long getLocalTimeT(void);
long secondsDiffFromUTC(void);

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
  int debug_level;
  // pointer to the generator function that will be used
  void (*generator)(void);
  // configurable rtc drift correction
  int rtc_compensate;
  // config for generator_countdown:
  uint32_t countdown_target_millis;
  // config for generator_newyear:
  time_t newyear_target;
  // config for generator_clock:
  enum dotMode dot_mode;
  int fading;
  int want_transition_now;
  // other stuff
  int show_fps; 
  int show_time;
  // gps for sun algo
  float latitude;
  float longitude;
} config_t;

extern config_t cfg;
extern uint32_t uptime;

#endif

