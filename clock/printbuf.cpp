#include <printbuf.h>
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

// same as printbuf() but with already initialized va_list (used by dbg* funcs)
char *printbufva(const char *format, va_list args)
{
  vsniprintf(_print_buffer, PRINT_BUFFER_SIZE, format, args);
  return _print_buffer;
}

void dbg(int level, const char *format, ...)
{
  va_list args;

  DEBUG_OUTPUT.print( printbuf("[%lu.%03d] dbg%d: ", RTC_TSR, RTC_TPR/32768, level) );
  va_start(args, format);
  DEBUG_OUTPUT.println( printbufva(format, args) );
  va_end(args);
}


