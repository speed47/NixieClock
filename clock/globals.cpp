#include "globals.h"
#include "printbuf.h"

static long savedDiff = -1;
static time_t lastCallRtc = 0;

unsigned long getLocalTimeT(void)
{
  time_t rtc = rtc_get();
  if (rtc != lastCallRtc)
  {
    struct tm local_tm;
    struct tm utc_tm;
    localtime_r(&rtc, &local_tm);
    gmtime_r(&rtc, &utc_tm);
    // FIXME: yday overflow (31 december vs 1 january)
    savedDiff = (
      (local_tm.tm_yday - utc_tm.tm_yday) * 86400 +
      (local_tm.tm_hour - utc_tm.tm_hour) * 3600  +
      (local_tm.tm_min  - utc_tm.tm_min ) * 60    +
      (local_tm.tm_sec  - utc_tm.tm_sec )
    );
    lastCallRtc = rtc;
  }
  return rtc + savedDiff;
}

long secondsDiffFromUTC(void)
{
  // ensure savedDiff it up to date
  (void) getLocalTimeT();
  return savedDiff;
}

