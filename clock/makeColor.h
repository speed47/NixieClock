#ifndef _COLOR_H_
#define _COLOR_H_

#include "Arduino.h"

void makeColor(unsigned int hue, unsigned int saturation, unsigned int lightness, uint8_t * buffer);
unsigned int h2rgb(unsigned int v1, unsigned int v2, unsigned int hue);

#endif
