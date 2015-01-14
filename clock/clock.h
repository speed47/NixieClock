#ifndef _NIXIE_H_
#define _NIXIE_H_

#include "Arduino.h"

#define SERIAL_BUFFER_SIZE 32

typedef union {
  uint8_t buffer[6];
  struct {
    uint8_t hours10;
    uint8_t hours1;
    uint8_t minutes10;
    uint8_t minutes1;
    uint8_t seconds10;
    uint8_t seconds1;
  } splitted;
} time_u;

void setup();
void loop();
void updateDots();
void updateNixie(unsigned int frame);
uint32_t getTimestampFromString(char const* buffer, int len);
void securityNixieDot();
void getTmFromString(struct tm* tm, char const* buffer);
int readInt(const char* buffer, int *result);
void handleSerial(char const* buffer, int len);
char *seconds2duration(uint32_t seconds);

#endif
