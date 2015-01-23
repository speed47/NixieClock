#include "printbuf.h"
#include "globals.h"
#include <stdio.h>
#include <kinetis.h>
#include <HardwareSerial.h>

// buffer used and returned by printbuf() and printbufva()
char _print_buffer[PRINT_BUFFER_SIZE];

// used as sprintf() (return a char *) but buffer doesn't need to be specified
char *printbuf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsniprintf(_print_buffer, PRINT_BUFFER_SIZE, format, args);
  va_end(args);

  return _print_buffer;
}

// same as printbuf() but with already initialized va_list (used by dbg() func below)
char *printbufva(const char *format, va_list args)
{
  vsniprintf(_print_buffer, PRINT_BUFFER_SIZE, format, args);
  return _print_buffer;
}

// also see dbg1/dbg2/dbg3 macros
void dbg(int level, const char *format, ...)
{
  // if currently configured debug level is below this message debug level, skip it
  if (cfg.debug_level < level)
  {
    return;
  }
  va_list args;

  serial_print( printbuf("[%lu.%02d] dbg%d: ", RTC_TSR, RTC_TPR*100/32768, level) );
  va_start(args, format);
  serial_print( printbufva(format, args) );
  va_end(args);
  serial_print( "\n" );
}


