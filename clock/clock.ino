#include "clock.h"
#include "ws2811.h"
#include "makeColor.h"

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
clockMode mode = CLOCK;

unsigned int getTime()
{
  return RTC_TSR;
}

// Main loop
void loop()
{
  static unsigned int lastEnd = 0;
  
  // Generate what to display
  switch(mode)
  {
    case CLOCK:
      clock();
      break;
      
    case COUNTER:
      counter();
      break;
      
    case BIRTHDAY:
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
     mode = CLOCK;
   }
   else if(*buffer == 'c' && len == 1)
   {
     mode = COUNTER;
   }
   else if (*buffer == 'A' && len == 1)
   {
     mode = BIRTHDAY;
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
  unsigned int currentTime = getTime();
  
  /* During 250ms display progressive change between numbers */
  /* TODO: need more testing */
  // 125ms after change
  /*if(RTC_TPR < 4096)
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
  }*/
  currentTime = getTime();

  /* UPDATE DOTS */
  frameBuffer.dots[0] = ((currentTime % 6)==0);
  frameBuffer.dots[1] = ((currentTime % 6)==1);
  frameBuffer.dots[2] = ((currentTime % 6)==2);
  frameBuffer.dots[3] = ((currentTime % 6)==3);
  frameBuffer.dots[4] = ((currentTime % 6)==4);
  frameBuffer.dots[5] = ((currentTime % 6)==5);
  
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
  frameBuffer.digits[4] = currentTime%6;
  currentTime /= 6;
  // Minutes
  frameBuffer.digits[3] = currentTime % 10;
  currentTime /= 10;
  frameBuffer.digits[2] = currentTime%6;
  currentTime /= 6;
  // Hours
  unsigned int hours = currentTime%24;
  frameBuffer.digits[1] = hours % 10;
  frameBuffer.digits[0] = hours / 10;
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
