#include "trigger.h"
#include "motor.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "protocol.h"
#include "uart.h"

extern volatile uint32_t seq;

void Trigger_Init(void)
{
    // PA0 버튼 사용
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOA, &gpio);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    EXTI_InitTypeDef exti;
    exti.EXTI_Line = EXTI_Line0;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    // NVIC 설정
    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0x01;
    nvic.NVIC_IRQChannelSubPriority = 0x01;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

// 버튼 눌림 인터럽트
void EXTI0_IRQHandler(void)
{
    static uint32_t last_press = 0;
    uint32_t now = millis();

    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        // 1000ms 이내에 눌리면 무시
        if (now - last_press > 1000)
        {
            Motor_Run(2000);
            last_press = now;

            uint32_t ambient = ReadAmbient();

            char *msg = Protocol_BuildTriggerMessage(++seq, ambient);
            USART1_SendString(msg);
        }

        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
