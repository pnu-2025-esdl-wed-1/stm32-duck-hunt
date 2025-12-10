#include "motor.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

// PA1번 모터 사용
#define MOTOR_PORT GPIOA
#define MOTOR_PIN GPIO_Pin_1

static uint32_t motor_end_time = 0;

extern uint32_t millis(void);

void Motor_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = MOTOR_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_PORT, &gpio);

    Motor_Off();
}

void Motor_On(void)
{
    GPIO_SetBits(MOTOR_PORT, MOTOR_PIN);
}

void Motor_Off(void)
{
    GPIO_ResetBits(MOTOR_PORT, MOTOR_PIN);
}

void Motor_Run(uint32_t ms)
{
    Motor_On();
    motor_end_time = millis() + ms;
}

void Motor_Update(void)
{
    if (motor_end_time != 0 && millis() >= motor_end_time)
    {
        Motor_Off();
        motor_end_time = 0;
    }
}
