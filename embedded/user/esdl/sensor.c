#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_tim.h"
#include "sensor.h"

volatile uint16_t adc_buf[ADC_BUF_LEN];

/* ============================
     ADC + DMA + TIM2 Init
   ============================ */
void Sensor_ADC_Init(void)
{
    /* --- DMA 설정 --- */
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)adc_buf;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize         = ADC_BUF_LEN;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;

    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* --- ADC 설정 --- */
    ADC_InitTypeDef ADC_InitStructure;

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;      // Trigger mode 사용
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC2;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;

    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(
        ADC1,
        ADC_Channel_0,
        1,
        ADC_SampleTime_7Cycles5
    );

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    /* --- ADC Calibration --- */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    /* 이제 TIM2 trigger가 들어오면 DMA로 자동 저장됨 */
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
        int16_t idx = cur - 1 - i;
        if (idx < 0)
            idx += ADC_BUF_LEN;

        sum += adc_buf[idx];
    }

    return (uint16_t)(sum / count);
}

/* ============================
     외부에서 호출할 API
   ============================ */
uint16_t ReadAmbient(void)
{
    return ADC_GetAmbientAverage();
}
