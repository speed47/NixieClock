#include "globals.h"
#include "printbuf.h"

#include <stdio.h>
#include <kinetis.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <usb_serial.h>
#include <core_pins.h>

#define SERIAL_OUT_USB
#define SERIAL_OUT_BT

// buffer used and returned by printbuf() and printbufva()
char _print_buffer[PRINT_BUFFER_SIZE];

// used as sprintf() (return a char *) but buffer doesn't need to be specified
char *printbuf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsnprintf(_print_buffer, PRINT_BUFFER_SIZE, format, args);
  va_end(args);

  return _print_buffer;
}

// same as printbuf() but with already initialized va_list (used by dbgva() func below)
char *printbufva(const char *format, va_list args)
{
  vsnprintf(_print_buffer, PRINT_BUFFER_SIZE, format, args);
  return _print_buffer;
}

// wrapper to usb_serial_write
inline int usb_serial_print(const char *buffer)
{
  return usb_serial_write((const void *)buffer, strlen(buffer));
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
  va_start(args, format);
  dbgva(level, format, args);
  va_end(args);
}

// same as dbg() but with already initialized va_list (used by dbg1/2/3 macros)
void dbgva(int level, const char *format, va_list args)
{
  unsigned long now = millis();
#ifdef SERIAL_OUT_USB
  usb_serial_print( printbuf("[%5lu.%03d] dbg%d: ", now/1000, (int)((now/1000.0 - now/1000) * 1000), level) );
//usb_serial_print( printbuf("[%5lu.%02lu] dbg%d: ", rtc_get(), RTC_TPR*100/32768, level) ); 
  usb_serial_print( printbufva(format, args) );
  usb_serial_print( "\r\n" );
#endif
#ifdef SERIAL_OUT_BT
      serial_print( printbuf("[%5lu.%03d] dbg%d: ", now/1000, (int)((now/1000.0 - now/1000) * 1000), level) );
//    serial_print( printbuf("[%5lu.%02lu] dbg%d: ", rtc_get(), RTC_TPR*100/32768, level) ); 
      serial_print( printbufva(format, args) );
      serial_print( "\r\n" );
#endif
}

void out(const char *s)
{
  usb_serial_print(s);
  serial_print(s);
}


