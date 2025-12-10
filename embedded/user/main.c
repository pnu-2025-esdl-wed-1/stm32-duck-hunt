#include "stm32f10x.h"
#include "esdl/motor.h"
#include "esdl/trigger.h"
#include "esdl/sensor.h"
#include "esdl/uart.h"
#include "esdl/time.h"
#include "string.h"
#include "stdio.h"

void Init(void);

void Init(void)
{
    SystemInit();

    // SysTick 1ms 설정 → millis() 사용 가능
    SysTick_Config(SystemCoreClock / 1000);
    Sensor_ADC_Init();
    Motor_Init();
    Trigger_Init();
    Trigger_Init();
}

int main(void)
{
    Init();

    while (1)
    {
        char chr[1024];
        sprintf(chr,"%d", _millis);
        USART1_SendString(chr);
    }
}
