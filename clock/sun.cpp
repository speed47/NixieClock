#include <time.h>
#include <stdio.h>
#include <math.h>

#include "sun.h"
#include "globals.h"

void setSunRiseSunSet(time_t time, float lat, float lng, float offset, float *sunRiseOut, float *sunSetOut)
{
  struct tm splittedDate;
  gmtime_r(&time, &splittedDate);

  // calc the day of the year
  float N1 = floorf(275.0 * (splittedDate.tm_mon+1) /9.0);
  float N2 = floorf(( (splittedDate.tm_mon+1) +9.0)/12.0);
  float N3 = 1 + floorf(( (splittedDate.tm_year+1900) - 4 * floorf( (splittedDate.tm_year+1900) / 4) + 2) / 3);
  float N = N1 - (N2 * N3) + splittedDate.tm_mday - 30;

  // convert the longitude to hour value and calculate an approximate time

  float lngHour = lng / 15.0;

  float tRise = N + ((6 - lngHour) / 24.0);
  float tSet = N + ((18 - lngHour) / 24.0);

  // calculate the Sun's mean anomaly

  float MRise = (0.9856 * tRise) - 3.289;
  float MSet = (0.9856 * tSet) - 3.289;

  // calculate the Sun's true longitude

  (void) PI;
  float LRise = MRise + (1.916 * sinf(PI/180.0 *MRise)) + (0.020 * sinf(PI/180.0 *2 * MRise)) + 282.634;
  float LSet = MSet + (1.916 * sinf(PI/180.0 *MSet)) + (0.020 * sinf(PI/180.0 *2 * MSet)) + 282.634;

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

  float RARise = (180.0/PI) * atanf(0.91764 * tanf(PI/180.0 *LRise));
  float RASet = (180.0/PI) * atanf(0.91764 * tanf(PI/180.0 *LSet));

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

  float LquadrantRise  = (floorf( LRise/90.0)) * 90;
  float RAquadrantRise = (floorf(RARise/90.0)) * 90;
  RARise = RARise + (LquadrantRise - RAquadrantRise);
  float LquadrantSet  = (floorf( LSet/90.0)) * 90;
  float RAquadrantSet = (floorf(RASet/90.0)) * 90;
  RASet = RASet + (LquadrantSet - RAquadrantSet);

  //right ascension value needs to be converted into hours

  RARise = RARise / 15.0;
  RASet = RASet / 15.0;

  //calculate the Sun's declination

  float sinDecRise = 0.39782 * sinf(PI/180.0 *LRise);
  float cosDecRise = cosf(PI/180.0 *(180.0/PI) *asinf(sinDecRise));
  float sinDecSet = 0.39782 * sinf(PI/180.0 *LSet);
  float cosDecSet = cosf(PI/180.0 *(180.0/PI) *asinf(sinDecSet));

  //calculate the Sun's local hour angle
  float cosHRise = (cosf(PI/180.0 *90.5) - (sinDecRise * sinf(PI/180.0 *lat))) / (cosDecRise * cosf(PI/180.0 *lat));
  float cosHSet = (cosf(PI/180.0 *90.5) - (sinDecSet * sinf(PI/180.0 *lat))) / (cosDecSet * cosf(PI/180.0 *lat));

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

  float HRise =(360 - ( (180.0/PI) *acosf(cosHRise)));
  float HSet = (180.0/PI) *acosf(cosHSet);
  HRise = HRise / 15.0;
  HSet = HSet / 15.0;

  //calculate local mean time of rising/setting

  float TRise = HRise + RARise - (0.06571 * tRise) - 6.622;
  float TSet = HSet + RASet - (0.06571 * tSet) - 6.622;

  // adjust back to UTC

  float UTRise = TRise - lngHour;
  float UTSet = TSet - lngHour;
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
  float SetTime = UTSet + offset;

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




