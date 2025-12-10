#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "sensor.h"

volatile uint16_t adc_buf[ADC_BUF_LEN];

/* 내부 함수 선언 */
void Sensor_TIM2_Init(void);

/* ============================
     ADC(PC0) + DMA + TIM2 Init
   ============================ */
void Sensor_ADC_Init(void)
{
    // 1. 클럭 활성화 
    // DMA1, ADC1, GPIOC(조도센서), TIM2
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOC, ENABLE); // GPIOC로 변경
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // [중요] ADC 클럭은 14MHz를 넘으면 안됨. (72MHz / 6 = 12MHz)
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    // 2. GPIO 설정 (PC0 - Analog Input) -> 회로에서 센서를 PC0에 연결하세요!
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure); // GPIOC로 변경

    /* --- DMA 설정 --- */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_buf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ADC_BUF_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* --- ADC 설정 --- */
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC2;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;

    ADC_Init(ADC1, &ADC_InitStructure);

    // [중요] 채널 변경: ADC_Channel_0 (PA0) -> ADC_Channel_10 (PC0)
    ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_7Cycles5);
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    /* --- TIM2 설정 (2kHz 트리거 발생기) --- */
    Sensor_TIM2_Init();

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    /* --- ADC Calibration --- */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

/* ============================
     TIM2 설정 (2kHz) - 변경 없음
   ============================ */
void Sensor_TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 72MHz / 36 / 1000 = 2000Hz (0.5ms 주기)
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 36 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // Output Compare 2번 채널 (ADC 트리거용)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 500;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

/* ============================
     DMA write index 계산
   ============================ */
uint16_t ADC_GetWriteIndex(void)
{
    uint16_t remaining = DMA_GetCurrDataCounter(DMA1_Channel1);
    uint16_t written = ADC_BUF_LEN - remaining;

    if (written >= ADC_BUF_LEN)
        written -= ADC_BUF_LEN;

    return written;
}

/* ============================
     최근 N개 평균 ambient 계산
   ============================ */
uint16_t ADC_GetAmbientAverage(void)
{
    uint32_t sum = 0;
    uint16_t count = AMBIENT_AVG_SAMPLES;
    uint16_t cur = ADC_GetWriteIndex();

    for (uint16_t i = 0; i < count; i++)
    {
        int16_t idx = (int16_t)cur - 1 - (int16_t)i;
        if (idx < 0)
            idx += ADC_BUF_LEN;

        sum += adc_buf[idx];
    }

    return (uint16_t)(sum / count);
}

/* ============================
     외부 API
   ============================ */
uint16_t ReadAmbient(void)
{
    return ADC_GetAmbientAverage();
}

uint16_t ReadPeak(uint16_t start_idx, uint16_t len)
{
    uint16_t min_val = 4095;

    if (len > ADC_BUF_LEN) len = ADC_BUF_LEN;

    for (uint16_t i = 0; i < len; i++)
    {
        uint16_t idx = (start_idx + i) % ADC_BUF_LEN;
        if (adc_buf[idx] < min_val) {
            min_val = adc_buf[idx];
        }
    }
    return min_val;
}
