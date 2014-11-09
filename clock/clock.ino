#include "clock.h"
#include "ws2811.h"
#include "makeColor.h"

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

void setup() {
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
  
  // Init wait (pb with ws2811)
  delay(1000);
  Serial1.println("Up\n");
}

frameBuffer_t frameBuffer;
char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferLen = 0;
clockMode config_clock_mode = CLOCK_MODE_CLOCK;
dotMode config_dot_mode = DOT_MODE_CHASE; // TODO: should be configurable via bt
int config_showfps = 1; // default value, can be configured via bt
int config_want_transition_now = 0;

unsigned int getTime()
{
  return RTC_TSR;
}

// Main loop
void loop()
{
  static uint32_t lastEnd = 0;
  static uint32_t nextFpsMark = 1000*1000;
  static uint16_t fps = 0;
  
  if (config_showfps && lastEnd >= nextFpsMark)
  {
    Serial1.print(millis() / 1000, DEC);
    Serial1.print("s, fps=");
    Serial1.println(fps, DEC);
    nextFpsMark = lastEnd + 1000 * 1000;
    fps = 0;
  }
  fps++;
  
  // Generate what to display
  switch(config_clock_mode)
  {
    case CLOCK_MODE_CLOCK:
      clock();
      break;
      
    case CLOCK_MODE_COUNTER:
      counter();
      break;
      
    case CLOCK_MODE_BIRTHDAY:
      birthday();
      break;
  }
  
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

// serialHandler
void handleSerial(char const* buffer, int len)
{
   if(*buffer == 'C' && len == 1)
   {
     config_clock_mode = CLOCK_MODE_CLOCK;
     Serial1.println("Clock mode set to CLOCK");
   }
   else if(*buffer == 'c' && len == 1)
   {
     config_clock_mode = CLOCK_MODE_COUNTER;
     Serial1.println("Clock mode set to COUNTER");
   }
   else if (*buffer == 'A' && len == 1)
   {
     config_clock_mode = CLOCK_MODE_BIRTHDAY;
     Serial1.println("Clock mode set to BIRTHDAY");
   }
   else if (*buffer == 't' && len == 1)
   {
     config_want_transition_now = 1;
     Serial1.println("Asked for a new transition... NOW!");
   }
   else if (*buffer == 'f' && len == 1)
   {
     if (config_showfps)
     {
       config_showfps = 0;
       Serial1.println("Show FPS mode is OFF");
     }
     else
     {
       config_showfps = 1;
       Serial1.println("Show FPS mode is ON");
     }
   }
   else if(*buffer == 'T' && len == 7)
   {
     // Simple version which handle only hours-minutes-seconds
     // TODO: Handle timestamps
     buffer++;
     unsigned int newTime = 0;
     for(int i = 0; i < 3; i++)
     {
       newTime *= 60;
       newTime += (buffer[0]-'0') * 10 + (buffer[1]-'0');
       buffer += 2;
     }
     rtc_set(newTime);
     Serial1.println("Time set");
   }
   else if (len > 0)
   {
     Serial1.println("This is NixieClock Software v0.1");
     Serial1.print("Unknown command <");
     Serial1.print(*buffer);
     Serial1.println(">. Supported commands are:");
     Serial1.println("A : birthday mode");
     Serial1.println("C : clock mode");
     Serial1.println("c : counter mode");
     Serial1.println("t : ask for a new transition now");
     Serial1.println("f : toggle showfps");
     Serial1.println("THHMMSS : set time, e.g. T123759\n");
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

// Generator functions
void clock()
{
  static unsigned int frame = 0;
  frame++;
  
  // Get Current Time
  unsigned int currentTime = getTime(); // might be modified by fading
  unsigned int currentTimeReal = currentTime; // we need the real unmodified currentTime later (at least for some dot modes)
  
  #ifdef WANT_FADING
  /* During 250ms display progressive change between numbers */
  /* TODO: need more testing */
  // 125ms after change
  if(RTC_TPR < 4096)
  {
    // 50 => 100%
    uint8_t percent = 4 + ((4 * RTC_TPR) / 4096);
    if((frame%8) < percent)
    {
      currentTime = getTime();
    }
    else
    {
      currentTime = getTime() - 1;
    }
  }
  // 250ms before change
  else if(RTC_TPR > 28672)
  {
    // 0 => 50%
    uint8_t percent = (4 * (RTC_TPR-28672)) / 4096;
    if((frame%8) < percent)
    {
      currentTime = getTime()+1;
    }
    else
    {
      currentTime = getTime();
    }
  }
  else
  {
    currentTime = getTime();
  }
  #else
  currentTime = getTime();
  #endif

  /* UPDATE DOTS */
  if (config_dot_mode == DOT_MODE_CHASE)
  {
    // yes, we could use loops below, but this way right values are computed at compile time and we're freaking fast
    if (currentTimeReal % 4 == 0)
    {
      frameBuffer.dots[5] = (RTC_TPR > 32768/7*1);
      frameBuffer.dots[4] = (RTC_TPR > 32768/7*2);
      frameBuffer.dots[3] = (RTC_TPR > 32768/7*3);
      frameBuffer.dots[2] = (RTC_TPR > 32768/7*4);
      frameBuffer.dots[1] = (RTC_TPR > 32768/7*5);
      frameBuffer.dots[0] = (RTC_TPR > 32768/7*6);
    }
    else if (currentTimeReal % 4 == 1)
    {
      frameBuffer.dots[5] = (RTC_TPR < 32768/7*1);
      frameBuffer.dots[4] = (RTC_TPR < 32768/7*2);
      frameBuffer.dots[3] = (RTC_TPR < 32768/7*3);
      frameBuffer.dots[2] = (RTC_TPR < 32768/7*4);
      frameBuffer.dots[1] = (RTC_TPR < 32768/7*5);
      frameBuffer.dots[0] = (RTC_TPR < 32768/7*6);
    }
    else if (currentTimeReal % 4 == 2)
    {
      frameBuffer.dots[5] = (RTC_TPR > 32768/7*6);
      frameBuffer.dots[4] = (RTC_TPR > 32768/7*5);
      frameBuffer.dots[3] = (RTC_TPR > 32768/7*4);
      frameBuffer.dots[2] = (RTC_TPR > 32768/7*3);
      frameBuffer.dots[1] = (RTC_TPR > 32768/7*2);
      frameBuffer.dots[0] = (RTC_TPR > 32768/7*1);
    }
    else // aka (currentTimeReal % 4 == 3)
    {
      frameBuffer.dots[5] = (RTC_TPR < 32768/7*6);
      frameBuffer.dots[4] = (RTC_TPR < 32768/7*5);
      frameBuffer.dots[3] = (RTC_TPR < 32768/7*4);
      frameBuffer.dots[2] = (RTC_TPR < 32768/7*3);
      frameBuffer.dots[1] = (RTC_TPR < 32768/7*2);
      frameBuffer.dots[0] = (RTC_TPR < 32768/7*1);
    }
  }
  else if (config_dot_mode == DOT_MODE_CLASSIC)
  {
    frameBuffer.dots[0] = ((currentTime % 6)==0);
    frameBuffer.dots[1] = ((currentTime % 6)==1);
    frameBuffer.dots[2] = ((currentTime % 6)==2);
    frameBuffer.dots[3] = ((currentTime % 6)==3);
    frameBuffer.dots[4] = ((currentTime % 6)==4);
    frameBuffer.dots[5] = ((currentTime % 6)==5);
  }
  else if (config_dot_mode == DOT_MODE_PROGRESSIVE)
  {
    frameBuffer.dots[5] = (RTC_TPR >= 32768/7*1);
    frameBuffer.dots[4] = (RTC_TPR >= 32768/7*2);
    frameBuffer.dots[3] = (RTC_TPR >= 32768/7*3);
    frameBuffer.dots[2] = (RTC_TPR >= 32768/7*4);
    frameBuffer.dots[1] = (RTC_TPR >= 32768/7*5);
    frameBuffer.dots[0] = (RTC_TPR >= 32768/7*6);
  }
  
  /* UPDATE LEDS */
  // Slow rainbow, same color on each led
  for(int i = 0; i < 6; i++)
  {
    makeColor(frame/16, 100, 50, &frameBuffer.leds[3*i]);
  }

  /* SPLIT TIME in the 6 digits */  
  // Seconds
  frameBuffer.digits[5] = currentTime % 10;
  currentTime /= 10;
  frameBuffer.digits[4] = currentTime % 6;
  currentTime /= 6;
  // Minutes
  frameBuffer.digits[3] = currentTime % 10;
  currentTime /= 10;
  frameBuffer.digits[2] = currentTime % 6;
  currentTime /= 6;
  // Hours
  unsigned int hours = currentTime % 24;
  frameBuffer.digits[1] = hours % 10;
  frameBuffer.digits[0] = hours / 10;
  
  // transition test
  static int transitioning = 0;
  static unsigned int transition_started_at = 0;

  if (config_want_transition_now > 0)
  {
    config_want_transition_now = 0;
    transitioning = 1;
    transition_started_at = 0;
  }

  if (frameBuffer.digits[5] == 4 and frameBuffer.digits[4] == 5 and frameBuffer.digits[3] % 5 == 4 and transitioning == 0)
  {
    transitioning = 1;
    transition_started_at = 0;
  }

  if (transitioning == 1)
  {
    static unsigned int lastTime = getTime();
    static int frozenNixies[6];
    currentTime = getTime();
    if (transition_started_at == 0)
    {
      // first loop, init stuff
      transition_started_at = currentTime;
      for (int i = 0; i < 6; i++)
      {
        frozenNixies[i] = 0;
      }
    }
    // transition lasts 2 + 6 seconds
    if (transition_started_at + 2 + 5 >= currentTime)
    {
      if (transition_started_at + 2 <= currentTime && lastTime != currentTime)
      {
        // another elapsed second, freezing another nixie
        // FIXME ugly, theoretically possible infinite loop
        for (int antiloop = 0; antiloop < 1000; antiloop++)
        {
          int index = rand() % 6;
          if (frozenNixies[index] == 0)
          {
            frozenNixies[index] = 1;
            break;
          }
        }
      }
      // randomize digits on the non-frozen nixies
      for (int i = 0; i < 6; i++)
      {
        if (frozenNixies[i] == 0)
        {
          frameBuffer.digits[i] = rand() % 10;
        }
      }
      // random color on the leds, one different each 16 frames
      static int hue = rand() % 360;
      if (frame % 16 == 0)
      {
        hue = rand() % 360;
      }
      for(int i = 0; i < 6; i++)
      {
        if (frozenNixies[i] == 0)
        {
          makeColor(hue, 100, 50, &frameBuffer.leds[3*i]);
        }
      }
      // between seconds 3 and 3+nb(nixies), freeze a nixie each second
    }
    else
    {
      // transition is done
      transition_started_at = 0;
      transitioning = 0;
    }
    lastTime = currentTime;
  }
}

void counter()
{
  static unsigned int frame = 0;
  frame++;
  
  // Debug mode, to test each digit/dot on each tube
  frameBuffer.digits[5] = (frame/100) % 10;
  frameBuffer.digits[4] = (frame/100) % 10;
  frameBuffer.digits[3] = (frame/100) % 10;
  frameBuffer.digits[2] = (frame/100) % 10;
  frameBuffer.digits[1] = (frame/100) % 10;
  frameBuffer.digits[0] = (frame/100) % 10;
  
  frameBuffer.dots[0] = ((frame/100 % 6)==0);
  frameBuffer.dots[1] = ((frame/100 % 6)==1);
  frameBuffer.dots[2] = ((frame/100 % 6)==2);
  frameBuffer.dots[3] = ((frame/100 % 6)==3);
  frameBuffer.dots[4] = ((frame/100 % 6)==4);
  frameBuffer.dots[5] = ((frame/100 % 6)==5);
}

void birthday()
{
  static unsigned int frame = 0;
  frame++;

  // Set 30 as binary on tubes
  frameBuffer.digits[0] = 0;
  frameBuffer.digits[1] = 1;
  frameBuffer.digits[2] = 1;
  frameBuffer.digits[3] = 1;
  frameBuffer.digits[4] = 1;
  frameBuffer.digits[5] = 0;
  
  /* UPDATE LEDS */
  // Rainbow over all leds
  for(int i = 0; i < 6; i++)
  {
    makeColor(frame+32*i, 100, 50, &frameBuffer.leds[3*i]);
  }

  /* Turn dots OFF */
  frameBuffer.dots[0] = 0;
  frameBuffer.dots[1] = 0;
  frameBuffer.dots[2] = 0;
  frameBuffer.dots[3] = 0;
  frameBuffer.dots[4] = 0;
  frameBuffer.dots[5] = 0;
}
