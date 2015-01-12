#include <time.h>
#include <stdint.h>

#include "globals.h"
#include "printbuf.h"
#include "clock.h"
#include "ws2811.h"
#include "generators.h"

#define WANT_FADING

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
  .generator = &generator_clock,
  .countdown_target_millis = 0,
  .newyear_target = 0,
  .dot_mode = DOT_MODE_CHASE,
  .want_transition_now = 0,
  .show_fps = 0,
  .show_time = 0,
};

char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferLen = 0;

void setup()
{
  // Init BT Serial
  Serial1.begin(115200, SERIAL_8N1);

#ifdef RTC_COMPENSATE
  // compensate RTC
  rtc_compensate(RTC_COMPENSATE);
  dbg1("starting up, rtc_compensate is %d", RTC_COMPENSATE);
#else
  dbg1("starting up");
#endif

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
  static uint32_t lastTime = RTC_TSR;
  
  if (cfg.show_time && lastTime != RTC_TSR)
  {
    Serial1.println(RTC_TSR);
    lastTime = RTC_TSR;
  }
  if (lastEnd >= nextFpsMark)
  {
    uptime++;
    if (cfg.show_fps)
    {
      Serial1.print( printbuf("uptime=%lu, fps=%d\n", uptime, fps) );
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
      if(Serial1.available())
      {
        unsigned char c = Serial1.read();
        Serial.print(c);
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

void getTmFromString(struct tm* tm, char const* buffer)
{
  tm->tm_mday = (buffer[0]  - '0') * 10 + (buffer[1]  - '0');
  tm->tm_mon  = (buffer[2]  - '0') * 10 + (buffer[3]  - '0') - 1;
  tm->tm_year = (buffer[4]  - '0') * 10 + (buffer[5]  - '0') + 100;
  tm->tm_hour = (buffer[6]  - '0') * 10 + (buffer[7]  - '0');
  tm->tm_min  = (buffer[8]  - '0') * 10 + (buffer[9]  - '0');
  tm->tm_sec  = (buffer[10] - '0') * 10 + (buffer[11] - '0');
}
  
// serialHandler
inline
void handleSerial(char const* buffer, int len)
{
  if(strncmp(buffer, "ERROR", 5) == 0)
  {
    ;//dbg1("BT: ignoring 'error' message");
  }
  else if(*buffer == 'C' && len == 1)
  {
    cfg.generator = &generator_clock;
    Serial1.println("mode set to CLOCK");
  }
  else if(*buffer == 'O' && len == 1)
  {
    cfg.generator = &generator_counter;
    Serial1.println("mode set to COUNTER");
  }
  else if (*buffer == 'B' && len == 1)
  {
    cfg.generator = &generator_birthday;
    Serial1.println("mode set to BIRTHDAY");
  }
  else if (*buffer == 'Y' && (len == 12 || len == 13))
  {
    buffer++;
    struct tm tm_target;
    if (len == 13)
    {
      //Ymmddyyhhmmss
      getTmFromString(&tm_target, buffer);
      cfg.newyear_target = mktime(&tm_target);
    }
    else
    {
      cfg.newyear_target = getTimestampFromString(buffer, 10);
      gmtime_r(&cfg.newyear_target, &tm_target);
    }
    cfg.generator = &generator_newyear;
    Serial1.print( printbuf("Clock mode set to NEWYEAR, counting down to: %lu aka %02d/%02d/%04d %02d:%02d:%02d\n",
      cfg.newyear_target, tm_target.tm_mday, tm_target.tm_mon+1, tm_target.tm_year+1900,
      tm_target.tm_hour, tm_target.tm_min, tm_target.tm_sec) );
  }
  else if (*buffer == 'R' && len == 1)
  {
    cfg.want_transition_now = 1;
    Serial1.println("Asked for a new transition... NOW!");
  }
  else if (*buffer == 'F' && len == 1)
  {
    cfg.show_fps = !cfg.show_fps;
    Serial1.print("Show FPS mode is ");
    Serial1.println(cfg.show_fps ? "ON" : "OFF");
  }
  else if (*buffer == 'M' && len == 1)
  {
    cfg.show_time = !cfg.show_time;
    Serial1.print("Show TIME mode is ");
    Serial1.println(cfg.show_time ? "ON" : "OFF");
  }
  else if(*buffer == 'T' && len == 7)
  {
    buffer++;
    uint32_t newTime = 0;
    for(int i = 0; i < 3; i++)
    {
      newTime *= 60;
      newTime += (buffer[0]-'0') * 10 + (buffer[1]-'0');
      buffer += 2;
    }
    rtc_set(newTime);
    Serial1.println("Time set");
  }
  else if (*buffer == 'D' && (len == 12 || len == 13))
  {
    buffer++;
    struct tm tm_target;
    time_t newTime;
    if (len == 13)
    {
      //Dmmddyyhhmmss
      getTmFromString(&tm_target, buffer);
      newTime = mktime(&tm_target);
    }
    else
    {
      newTime = getTimestampFromString(buffer, 10);
      gmtime_r(&newTime, &tm_target);
    }
    rtc_set(newTime);
    gmtime_r(&newTime, &tm_target);
    Serial1.print( printbuf("Time set to timestamp=%ld aka %02d/%02d/%04d %02d:%02d:%02d\n",
      newTime, tm_target.tm_mday, tm_target.tm_mon+1, tm_target.tm_year+1900,
      tm_target.tm_hour, tm_target.tm_min, tm_target.tm_sec) );
  }
  else if(*buffer == 'W' && len == 5)
  {
    buffer++;
    uint32_t countdown_seconds = (buffer[0]-'0') * 10 * 60 + (buffer[1]-'0') * 60 + (buffer[2]-'0') * 10 + (buffer[3]-'0');;
    Serial1.print( printbuf("Countdown for %d seconds\n", countdown_seconds) );
    // FIXME: millis() reset not taken into account. tocheck also : uint32 overflow
    cfg.countdown_target_millis = millis() + countdown_seconds * 1000;
    cfg.generator = &generator_countdown;
  }
  else if (*buffer == 'i' && len == 1)
  {
    Serial1.println();
    Serial1.println("NixieClock git." EXPAND2STR(GIT_REVISION) "." EXPAND2STR(GIT_DIRTY) );
    Serial1.println("Built on " EXPAND2STR(BUILD_TIME) );
    Serial1.println("Compiler version " __VERSION__ );
    Serial1.println( printbuf("Uptime is %s\n", seconds2duration(uptime)) );
    Serial1.print( printbuf("Teensy core is running at %d MHz\n", F_CPU / 1000000) );
  }
  else if (len > 0)
  {
    Serial1.println();
    Serial1.print("Unknown cmd <");
    Serial1.print(buffer);
    Serial1.println(">. Supported cmds are:");
    Serial1.println("Time setup: [T]HHMMSS or [D]<UNIXTAMP> or [D]DDMMYYHHMMSS");
    Serial1.println("Toggle options: show [F]ps, show ti[M]e");
    Serial1.println("Actions: force t[R]ansition now, show build [I]nfo");
    Serial1.println("Simple modes: [B]irthday, [C]lock, c[O]unter");
    Serial1.println("Complex modes:");
    Serial1.println("- new year: [Y]<UNIXTAMP> or [Y]DDMMYYHHMMSS");
    Serial1.println("- countdown: [W]MMSS, e.g. D9000 for 90 minutes");
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


