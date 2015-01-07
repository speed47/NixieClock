#include "generators.h"
#include "makeColor.h"
#include "globals.h"
#include "btprint.h"

// helper functions
void splitTimeToFramebuffer(unsigned long time, splitMode split_mode)
{
  if (split_mode == SPLIT_SEC_TO_HOUR_MIN_SEC)
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
  else if (split_mode == SPLIT_MS_TO_MIN_SEC_CENTISEC)
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
  if (cfg.dot_mode == DOT_MODE_CHASE)
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
  else if (cfg.dot_mode == DOT_MODE_CLASSIC)
  {
    frameBuffer.dots[0] = ((currentTime % 6)==0);
    frameBuffer.dots[1] = ((currentTime % 6)==1);
    frameBuffer.dots[2] = ((currentTime % 6)==2);
    frameBuffer.dots[3] = ((currentTime % 6)==3);
    frameBuffer.dots[4] = ((currentTime % 6)==4);
    frameBuffer.dots[5] = ((currentTime % 6)==5);
  }
  else if (cfg.dot_mode == DOT_MODE_PROGRESSIVE)
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
  splitTimeToFramebuffer(currentTime, SPLIT_SEC_TO_HOUR_MIN_SEC);
  
  // transition test
  static int transition_step = 0;

  if (cfg.want_transition_now > 0)
  {
    cfg.want_transition_now = 0;
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
      dbg2("transition step 1 => 2");
      transition_step = 2;
    }
    else if (transition_step >= 2 and transition_step <= 9)
    {
      if (currentTime != lastTime)
      {
        dbg2("transition step++");
        transition_step++;
      }
    }
    else
    {
      dbg2("transition done!");
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
  // FIXME: millis() counter reset is not taken into account
  int32_t remaining_millis = cfg.countdown_target_millis - millis();
  if (remaining_millis >= 0)
  {
    splitTimeToFramebuffer(remaining_millis, SPLIT_MS_TO_MIN_SEC_CENTISEC);
  }
  else
  {
    // TODO do something fancy when the time is up, before restoring the clock
    cfg.countdown_target_millis = 0;
    cfg.generator = &generator_clock;
  }
}

void generator_newyear()
{
  static unsigned int frame = 0;
  frame++;

  // config_newyear_target is target timestamp
  // getTime() is current timestamp
  int32_t togo_sec = cfg.newyear_target - getTime();
  unsigned int togo_ms = (32768 - RTC_TPR) / (32768 / 1000);

  /* Turn dots OFF */
  frameBuffer.dots[0] = 0;
  frameBuffer.dots[1] = 0;
  frameBuffer.dots[2] = 0;
  frameBuffer.dots[3] = 0;
  frameBuffer.dots[4] = 0;
  frameBuffer.dots[5] = 0;

  // Fast rainbow, same color on each led
  for(int i = 0; i < 6; i++)
  {
    makeColor(frame+32*i, 100, 50, &frameBuffer.leds[3*i]);
  }

  // we display different things depending on the number of
  // significative digits we have in togo_sec

  // A) togo_sec >= 1000000 (1.2 days to infinity) => just display seconds on all the nixies
  // special sub-case if togo_sec > 999999 (11.5 days), we're stuck on 999999
//dbg(togo_sec);
//dbg(togo_ms);
  if (togo_sec >= 100000)
  {
    // FIXME later: implement "show minutes" for >999999 instead of being stuck at 999999
    if (togo_sec > 999999) { togo_sec = 999999; }
    // split each digit to where it belongs
    frameBuffer.digits[0] =  togo_sec/100000;
    frameBuffer.digits[1] = (togo_sec%100000)/10000;
    frameBuffer.digits[2] = (togo_sec%10000)/1000;
    frameBuffer.digits[3] = (togo_sec%1000)/100;
    frameBuffer.digits[4] = (togo_sec%100)/10;
    frameBuffer.digits[5] =  togo_sec%10;
  }       
  // B) togo_sec >= 10000 (2.7 hours to 1.2 days) => display decisecs on last nixie aka 12345.6
  else if (togo_sec >= 10000)
  {
    // split each seconds digit to where it belongs
    frameBuffer.digits[0] =  togo_sec/10000;
    frameBuffer.digits[1] = (togo_sec%10000)/1000;
    frameBuffer.digits[2] = (togo_sec%1000)/100;
    frameBuffer.digits[3] = (togo_sec%100)/10;
    frameBuffer.digits[4] =  togo_sec%10;
    // and decisecs
    frameBuffer.dots[1] = 1;
    // FIXME i might have a bug here, seems that the last nixie turns off briefly between .9 and .0
    // probably out of bounds at some point (<0 or >9)
    frameBuffer.digits[5] = togo_ms/100;
  }       
  // C) togo_sec >= 1000 (15 min to 2.7 hours) => display centisecs too aka 1234.56
  else if (togo_sec >= 1000)
  {
    // split each seconds digit to where it belongs
    frameBuffer.digits[0] =  togo_sec/1000;
    frameBuffer.digits[1] = (togo_sec%1000)/100;
    frameBuffer.digits[2] = (togo_sec%100)/10 ;
    frameBuffer.digits[3] =  togo_sec%10;
    // and decisecs
    frameBuffer.dots[2] = 1;
    frameBuffer.digits[4] =  togo_ms/100;
    frameBuffer.digits[5] = (togo_ms%100)/10;
  }
  // D) togo_sec >= 100 (1.5 min to 15 min) => K2000 of 3 digits one way per second, such as:
  /*
          999...
          .999..
          ..999.
          ...998
          ..998.
          .998..
          997...
          .997..
          ..997.
  */
  else if (togo_sec >= 100)
  {
    int nixieOn;
    for (int i = 0; i < 6; i++)
    {
      frameBuffer.digits[i] = 0xF; // nixie off
    }
    // if togo_sec is odd, left to right
    if (togo_sec % 2 == 0)
    {
      if      (togo_ms <= 333) { nixieOn = 1; }
      else if (togo_ms <= 666) { nixieOn = 2; }
      else                     { nixieOn = 3; }
    }
    // else if even, right to left
    else
    {
      if      (togo_ms <= 333) { nixieOn = 2; }
      else if (togo_ms <= 666) { nixieOn = 1; }
      else                     { nixieOn = 0; }
    }
    frameBuffer.digits[nixieOn+0] =  togo_sec/100;
    frameBuffer.digits[nixieOn+1] = (togo_sec%100)/10;
    frameBuffer.digits[nixieOn+2] = (togo_sec%10);
    for (int i = 0; i < 6; i++)
    {
      if (i != nixieOn && i != nixieOn+1 && i != nixieOn+2)
      {
        makeColor(0, 0, 50, &frameBuffer.leds[3*5-3*i]);
      }
    }
  }
  // E) togo_sec >= 10 => 2+2+2 couples
  /*
          999999
          989898
          979797
  */
  else if (togo_sec >= 10)
  {
    frameBuffer.digits[0] = togo_sec/10;
    frameBuffer.digits[1] = togo_sec%10;
    frameBuffer.digits[2] = togo_sec/10;
    frameBuffer.digits[3] = togo_sec%10;
    frameBuffer.digits[4] = togo_sec/10;
    frameBuffer.digits[5] = togo_sec%10;

    makeColor((frame+240)*2, 100, 50, &frameBuffer.leds[3*0]);
    makeColor((frame+240)*2, 100, 50, &frameBuffer.leds[3*1]);

    makeColor((frame+120)*2, 100, 50, &frameBuffer.leds[3*2]);
    makeColor((frame+120)*2, 100, 50, &frameBuffer.leds[3*3]);

    makeColor((frame+0)*2, 100, 50, &frameBuffer.leds[3*4]);
    makeColor((frame+0)*2, 100, 50, &frameBuffer.leds[3*5]);
  }
  // F) last seconds !
  else if (togo_sec > 0)
  {
    for (int i = 0; i < 6; i++)
    {
      frameBuffer.digits[i] = 0xF; // nixie off
    }
    int nixieLit = 6 - RTC_TPR*7/32768;
    frameBuffer.digits[nixieLit] = togo_sec;
    
    // stroboscopic leds
    for(int i = 0; i < 6; i++)
    {
      int col = 0;
      if (RTC_TPR/512 % 2 == 0)
      {
        col = 0xFF;
      }
      makeColor(0, 100, col, &frameBuffer.leds[3*i]);
    }
  }
  // G) here we are
  else if (togo_sec >= -30)
  {
    for (int i = 0; i < 6; i++)
    {
      frameBuffer.digits[i] = 0xF;
    }
    if (RTC_TPR/2048 % 2 == 1)
    {
      frameBuffer.digits[1] = 2;
      frameBuffer.digits[2] = 0;
      frameBuffer.digits[3] = 1;
      frameBuffer.digits[4] = 5;
    }

    // stroboscopic leds
    for(int i = 0; i < 6; i++)
    {
      int col = 0;
      if (RTC_TPR/1024 % 2 == 0)
      {
        col = 0xFF;
      }
      makeColor(0, 100, col, &frameBuffer.leds[3*i]);
    }
  }
  // H) done.
  else
  {
    cfg.generator = &generator_clock;
  }
}


  


