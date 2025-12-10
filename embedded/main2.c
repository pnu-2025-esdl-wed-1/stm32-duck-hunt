/*
  Project: Retro Duck Hunt System Firmware (Final)
  Target : STM32F103RB (Cortex-M3)
 
  ��� ���
  1) ADC + DMA + TIM2(2kHz ?����)�� �������� ����� (Circular Buffer)
  2) ?����(PC4) �� TRIG,seq,ambient ���� + ���� ����
  3) PC ACK(���� seq) ���� �� 120ms ������ ����� ����
  4) DMA ���?��� 120ms ����(min ADC = peak) �?� �� hit ���� �� RESULT ����
  5) PC STATUS �?����� ���������� �?�
  6) ����?�: IDLE (���) / WAIT_ACK (�?� ��? ����) / SAMPLING (?�� ����� ���� ��)
 */

#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

 /* ==========================================================================
   Config & Constants
   ========================================================================== */

  // DMA�� ���� �� ������ ���� ���� ( ���� 256�� ����)
#define ADC_BUF_LEN             256                   //512�� ����� �?� ��

// PC ����� RX ����
#define RX_BUF_SIZE             128

// ADC ����� ���?� (TIM2�� 2kHz ?����)    -> TIM2�� 2kHz ����� (0.5ms ����) �??� ����
#define ADC_SAMPLE_RATE_HZ      2000

// ACK ���� ����� ������ (ms)
#define SAMPLE_WINDOW_MS        120

// 120ms ���� ���� ���� (2kHz * 0.12s �� 240)
#define SAMPLE_WINDOW_SAMPLES   ((ADC_SAMPLE_RATE_HZ * SAMPLE_WINDOW_MS) / 1000)

// ambient ��?� ����� ���� ����
#define AMBIENT_AVG_SAMPLES     8

// ���� ���� �?? (peak(ADC ��)�� �� ������ ������ hit=1)
#define HIT_THRESHOLD           1000                             // ?���?�!!!!!! (��?���� ���� ?)



typedef enum {
    SHOT_IDLE = 0,     // �?� �?� �� �?� ����
    SHOT_WAIT_ACK,     // TRIG ���� �� PC���� ACK ��?��� ��
    SHOT_SAMPLING      // ACK ���� �� 120ms ����� ��
} ShotState;

volatile uint32_t msTicks = 0;                  // SysTick millis
volatile uint16_t adc_buf[ADC_BUF_LEN];         // ADC ���� �����?� DMA circular ����

volatile uint32_t sequence_num = 0;        // ��� �� ������
volatile uint32_t current_shot_seq = 0;        // ���� � ���� �� ��?  >> TRIG/ACK/RESULT ��� �?� ������� �� 

volatile uint8_t  flag_trigger = 0;        // ?���� �?� (ISR �� main)  , EXTI ���?�?���� 1�� ����. main������ ?���� �
volatile uint8_t  flag_ack_matched = 0;        // �?� ���� ���� ACK ����, ACK ����, seq�� ���� �� 1�� ���� 
volatile uint8_t  flag_pc_msg_ready = 0;        // USART1 ISR���� �� �� ���� �?�?� 1
volatile ShotState shot_state = SHOT_IDLE;     // ���� �� ����

// ����� ����
uint32_t sampling_start_time = 0;              // ACK ���� ������ miilis(). 120ms �� ����
uint16_t sampling_start_index = 0;              // ACK ���� ���� DMA write index   -> �� �?������� 240 ���� ��?
uint16_t min_peak_value = 4095;           // ����� �������� �?� ADC (���� ���� ��)

// PC RX ����
char usart1_rx_buf[RX_BUF_SIZE];     //PC���� ���� ���?��� �����?� ����/�?���
volatile uint8_t usart1_rx_idx = 0;    // ISR���� �?��?� ��?� \n ������ ���?� �?�.



void System_Init_All(void);
void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void USART1_Init(void);
void USART2_Init(void);
void TIM2_Config(void);
void ADC_DMA_Config(void);
void EXTI_Configure(void);
void SysTick_Init(void);

