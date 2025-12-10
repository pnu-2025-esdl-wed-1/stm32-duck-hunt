#include "stm32f10x.h"
#include "time.h"
#include "motor.h"
#include <stdint.h>

volatile uint32_t _millis = 0;

uint32_t millis(void)
{
    return _millis;
}

void SysTick_Handler(void)
{
    _millis++;
    Motor_Update();
}
