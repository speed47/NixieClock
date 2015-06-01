#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#include "globals.h"
#include "printbuf.h"
#include "clock.h"
#include "ws2811.h"
#include "generators.h"

/* Configuration pins */

const int dot1Pin = 3;
const int dot2Pin = 4;
const int dot3Pin = 16;
const int dot4Pin = 17;
const int dot5Pin = 18;
const int dot6Pin = 19;

const int ledsPin = 12;

const int tube14Pin = 15;
const int tube25Pin = 22;
const int tube36Pin = 23;

// also declared in globals.h for use in other files
uint32_t uptime = 0;
frameBuffer_t frameBuffer;
char _printbuf[PRINT_BUFFER_SIZE];
config_t cfg = {
  .debug_level = PROJECT_DEBUG,
  .generator = &generator_clock,
  .rtc_compensate = 0,
  .countdown_target_millis = 0,
  .newyear_target = 0,
  .dot_mode = DOT_MODE_CHASE,
  .fading = 1,
  .want_transition_now = 0,
  .show_fps = 0,
  .show_time = 0,
};

char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferLen = 0;

// redefine this to avoid inclusion of a lot of teensy
// files we don't need
void yield(void) {}

void setup()
{
  // Init BT Serial
  serial_begin(BAUD2DIV(115200));
  serial_format(SERIAL_8N1);

#ifdef RTC_COMPENSATE
  // compensate RTC
  cfg.rtc_compensate = RTC_COMPENSATE;
  rtc_compensate(RTC_COMPENSATE);
#endif

  // Show last reset reason
  uint16_t resetReasonHw = RCM_SRS0;
  resetReasonHw |= (RCM_SRS1<<8);
  uint16_t mask = 1;
  out("starting up, last reset reason:");
  do
  {
    switch (mask & resetReasonHw)
    {
      //RCM_SRS0
      case 0x0001: out(" WakeUp"); break;
      case 0x0002: out(" LowVoltage");  break;
      case 0x0004: out(" LossOfClock"); break;
      case 0x0008: out(" LossOfLock"); break;
      //case 0x0010 reserved
      case 0x0020: out(" Watchdog"); break;
      case 0x0040: out(" ExtResetPin"); break;
      case 0x0080: out(" PowerOn"); break;
      //RCM_SRS1
      case 0x0100: out(" JTAG"); break;
      case 0x0200: out(" CoreLockup"); break;
      case 0x0400: out(" Software"); break;
      case 0x0800: out(" MDM_AP"); break;
      case 0x1000: out(" EZPT"); break;
      case 0x2000: out(" SACKERR"); break;
    }
  } while (mask <<= 1);
  out("\r\n");

  // Set PORTD as output
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(20, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(5, OUTPUT);

  // Dots
  pinMode(dot1Pin, OUTPUT);
  pinMode(dot2Pin, OUTPUT);
  pinMode(dot3Pin, OUTPUT);
  pinMode(dot4Pin, OUTPUT);
  pinMode(dot5Pin, OUTPUT);
  pinMode(dot6Pin, OUTPUT);

  // Multiplex
  pinMode(tube14Pin, OUTPUT);
  pinMode(tube25Pin, OUTPUT);
  pinMode(tube36Pin, OUTPUT);

  // Leds
  pinMode(ledsPin, OUTPUT);

  // Timezone
  setenv("TZ", TIMEZONE, 1);
  tzset();
  dbg1("timezone data is timezone=%ld daylight=%d name=%s (tz=%s)", _timezone, _daylight, _tzname[_daylight], TIMEZONE);

  // Init wait (pb with ws2811)
  dbg1("sleeping for ws2811 init");
  delay(1000);
  dbg1("up, entering main loop");
}

// Main loop
void loop()
{
  static uint32_t lastEnd = micros();
  static uint32_t nextFpsMark = lastEnd + 1000*1000;
  static uint16_t fps = 0;
  static uint32_t lastTime = getLocalTimeT();

  if (cfg.show_time && lastTime != getLocalTimeT())
  {
    lastTime = getLocalTimeT();
    out( printbuf("%lu\r\n", lastTime) );
  }
  if (lastEnd >= nextFpsMark)
  {
    uptime++;
    if (cfg.show_fps)
    {
      out( printbuf("uptime=%lu, fps=%d\r\n", uptime, fps) );
    }
    nextFpsMark = lastEnd + 1000 * 1000;
    fps = 0;
  }
  fps++;

  // Generate what to display
  cfg.generator();

  // Security: ensure that no nixie-dot is lit unless the nixie-digit is also lit (avoid too-high current)
  securityNixieDot();

  // Update Leds
  updateLeds(ledsPin, frameBuffer.leds, 6);

  // Update nixies dots
  updateDots();

  // Update nixies (3 loop, 1 for each multiplex)
  for(int i = 0; i < 3; i++)
  {
    // Wait until 1000µs => 7.5% blanking time, handle serial buffer during this time
    while(micros() - lastEnd < 1000)
    {
      if(serial_available())
      {
        unsigned char c = serial_getchar();
        //Serial.print(c);
        if(c == '\r' || c == '\n')
        {
          serialBuffer[serialBufferLen] = '\0';
          handleSerial(serialBuffer, serialBufferLen);
          serialBufferLen = 0;              
        }
        else
        {
          serialBuffer[serialBufferLen] = c;
          serialBufferLen = (serialBufferLen+1)%SERIAL_BUFFER_SIZE;
        }
      }
    }

    // Update nixie output multiplex i
    updateNixie(i);

    // Save current time
    lastEnd = micros();
  }  
}

// Output subs, handle dots, nixie
inline
void updateDots()
{
  digitalWriteFast(dot1Pin, frameBuffer.dots[0]);
  digitalWriteFast(dot2Pin, frameBuffer.dots[1]);
  digitalWriteFast(dot3Pin, frameBuffer.dots[2]);
  digitalWriteFast(dot4Pin, frameBuffer.dots[3]);
  digitalWriteFast(dot5Pin, frameBuffer.dots[4]);
  digitalWriteFast(dot6Pin, frameBuffer.dots[5]);
}

inline
void updateNixie(unsigned int frame)
{
  // Disable all tubes
  digitalWriteFast(tube14Pin, 0);
  digitalWriteFast(tube25Pin, 0);
  digitalWriteFast(tube36Pin, 0);

  // Blanking time, waiting some time to be sure tubes are off before changing number
  // Avoid ghosting
  delayMicroseconds(75);

  if(frame == 0)
  {
    // Display digit 1 & 4
    GPIOD_PDOR = (frameBuffer.digits[2]<<4)+(frameBuffer.digits[5]);

    // Enable Tube 1 & 4
    digitalWriteFast(tube14Pin, 1);
  }
  else if(frame == 1)
  {
    // Display digit 2 & 5
    GPIOD_PDOR = (frameBuffer.digits[1]<<4)+(frameBuffer.digits[4]);

    // Enable Tube 2 & 5
    digitalWriteFast(tube25Pin, 1);
  }
  else if(frame == 2)
  {
    // Display digit 3 & 6
    GPIOD_PDOR = (frameBuffer.digits[0]<<4)+(frameBuffer.digits[3]);

    // Enable Tube 3 & 6
    digitalWriteFast(tube36Pin, 1);
  }
}

uint32_t getTimestampFromString(char const* buffer, int len)
{
  uint32_t timestamp = 0;
  for (int i = 0; i < len; i++)
  {
    // missing check for uint overflow... but it's ok for good old unix timestamps
    timestamp *= 10;
    timestamp += buffer[i] - '0';
  }
  return timestamp;
}

inline
void securityNixieDot()
{
  for (int i = 0; i < 6; i++)
  {
    if (frameBuffer.dots[i] != 0 && frameBuffer.digits[i] > 9)
    {
      dbg1("securityNixieDot: shutting down dot ");//, i, DEC);
      frameBuffer.dots[i] = 0;
    }
  }
}

void setStructTmFromString(struct tm* tm, char const* buffer)
{
  tm->tm_mday = (buffer[0]  - '0') * 10 + (buffer[1]  - '0');
  tm->tm_mon  = (buffer[2]  - '0') * 10 + (buffer[3]  - '0') - 1;
  tm->tm_year = (buffer[4]  - '0') * 10 + (buffer[5]  - '0') + 100;
  tm->tm_hour = (buffer[6]  - '0') * 10 + (buffer[7]  - '0');
  tm->tm_min  = (buffer[8]  - '0') * 10 + (buffer[9]  - '0');
  tm->tm_sec  = (buffer[10] - '0') * 10 + (buffer[11] - '0');
  tm->tm_isdst= -1; // cf man 3 mktime() : DST/noDST autodetected from sys tz
}

int readInt(const char* buffer, int *result)
{
  int sign = 1;
  if (*buffer == '-')
  {
    sign = -1;
    buffer++;
  } 
  else if (*buffer == '+')
  {
    buffer++;
  }

  int value = 0;
  while (*buffer != '\0')
  {
    if (*buffer >= '0' && *buffer <= '9')
    {
      value *= 10;
      value += *buffer - '0';
    }
    else
    {
      return 0;
    }
    buffer++;
  }

  *result = value * sign;
  return 1;
}
  
// serialHandler
inline
void handleSerial(char const* buffer, int len)
{
  // ignore the 4 BT-182 result codes for now
  if (strncmp(buffer, "ERROR", 5) == 0)
  {
    ;
  }
  else if (strncmp(buffer, "CONNECT", 7) == 0)
  {
    ;
  }
  else if (strncmp(buffer, "DISCONNECT", 10) == 0)
  {
    ;
  }
  else if (strncmp(buffer, "OK", 2) == 0)
  {
    ;
  }
  else if ((*buffer == 'c' || *buffer == 'C') && len == 1)
  {
    cfg.generator = &generator_clock;
    out("mode set to CLOCK\r\n");
  }
  else if ((*buffer == 'o' || *buffer == 'O') && len == 1)
  {
    cfg.generator = &generator_counter;
    out("mode set to COUNTER\r\n");
  }
  else if ((*buffer == 'b' || *buffer == 'B') && len == 1)
  {
    cfg.generator = &generator_birthday;
    out("mode set to BIRTHDAY\r\n");
  }
  else if (*buffer == 'Y' && (len == 12 || len == 13))
  {
    buffer++;
    struct tm tm_target;
    if (len == 13)
    {
      //Ymmddyyhhmmss
      setStructTmFromString(&tm_target, buffer);
      cfg.newyear_target = mktime(&tm_target);
    }
    else
    {
      cfg.newyear_target = getTimestampFromString(buffer, 10);
      localtime_r(&cfg.newyear_target, &tm_target);
    }
    cfg.generator = &generator_newyear;
    out( printbuf("Clock mode set to NEWYEAR, counting down to: %lu aka %02d/%02d/%04d %02d:%02d:%02d\r\n",
      cfg.newyear_target, tm_target.tm_mday, tm_target.tm_mon+1, tm_target.tm_year+1900,
      tm_target.tm_hour, tm_target.tm_min, tm_target.tm_sec) );
  }
  else if ((*buffer == 'r' ||*buffer == 'R') && len == 1)
  {
    cfg.want_transition_now = 1;
    out("Asked for a new transition... NOW!\r\n");
  }
  else if ((*buffer == 'f' || *buffer == 'F') && len == 1)
  {
    cfg.show_fps = !cfg.show_fps;
    out( printbuf("Show FPS mode is %s\r\n", cfg.show_fps ? "ON" : "OFF") );
  }
  else if ((*buffer == 'a' || *buffer == 'A') && len == 1)
  {
    cfg.fading = !cfg.fading;
    out( printbuf("Clock fading mode is %s\r\n", cfg.fading ? "ON" : "OFF") );
  }
  else if ((*buffer == 'm' ||*buffer == 'M') && len == 1)
  {
    cfg.show_time = !cfg.show_time;
    out( printbuf("Show TIME mode is %s\r\n", cfg.show_time ? "ON" : "OFF") );
  }
  else if ((*buffer == 't' || *buffer == 'T') && len == 7)
  {
    buffer++;
    // first; get the current date
    struct tm tm_target;
    time_t newTime = rtc_get(); //FIXME
    localtime_r(&newTime, &tm_target);
    // then, change the time part
    tm_target.tm_hour = (buffer[0]-'0')*10 + (buffer[1]-'0');
    tm_target.tm_min  = (buffer[2]-'0')*10 + (buffer[3]-'0');
    tm_target.tm_sec  = (buffer[4]-'0')*10 + (buffer[5]-'0');
    // convert that back to time_t and set the rtc
    newTime = mktime(&tm_target);
    rtc_set(newTime);
    out( printbuf("Time set to timestamp=%ld aka %02d/%02d/%04d %02d:%02d:%02d\r\n",
      newTime, tm_target.tm_mday, tm_target.tm_mon+1, tm_target.tm_year+1900,
      tm_target.tm_hour, tm_target.tm_min, tm_target.tm_sec) );
  }
  else if ((*buffer == 'd' ||*buffer == 'D') && (len == 12 || len == 13))
  {
    buffer++;
    struct tm tm_target;
    time_t newTime;
    if (len == 13)
    {
      //Dmmddyyhhmmss
      setStructTmFromString(&tm_target, buffer);
      newTime = mktime(&tm_target);
    }
    else
    {
      newTime = getTimestampFromString(buffer, 10);
      localtime_r(&newTime, &tm_target);
    }
    rtc_set(newTime);
    localtime_r(&newTime, &tm_target);
    out( printbuf("timezone=%ld daylight=%d\n", _timezone, _daylight) );
    out( printbuf("Time set to timestamp=%ld aka %02d/%02d/%04d %02d:%02d:%02d\r\n",
      newTime, tm_target.tm_mday, tm_target.tm_mon+1, tm_target.tm_year+1900,
      tm_target.tm_hour, tm_target.tm_min, tm_target.tm_sec) );
  }
  else if ((*buffer == 'w' || *buffer == 'W') && len == 5)
  {
    buffer++;
    uint32_t countdown_seconds = (buffer[0]-'0') * 10 * 60 + (buffer[1]-'0') * 60 + (buffer[2]-'0') * 10 + (buffer[3]-'0');;
    out( printbuf("Countdown for %lu seconds\r\n", countdown_seconds) );
    // FIXME: millis() reset not taken into account. tocheck also : uint32 overflow
    cfg.countdown_target_millis = millis() + countdown_seconds * 1000;
    cfg.generator = &generator_countdown;
  }
  else if ((*buffer == 'i' || *buffer == 'I')  && len == 1)
  {
    time_t rtc = rtc_get(); //FIXME
    struct tm tm_utc;
    struct tm tm_local;
    localtime_r(&rtc, &tm_local);
    gmtime_r(&rtc, &tm_utc);
    out("\r\nNixieClock git." EXPAND2STR(GIT_REVISION) "." EXPAND2STR(GIT_DIRTY) "\r\n");
    out("Built on " EXPAND2STR(BUILD_TIME) "\r\n");
    out("With compiler v" __VERSION__ "\r\n");
    out( printbuf("RTC compensation is %d\r\n", cfg.rtc_compensate) );
    out( printbuf("RTC current raw value is %lu\r\n", rtc) );
    out( printbuf("Uptime is %s\r\n", seconds2duration(uptime)) );
    out( printbuf("Current TZ is %s (offset %lds)\r\n",
      _tzname[(tm_local.tm_isdst == 0 || tm_local.tm_isdst == 1) ? tm_local.tm_isdst : 0],
      _timezone));
    if (_daylight)
    {
      out( printbuf("This TZ is DST-aware (currently %sactive)\r\n", tm_local.tm_isdst == 1 ? "" : "NOT ") );
    }
    else
    {
      out( printbuf("This TZ is NOT DST-aware\r\n") );
    }
    out( printbuf("UTC  : %02d/%02d/%04d %02d:%02d:%02d\r\n",
      tm_utc.tm_mday, tm_utc.tm_mon+1, tm_utc.tm_year+1900,
      tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec) );
    out( printbuf("Local: %02d/%02d/%04d %02d:%02d:%02d\r\n",
      tm_local.tm_mday, tm_local.tm_mon+1, tm_local.tm_year+1900,
      tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec) );
    out( printbuf("Teensy core is running at %d MHz\r\n", F_CPU / 1000000) );
  }
  else if ((*buffer == 'r' || *buffer == 'R') && len > 1)
  {
    buffer++;
    int value;
    if (readInt(buffer, &value) == 1)
    {
      out( printbuf("RTC compensation value changed from %d to %d\r\n", cfg.rtc_compensate, value) );
      cfg.rtc_compensate = value;
      rtc_compensate(value);
    }
    else
    {
      out("parsing error\r\n");
    }
  }
  else if ((*buffer == 'e' || *buffer == 'E') && len == 1)
  {
    CPU_RESTART; // soft reset
    delay(1000); // justin case
    out("BUG: reboot failed!?\r\n"); // never reached
  }
  else if ((*buffer == 'g' || *buffer == 'G') && len == 2)
  {
    buffer++;
    int value = *buffer - '0';
    if (value < 0 || value > 3)
    {
      out( printbuf("Invalid value %d, expected one of 0 1 2 3\r\n", value) );
    }
    else if (value > PROJECT_DEBUG)
    {
      out( printbuf("Can't set debug value to %d, maximum compiled-in value is %d\r\n", value, PROJECT_DEBUG) );
    }
    else
    {
      out( printbuf("Debug level changed from %d to %d\r\n", cfg.debug_level, value) );
      cfg.debug_level = value;
    }
  }
  else if (len > 0)
  {
    out("\r\nUnknown cmd <");
    out(buffer);
    out(">. Supported cmds are:\r\n"
                 "Time setup: [T]HHMMSS or [D]<UNIXTAMP> or [D]DDMMYYHHMMSS\r\n"
                 "Set RTC compensation: [R]<value>\r\n"
                 "Toggle options: show [F]ps, show ti[M]e, f[A]ding\r\n"
                 "Actions: t[R]ansition now, [I]nfo, r[E]boot\r\n"
                 "Simple modes: [B]irthday, [C]lock, c[O]unter\r\n"
                 "Complex modes:\r\n"
                 "- new year: [Y]<UNIXTAMP> or [Y]DDMMYYHHMMSS\r\n"
                 "- countdown: [W]MMSS, e.g. D9000 for 90 minutes\r\n");
    // TODO: be able to change debug mode on the fly
  }
}

// caller should not free() the returned pointer
char *seconds2duration(uint32_t seconds)
{
  static char buffer[20];
  int days    = seconds / 86400;
  seconds    -= days    * 86400;
  int hours   = seconds / 3600;
  seconds    -= hours   * 3600;
  int minutes = seconds / 60;
  seconds    -= minutes * 60;

  if (days > 0)
  {
    sniprintf(buffer, 20, "%dday%s+%02dh%02dm%02lus", days, (days == 1 ? "" : "s"), hours, minutes, seconds);
  }
  else if (hours > 0)
  {
    sniprintf(buffer, 20, "%dh%02dm%02lus", hours, minutes, seconds);
  }
  else if (minutes > 0)
  {
    sniprintf(buffer, 20, "%dm%02lus", minutes, seconds);
  }
  else
  {
    sniprintf(buffer, 20, "%lus", seconds);
  }
  return buffer;
}


