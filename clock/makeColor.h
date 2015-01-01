#ifndef _COLOR_H_
#define _COLOR_H_

#define LED_LIGHTNESS_DIVIDER 1 /* useful to code at night and/or on battery :) TODO: should be configurable via bt */

#include "Arduino.h"

void makeColor(unsigned int hue, unsigned int saturation, unsigned int lightness, uint8_t * buffer);
unsigned int h2rgb(unsigned int v1, unsigned int v2, unsigned int hue);

#endif
