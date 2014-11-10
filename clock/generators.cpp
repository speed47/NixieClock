#include "generators.h"
#include "makeColor.h"
#include "globals.h"

// helper functions
void splitTimeToFramebuffer(unsigned long time, splitMode split_mode)
{
  if (split_mode == SPLIT_HMS)
  {
    // here, input is seconds
    // Seconds
    frameBuffer.digits[5] = time % 10;
    time /= 10;
    frameBuffer.digits[4] = time % 6;
    time /= 6;
    // Minutes
    frameBuffer.digits[3] = time % 10;
    time /= 10;
    frameBuffer.digits[2] = time % 6;
    time /= 6;
    // Hours
    unsigned int hours = time % 24;
    frameBuffer.digits[1] = hours % 10;
    frameBuffer.digits[0] = hours / 10;
  }
  else if (split_mode == SPLIT_MSC)
  {
    // here, time is milliseconds
    frameBuffer.digits[5] = time % 100 / 10;
    time /= 100; // now deciseconds
    frameBuffer.digits[4] = time % 10;
    time /= 10; // now seconds
    frameBuffer.digits[3] = time % 10;
    time /= 10; // now 10x seconds
    frameBuffer.digits[2] = time % 6;
    time /= 6; // now minutes
    frameBuffer.digits[1] = time % 10;
    time /= 10; // now 10x minutes
    frameBuffer.digits[0] = time % 6;
  }
}

unsigned int getTime()
{
  return RTC_TSR;
}

// Generator functions
void generator_clock()
{
  static unsigned int frame = 0;
  frame++;
  
  // Get Current Time
  unsigned int currentTime = getTime(); // might be modified +/- 1 by fading algo
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
  splitTimeToFramebuffer(currentTime, SPLIT_HMS);
  
  // transition test
  static int transition_step = 0;

  if (config_want_transition_now > 0)
  {
    config_want_transition_now = 0;
    transition_step = 1;
  }

  if (frameBuffer.digits[5] == 3 and frameBuffer.digits[4] == 5 and frameBuffer.digits[3] % 5 == 4 and transition_step == 0)
  {
    transition_step = 1;
  }

  if (transition_step > 0)
  {
    static unsigned int lastTime = getTime();
    currentTime = getTime();
    /* steps :
    1 => set vars to their init value then set step=2
    2, 3 => casinoize all nixies (2 seconds)
    4 5 6 7 8 => casinoize respectively 5, 4, 3, 2, then 1 nixie (1 second each)
    9 => transition done, set step back to 0 (which means: no transition)
    */
    if (transition_step == 1) // first loop, init stuff
    {
      dbg("step 1 => 2");
      transition_step = 2;
    }
    else if (transition_step >= 2 and transition_step <= 9)
    {
      if (currentTime != lastTime)
      {
        dbg("step++");
        transition_step++;
      }
    }
    else
    {
      dbg("transition done!");
      transition_step = 0; // done!
    }

    // random color on the leds, one different each 16 frames
    static int hue = rand() % 360;
    if (frame % 16 == 0)
    {
      hue = rand() % 360;
    }

    // casino-ize proper nixies
    for (int i = max(transition_step - 4, 0); i < 6; i++)
    {
       frameBuffer.digits[i] = rand() % 10;
       makeColor(hue, 100, 50, &frameBuffer.leds[3*(5-i)]); // led order is reversed on my board :)
    }

    lastTime = currentTime;
  }
}

void generator_counter()
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

void generator_birthday()
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

void generator_countdown()
{
  // TODO do something with the leds
  // TODO do something with the nixie dots (allow classic use of DOT_MODE_* ?)
  static unsigned long target_millis = 0; // FIXME: millis() counter reset is not taken into account
  if (config_countdown_ms > 0)
  {
    target_millis = millis() + config_countdown_ms;
    config_countdown_ms = 0;
  }
  signed long remaining_millis = target_millis - millis();
  if (remaining_millis >= 0)
  {
    splitTimeToFramebuffer(remaining_millis, SPLIT_MSC);
  }
  else
  {
    // TODO do something fancy when the time is up, before restoring the clock
    generator = &generator_clock;
  }
}


