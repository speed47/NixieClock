#ifndef _PRINTBUF_H
#define _PRINTBUF_H

#define PRINT_BUFFER_SIZE 128

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

extern char _printbuf[PRINT_BUFFER_SIZE];

#define printbt(...) { \
  sniprintf(_printbuf, PRINT_BUFFER_SIZE, __VA_ARGS__); \
  Serial1.print(_printbuf); \
}

#if NIXIE_DEBUG >= DEBUG_LEVEL_1
# define dbg1(...) { \
  sniprintf(_printbuf, PRINT_BUFFER_SIZE, __VA_ARGS__); \
  DEBUG_OUTPUT.print("dbg1: "); \
  DEBUG_OUTPUT.println(_printbuf); \
}
#else
# define dbg1(...) ;
#endif

#if NIXIE_DEBUG >= DEBUG_LEVEL_2
# define dbg2(...) { \
  sniprintf(_printbuf, PRINT_BUFFER_SIZE, __VA_ARGS__); \
  DEBUG_OUTPUT.print("dbg2: "); \
  DEBUG_OUTPUT.println(_printbuf); \
}
#else
# define dbg2(...) ;
#endif

#if NIXIE_DEBUG >= DEBUG_LEVEL_3
# define dbg3(...) { \
  sniprintf(_printbuf, PRINT_BUFFER_SIZE, __VA_ARGS__); \
  DEBUG_OUTPUT.print("dbg3: "); \
  DEBUG_OUTPUT.println(_printbuf); \
}
#else
# define dbg3(...) ;
#endif

#endif
