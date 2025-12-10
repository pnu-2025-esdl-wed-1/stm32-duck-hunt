#ifndef ESDL_TIME_H
#define ESDL_TIME_H

#include <stdint.h>

extern volatile uint32_t _millis;

uint32_t millis(void);
void SysTick_Handler(void);

#endif
