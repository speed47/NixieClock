#include <time.h>
#include <stdio.h>
#include <math.h>

#include "sun.h"
#include "globals.h"

void setSunRiseSunSet(time_t time, float lat, float lng, float offset, float *sunRiseOut, float *sunSetOut)
{
  struct tm splittedDate;
  gmtime_r(&time, &splittedDate);

  // convert the longitude to hour value and calculate an approximate time

  float lngHour = lng / 15.0f;

  float tRise = (splittedDate.tm_yday + 1) + (( 6 - lngHour) / 24.0f);
  float tSet  = (splittedDate.tm_yday + 1) + ((18 - lngHour) / 24.0f);

  // calculate the Sun's mean anomaly

  float MRise = (0.9856f * tRise) - 3.289f;
  float MSet  = (0.9856f * tSet)  - 3.289f;

  // calculate the Sun's true longitude

  float LRise = MRise + (1.916f * sinf(PI/180.0f *MRise)) + (0.020f * sinf(PI/180.0f * 2 * MRise)) + 282.634f;
  float LSet  = MSet  + (1.916f * sinf(PI/180.0f *MSet )) + (0.020f * sinf(PI/180.0f * 2 * MSet )) + 282.634f;

  if(LRise < 0)
  {
    LRise += 360;
  }
  if(LSet < 0)
  {
    LSet += 360;
  }
  if(LRise >= 360)
  {
    LRise -= 360;
  }
  if(LSet >= 360)
  {
    LSet -= 360;
  }
  // calculate the Sun's right ascension

  float RARise = (180.0f/PI) * atanf(0.91764f * tanf(PI/180.0f * LRise));
  float RASet  = (180.0f/PI) * atanf(0.91764f * tanf(PI/180.0f * LSet));

  if(RARise < 0)
  {
    RARise += 360;
  }
  if(RASet < 0)
  {
    RASet += 360;
  }
  if(RARise >= 360)
  {
    RARise -= 360;
  }
  if(RASet >= 360)
  {
    RASet -= 360;
  }

  // right ascension value needs to be in the same quadrant as L

  float LquadrantRise  = (floorf( LRise/90.0f)) * 90;
  float RAquadrantRise = (floorf(RARise/90.0f)) * 90;
  RARise = RARise + (LquadrantRise - RAquadrantRise);
  float LquadrantSet  = (floorf( LSet/90.0f)) * 90;
  float RAquadrantSet = (floorf(RASet/90.0f)) * 90;
  RASet = RASet + (LquadrantSet - RAquadrantSet);

  //right ascension value needs to be converted into hours

  RARise = RARise / 15.0f;
  RASet  = RASet  / 15.0f;

  //calculate the Sun's declination

  float sinDecRise = 0.39782f * sinf(PI/180.0f * LRise);
  float sinDecSet  = 0.39782f * sinf(PI/180.0f * LSet);
  float cosDecRise = cosf(PI/180.0f * (180.0f/PI) * asinf(sinDecRise));
  float cosDecSet  = cosf(PI/180.0f * (180.0f/PI) * asinf(sinDecSet));

  //calculate the Sun's local hour angle
  float cosHRise = (cosf(PI/180.0f * 90.5f) - (sinDecRise * sinf(PI/180.0f * lat))) / (cosDecRise * cosf(PI/180.0f * lat));
  float cosHSet =  (cosf(PI/180.0f * 90.5f) - (sinDecSet  * sinf(PI/180.0f * lat))) / (cosDecSet  * cosf(PI/180.0f * lat));

  //the sun never sets, or never rises
  if (cosHRise >  1)
  {
    *sunRiseOut = SUNTIME_NEVER_RISE;
    *sunSetOut  = SUNTIME_NEVER_RISE;
    return;
  }
  if (cosHSet < -1)
  {
    *sunRiseOut = SUNTIME_NEVER_SET;
    *sunSetOut  = SUNTIME_NEVER_SET;
    return;
  }

  // finish calculating H and convert into hours

  float HRise =(360 - ( (180.0f/PI) *acosf(cosHRise)));
  float HSet = (180.0f/PI) *acosf(cosHSet);
  HRise = HRise / 15.0f;
  HSet  = HSet  / 15.0f;

  //calculate local mean time of rising/setting

  float TRise = HRise + RARise - (0.06571f * tRise) - 6.622f;
  float TSet  = HSet  + RASet  - (0.06571f * tSet)  - 6.622f;

  // adjust back to UTC

  float UTRise = TRise - lngHour;
  float UTSet  = TSet  - lngHour;
  if(UTRise < 0)
  {
    UTRise += 24;
  }
  else if(UTRise >= 24)
  {
    UTRise -= 24;
  }
  if(UTSet < 0)
  {
    UTSet += 24;
  }
  else if(UTSet >= 24)
  {
    UTSet -= 24;
  }

  // convert UT value to local time zone of latitude/longitude

  float RiseTime = UTRise + offset;
  float SetTime  = UTSet  + offset;

  if(RiseTime < 0)
  {
    RiseTime += 24;
  }
  else if(RiseTime >= 24)
  {
    RiseTime -= 24;
  }

  if(SetTime < 0)
  {
    SetTime += 24;
  }
  else if(SetTime >= 24)
  {
    SetTime -= 24;
  }

  // now you can use the RiseTime & SetTime as you wish
  *sunRiseOut = RiseTime;
  *sunSetOut  = SetTime;
}

int lastComputedDay = -1;
float latitude  = 50.7217; /* tourcoing by default */ //FIXME
float longitude =  3.1592; /* represent ! */ //FIXME

void setLocation(float lat, float lng)
{
  latitude  = lng;
  longitude = lat;
  lastComputedDay = -1; // force recompute on next call
}

void getSunInfo(const float **sunRiseOut, const float **sunSetOut)
{
  static float sunRise = SUNTIME_INVALID;
  static float sunSet  = SUNTIME_INVALID;
  time_t now = rtc_get();
  struct tm splittedDate;
  gmtime_r(&now, &splittedDate);

  // if either one of the above variables has not yet been set (-1), or if
  // the day since the last computation has changed, recompute the values
  if (sunRise == SUNTIME_INVALID || sunSet == SUNTIME_INVALID || splittedDate.tm_yday != lastComputedDay)
  {
    float offset = secondsDiffFromUTC() / 3600.f;
    setSunRiseSunSet(now, latitude, longitude, offset, &sunRise, &sunSet);
    lastComputedDay = splittedDate.tm_yday;
  }
  *sunRiseOut = &sunRise;
  *sunSetOut  = &sunSet;
}




