#ifndef _SUN_H
#define _SUN_H

#define SUNTIME_INVALID    (-1.f)
#define SUNTIME_NEVER_RISE (-2.f)
#define SUNTIME_NEVER_SET  (-3.f)
#ifndef PI
// :D
# define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679f
#endif

void getSunInfo(const float** sunRiseOut, const float** sunSetOut);

#endif
