#include "stm32f10x.h"
#include "esdl/motor.h"
#include "esdl/trigger.h"
#include "esdl/sensor.h"
#include "esdl/uart.h"
#include "esdl/time.h"
#include "esdl/game.h"     //추가
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
    NVIC_Configure();
    USART1_Init();
    USART2_init();


}

int main(void)
{
    Init();

    while (1)
    {
    Game_Loop();  //추가
    }
}