uint32_t millis(void);
void Delay(uint32_t ms);
void USART_SendString(USART_TypeDef* USARTx, const char* str);
void Process_PC_Message(char* msg);
void Motor_Control(uint8_t state);

static inline uint16_t ADC_GetWriteIndex(void);
uint16_t ADC_GetAmbientAverage(void);
int Parse_Seq_From_Message(const char* msg);


int main(void)
{
    System_Init_All();

    char tx_buf[64];

    while (1)
    {

        if (flag_trigger && shot_state == SHOT_IDLE)   //IDLE �����?���
        {
            flag_trigger = 0;

            sequence_num++;
            current_shot_seq = sequence_num;
            shot_state = SHOT_WAIT_ACK;      //ACK�� ����

            // ���� ���� ��� ON
            Motor_Control(1);     //PA1 �� HIGH
            Delay(50);
            Motor_Control(0);     //���� off

            // DMA ���?��� �?� ���� 8�� ��� �� ambient ���������� ���
            uint16_t ambient = ADC_GetAmbientAverage();   

            // TRIG,seq=...,ambient=...
            sprintf(tx_buf, "TRIG,seq=%lu,ambient=%u\r\n",
                (unsigned long)current_shot_seq, ambient);
            USART_SendString(USART1, tx_buf);   //PC�� ����
        }

        // PC �?��� � (ACK, STATUS)
        if (flag_pc_msg_ready)
        {
            flag_pc_msg_ready = 0;
            Process_PC_Message(usart1_rx_buf);
        }

        // ACK ���� �� ����� ����
        if (flag_ack_matched && shot_state == SHOT_WAIT_ACK)
        {
            flag_ack_matched = 0;
            shot_state = SHOT_SAMPLING;
            sampling_start_time = millis();
            sampling_start_index = ADC_GetWriteIndex();
            min_peak_value = 4095;
        }

         // ����� ������ �� �� peak ��� & RESULT ����

        if (shot_state == SHOT_SAMPLING)
        {
            if ((millis() - sampling_start_time) >= SAMPLE_WINDOW_MS) // 120ms �������� ����� ���� ���� ������ ��
            {
                uint16_t samples = SAMPLE_WINDOW_SAMPLES;
                if (samples > ADC_BUF_LEN) samples = ADC_BUF_LEN;

                min_peak_value = 4095;

                for (uint16_t i = 0; i < samples; i++)        // DMA ���� ���� ��?
                {
                    uint16_t idx = (sampling_start_index + i) % ADC_BUF_LEN; // ���� ���� �?��� ���
                    uint16_t v = adc_buf[idx];   // �� ���� �� �?�
                    if (v < min_peak_value)      // �??� ����
                        min_peak_value = v;     
                }

                int is_hit = (min_peak_value < HIT_THRESHOLD) ? 1 : 0; 

                sprintf(tx_buf, "RESULT,seq=%lu,peak=%u,hit=%d\r\n",             //RESULT �?��� ���� & ����
                    (unsigned long)current_shot_seq, min_peak_value, is_hit);
                USART_SendString(USART1, tx_buf);

                shot_state = SHOT_IDLE;    // �?� IDLE ����� ���� -> ���� �� ���
            }
        }
    }
}


void System_Init_All(void)
{
    SystemInit();
    SysTick_Init();
    RCC_Configure();
    GPIO_Configure();
    USART1_Init();      // PC
    USART2_Init();      // Bluetooth
    TIM2_Config();      // ADC ?���?� 2kHz ��?�
    ADC_DMA_Config();   // �������� ADC+DMA
    EXTI_Configure();   // ?���� ��?
    NVIC_Configure();   // ���?�? �?����
}

