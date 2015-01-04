#include <time.h>

#include "globals.h"
#include "clock.h"
#include "ws2811.h"
#include "generators.h"

#include <stdint.h>

#define WANT_FADING
#define RTC_COMPENSATE 0

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
frameBuffer_t frameBuffer;
config_t cfg;

uint32_t getTimestampFromString(char const* buffer, int len)
{
  uint32_t timestamp = 0;
  for (int i = 0; i < len; i++)
  {
    uint32_t digit = buffer[i] - '0';
    for (int j = i+1; j < len; j++)
    {
      // missing check for uint overflow... but it's ok for good old unix timestamps
      digit *= 10;
    }
    timestamp += digit;
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

// Output subs, handle dots, nixie
void updateDots()
{
  digitalWriteFast(dot1Pin, frameBuffer.dots[0]);
  digitalWriteFast(dot2Pin, frameBuffer.dots[1]);
  digitalWriteFast(dot3Pin, frameBuffer.dots[2]);
  digitalWriteFast(dot4Pin, frameBuffer.dots[3]);
  digitalWriteFast(dot5Pin, frameBuffer.dots[4]);
  digitalWriteFast(dot6Pin, frameBuffer.dots[5]);
}

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

// serialHandler
void handleSerial(char const* buffer, int len)
{
  if(strncmp(buffer, "ERROR", 5) == 0)
  {
    ;//dbg1()Serial.println("FLUSHED SERIAL1");
  }
  else if(*buffer == 'C' && len == 1)
  {
    cfg.generator = &generator_clock;
    Serial1.println("Clock mode set to CLOCK");
  }
  else if(*buffer == 'c' && len == 1)
  {
    cfg.generator = &generator_counter;
    Serial1.println("Clock mode set to COUNTER");
  }
  else if (*buffer == 'A' && len == 1)
  {
    cfg.generator = &generator_birthday;
    Serial1.println("Clock mode set to BIRTHDAY");
  }
  else if (*buffer == 'Y' && len == 11)
    //Y1419891762
  {
    buffer++;
    cfg.newyear_target = getTimestampFromString(buffer, 10);
    cfg.generator = &generator_newyear;
    Serial1.println("Clock mode set to NEWYEAR, counting to:");
    struct tm *tm_target = gmtime((const long int*)&cfg.newyear_target);
    Serial1.print(tm_target->tm_mday, DEC);
    Serial1.print("/");
    Serial1.print(tm_target->tm_mon+1, DEC);
    Serial1.print("/");
    Serial1.print(tm_target->tm_year+1900, DEC);
    Serial1.print(" ");
    Serial1.print(tm_target->tm_hour, DEC);
    Serial1.print(":");
    Serial1.print(tm_target->tm_min, DEC);
    Serial1.print(":");
    Serial1.println(tm_target->tm_sec, DEC);
  }
  else if (*buffer == 't' && len == 1)
  {
    cfg.want_transition_now = 1;
    Serial1.println("Asked for a new transition... NOW!");
  }
  else if (*buffer == 'f' && len == 1)
  {
    cfg.show_fps = !cfg.show_fps;
    Serial1.print("Show FPS mode is ");
    Serial1.println(cfg.show_fps ? "ON" : "OFF");
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
  else if(*buffer == 'U' && len == 11)
  {
    // set time with an unix timestamp
    buffer++;
    uint32_t newTime = getTimestampFromString(buffer, 10);
    rtc_set(newTime);
    Serial1.print("Time set to timestamp=");
    Serial1.println(newTime);
    struct tm *tm_target = gmtime((const long int*)&newTime);
    Serial1.print(tm_target->tm_mday, DEC);
    Serial1.print("/");
    Serial1.print(tm_target->tm_mon+1, DEC);
    Serial1.print("/");
    Serial1.print(tm_target->tm_year+1900, DEC);
    Serial1.print(" ");
    Serial1.print(tm_target->tm_hour, DEC);
    Serial1.print(":");
    Serial1.print(tm_target->tm_min, DEC);
    Serial1.print(":");
    Serial1.println(tm_target->tm_sec, DEC);
  }
  else if(*buffer == 'D' && len == 5)
  {
    buffer++;
    cfg.generator = &generator_countdown;
    cfg.countdown_ms = ((buffer[0]-'0') * 10 * 60 + (buffer[1]-'0') * 60 + (buffer[2]-'0') * 10 + (buffer[3]-'0')) * 1000;
    Serial1.print("Counting down to ");
    Serial1.println(cfg.countdown_ms, DEC);
  }
  else if (len > 0)
  {
    Serial1.println("This is NixieClock Software v0.1");
    Serial1.print("Unknown command <");
    Serial1.print(*buffer);
    Serial1.println(">. Supported commands are:");
    Serial1.println("A : birthday mode");
    Serial1.println("C : clock mode");
    Serial1.println("Yxxx : new year mode, xxx is a UNIX timestamp");
    Serial1.println("c : counter mode");
    Serial1.println("t : ask for a new transition now");
    Serial1.println("f : toggle showfps");
    Serial1.println("THHMMSS : set time, e.g. T123759");
    Serial1.println("Txxx : set time, xxx is a UNIX timestamp");
    Serial1.println("DMMSS : countDown of MMSS, e.g. D9000 for 90 minutes");
  }
}


void setup() {
#ifdef RTC_COMPENSATE
  // compensate RTC
  rtc_compensate(RTC_COMPENSATE);
#endif

  // Init BT Serial
  Serial1.begin(115200, SERIAL_8N1);

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

  // Default config values
  cfg.generator = &generator_newyear;
  cfg.dot_mode = DOT_MODE_CHASE; // TODO: should be configurable via bt
  cfg.want_transition_now = 0;
  cfg.countdown_ms = 0;
  cfg.newyear_target = 0;
  cfg.show_fps = 1; // default value, can be configured via bt

  // Init wait (pb with ws2811)
  delay(1000);
  //Serial1.println("Up\n");
}

char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferLen = 0;

// Main loop
void loop()
{
  static uint32_t lastEnd = 0;
  static uint32_t nextFpsMark = 1000*1000;
  static uint16_t fps = 0;

  if (cfg.show_fps && lastEnd >= nextFpsMark)
  {
    Serial.print(millis() / 1000, DEC);
    Serial.print("s, fps=");
    Serial.println(fps, DEC);
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

void getDateStringFromTimestamp(char *buffer, unsigned int timestamp)
{
  // get year
}





