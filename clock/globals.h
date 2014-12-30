#ifndef _GLOBALS_H
#define _GLOBALS_H

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

extern void (*generator)(void);
extern frameBuffer_t frameBuffer;
extern dotMode config_dot_mode;
extern unsigned long config_countdown_ms;
extern int config_want_transition_now;
extern unsigned int config_newyear_target;

#endif