void RCC_Configure(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC |
        RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO |
        RCC_APB2Periph_ADC1 | RCC_APB2Periph_USART1,
        ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_TIM2,
        ENABLE);

    // ADC ?��= PCLK2(72MHz)/6 = 12MHz �� ����   >> F1 ADC �?� ?�� (14MHz) ���?� ���?� ���?�
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // PA0: ADC �?� (���� ����)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PC4: ?���� (?�� �?�, ������ �� LOW�� ����)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // PA1: ���� ���� (���)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART1: PA9(TX), PA10(RX)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART2: PD5(TX), PD6(RX) ����
    GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; // TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; // RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void TIM2_Config(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    // 72MHz / 36 / 1000 = 2kHz
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 36 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // CH2 PWM (ADC ?���?�)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 500;  // ��? 50%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void ADC_DMA_Config(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    // DMA1 Channel1: ADC1 �� adc_buf[]
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

    // ADC1 ����: TIM2 CC2 ?����, ���� ��? OFF
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // �?�
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_CC2;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_7Cycles5);

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    // ����?���� ?���� ���� TIM2 CC2�� ���� ��? ����
}

void USART1_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

void USART2_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

void EXTI_Configure(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // ?�?� ��� ����
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Configure(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // USART1
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // EXTI4 (Trigger)
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

 // ISRs

 // PC(USART1) RX: �� �� �?��?� flag�� ����
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char data = USART_ReceiveData(USART1);

        if (usart1_rx_idx < RX_BUF_SIZE - 1)
            usart1_rx_buf[usart1_rx_idx++] = data;

        if (data == '\n' || data == '\r')
        {
            usart1_rx_buf[usart1_rx_idx] = '\0';
            if (usart1_rx_idx > 0)
                flag_pc_msg_ready = 1;

            usart1_rx_idx = 0;
        }
    }
}

// ?���� ��?(PC4)
void EXTI4_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        static uint32_t last_trigger_time = 0;
        uint32_t now = millis();

        if (now - last_trigger_time > 200)
        {
            flag_trigger = 1;
            last_trigger_time = now;
        }

        EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

 // Utils & Helpers

void Process_PC_Message(char* msg)
{
    // ACK,seq=...
    if (strncmp(msg, "ACK", 3) == 0)
    {
        int rx_seq = Parse_Seq_From_Message(msg);
        if (rx_seq > 0 &&
            (uint32_t)rx_seq == current_shot_seq &&
            shot_state == SHOT_WAIT_ACK)
        {
            flag_ack_matched = 1;
        }
    }
    // STATUS,score=...,time=...
    else if (strncmp(msg, "STATUS", 6) == 0)
    {
        // ���������� �?�� ������
        USART_SendString(USART2, msg);
        if (msg[strlen(msg) - 1] != '\n')
            USART_SendString(USART2, "\r\n");
    }
}

void USART_SendString(USART_TypeDef* USARTx, const char* str)
{
    while (*str)
    {
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
        USART_SendData(USARTx, *str++);
    }
}

void Motor_Control(uint8_t state)
{
    if (state)
        GPIO_SetBits(GPIOA, GPIO_Pin_1);
    else
        GPIO_ResetBits(GPIOA, GPIO_Pin_1);
}

/* SysTick 1ms */

void SysTick_Init(void)
{
    if (SysTick_Config(SystemCoreClock / 1000))
    {
        while (1); // ���� �� ����
    }
}

void SysTick_Handler(void)
{
    msTicks++;
}

uint32_t millis(void)
{
    return msTicks;
}

void Delay(uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) < ms);
}

/* DMA ���� write index ���
 * CNDTR = ���� ?��? �� (�� - ����) = �� ����
 */
static inline uint16_t ADC_GetWriteIndex(void)
{
    uint16_t remaining = DMA_GetCurrDataCounter(DMA1_Channel1);
    uint16_t written = (uint16_t)(ADC_BUF_LEN - remaining);
    if (written >= ADC_BUF_LEN)
        written -= ADC_BUF_LEN;
    return written;
}

/* ambient = �?� ���� �������� �?��� N�� ��� */
uint16_t ADC_GetAmbientAverage(void)
{
    uint32_t sum = 0;
    uint16_t count = (AMBIENT_AVG_SAMPLES < ADC_BUF_LEN)
        ? AMBIENT_AVG_SAMPLES
        : ADC_BUF_LEN;

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

/* "...,seq=23,..." ���� 23 �?� */
int Parse_Seq_From_Message(const char* msg)
{
    const char* p = strstr(msg, "seq=");
    if (!p) return -1;
    return atoi(p + 4);
}
