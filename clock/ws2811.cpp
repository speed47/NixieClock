#include "ws2811.h"

#define CYCLES_800_T0H  (F_CPU / 4000000)
#define CYCLES_800_T1H  (F_CPU / 1200000)
#define CYCLES_800      (F_CPU / 800000)

unsigned int endTime = 0;

void updateLeds(unsigned int pin, uint8_t const * pixels, unsigned int nbLeds)
{
  uint8_t const *      p = pixels;
  uint8_t const *    end = p + (3*nbLeds);
  volatile uint8_t * set = portSetRegister(pin),
                   * clr = portClearRegister(pin);
  uint32_t           cyc;

  // First 50µs wait
  while((micros() - endTime) < 50);
  
  // Disable all interrupts
  __asm__ volatile("CPSID i");

  ARM_DEMCR    |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

  cyc = ARM_DWT_CYCCNT + CYCLES_800;
  while (p < end) {
    uint8_t pix = *p++;
    for (int mask = 0x80; mask; mask >>= 1)
    {
      if (pix & mask)
      {
        while (ARM_DWT_CYCCNT - cyc < CYCLES_800);
        cyc = ARM_DWT_CYCCNT;
        *set = 1;
        while (ARM_DWT_CYCCNT - cyc < CYCLES_800_T1H);
        *clr = 1;
      }
      else
      {
        while (ARM_DWT_CYCCNT - cyc < CYCLES_800);
        cyc = ARM_DWT_CYCCNT;
        *set = 1;
        while (ARM_DWT_CYCCNT - cyc < CYCLES_800_T0H);
        *clr = 1;
      }
    }
  }
  while (ARM_DWT_CYCCNT - cyc < CYCLES_800);
  endTime = micros();
  
  __asm__ volatile("CPSIE i");
}
