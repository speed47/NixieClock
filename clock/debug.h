#ifndef _DEBUG_H
#define _DEBUG_H

#include "debug.h"

#define DEBUG_BUFFER_SIZE 128

#define DEBUG_LEVEL_1 1
#define DEBUG_LEVEL_2 2
#define DEBUG_LEVEL_3 3

// should be defined via ./configure, but if not, here is a default
#ifndef NIXIE_DEBUG
# define NIXIE_DEBUG DEBUG_LEVEL_1
#endif

#define DEBUG_OUTPUT_BT Serial1
#define DEBUG_OUTPUT_USB Serial

#ifndef DEBUG_OUTPUT
# define DEBUG_OUTPUT DEBUG_OUTPUT_BT
#endif

extern char printbuf[DEBUG_BUFFER_SIZE];

#if NIXIE_DEBUG >= DEBUG_LEVEL_1
# define dbg1(...) { \
  sniprintf(printbuf, DEBUG_BUFFER_SIZE, __VA_ARGS__); \
  DEBUG_OUTPUT.print("dbg1: "); \
  DEBUG_OUTPUT.println(printbuf); \
}
#else
# define dbg1(...) ;
#endif

#if NIXIE_DEBUG >= DEBUG_LEVEL_2
# define dbg2(...) { \
  sniprintf(printbuf, DEBUG_BUFFER_SIZE, __VA_ARGS__); \
  DEBUG_OUTPUT.print("dbg2: "); \
  DEBUG_OUTPUT.println(printbuf); \
}
#else
# define dbg2(...) ;
#endif

#if NIXIE_DEBUG >= DEBUG_LEVEL_3
# define dbg3(...) { \
  sniprintf(printbuf, DEBUG_BUFFER_SIZE, __VA_ARGS__); \
  DEBUG_OUTPUT.print("dbg3: "); \
  DEBUG_OUTPUT.println(printbuf); \
}
#else
# define dbg3(...) ;
#endif

#endif
