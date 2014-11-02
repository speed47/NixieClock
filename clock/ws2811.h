#ifndef _WS2811_H_
#define _WS2811_H_

#include "Arduino.h"

void updateLeds(unsigned int pin, uint8_t const * pixels, unsigned int nbLeds);

#endif
