/*
  Project: Retro Duck Hunt System Firmware (Final)
  Target : STM32F103RB (Cortex-M3)
 
  기능 요약
  1) ADC + DMA + TIM2(2kHz 트리거)로 조도센서 샘플링 (Circular Buffer)
  2) 트리거(PC4) → TRIG,seq,ambient 전송 + 모터 진동
  3) PC ACK(같은 seq) 수신 → 120ms 윈도우 샘플링 시작
  4) DMA 버퍼에서 120ms 구간(min ADC = peak) 분석 → hit 판정 → RESULT 전송
  5) PC STATUS 메시지는 블루투스로 중계
  6) 상태머신: IDLE (대기) / WAIT_ACK (발사 신호 보냄) / SAMPLING (화면 플래시 감지 중)
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

  // DMA가 조도 값 저장할 버퍼 길이 ( 샘플 256개 보관)
#define ADC_BUF_LEN             256                   //512로 늘려도 되긴 함

// PC 시리얼 RX 버퍼
#define RX_BUF_SIZE             128

// ADC 샘플링 주파수 (TIM2로 2kHz 트리거)    -> TIM2로 2kHz 샘플링 (0.5ms 간격) 한다고 가정
#define ADC_SAMPLE_RATE_HZ      2000

// ACK 이후 샘플링 윈도우 (ms)
#define SAMPLE_WINDOW_MS        120

// 120ms 동안 샘플 개수 (2kHz * 0.12s ≒ 240)
#define SAMPLE_WINDOW_SAMPLES   ((ADC_SAMPLE_RATE_HZ * SAMPLE_WINDOW_MS) / 1000)

// ambient 평균에 사용할 샘플 개수
#define AMBIENT_AVG_SAMPLES     8

// 명중 판정 임계값 (peak(ADC 값)이 이 값보다 작으면 hit=1)
#define HIT_THRESHOLD           1000                             // 튜닝하기!!!!!! (어두울수록 숫자 큼)



typedef enum {
    SHOT_IDLE = 0,     // 아무 것도 안 하는 상태
    SHOT_WAIT_ACK,     // TRIG 보낸 후 PC에서 ACK 기다리는 중
    SHOT_SAMPLING      // ACK 받은 후 120ms 샘플링 중
} ShotState;

volatile uint32_t msTicks = 0;                  // SysTick millis
volatile uint16_t adc_buf[ADC_BUF_LEN];         // ADC 샘플 저장하는 DMA circular 버퍼

volatile uint32_t sequence_num = 0;        // 전체 샷 시퀀스
volatile uint32_t current_shot_seq = 0;        // 현재 처리 중인 샷 번호  >> TRIG/ACK/RESULT 모두 이걸 기반으로 함 

volatile uint8_t  flag_trigger = 0;        // 트리거 발생 (ISR → main)  , EXTI 인터럽트에서 1로 세팅. main루프가 트리거 처리
volatile uint8_t  flag_ack_matched = 0;        // 이번 샷에 대한 ACK 수신, ACK 오고, seq도 맞을 때 1로 세팅 
volatile uint8_t  flag_pc_msg_ready = 0;        // USART1 ISR에서 한 줄 수신 완료되면 1
volatile ShotState shot_state = SHOT_IDLE;     // 현재 샷 상태

// 샘플링 관련
uint32_t sampling_start_time = 0;              // ACK 받은 시점의 miilis(). 120ms 비교 기준
uint16_t sampling_start_index = 0;              // ACK 받은 시점 DMA write index   -> 이 인덱스부터 240 샘플 스캔
uint16_t min_peak_value = 4095;           // 샘플링 구간에서 최소 ADC (가장 밝은 값)

// PC RX 버퍼
char usart1_rx_buf[RX_BUF_SIZE];     //PC에서 들어온 문자열을 저장하는 버퍼/인덱스
volatile uint8_t usart1_rx_idx = 0;    // ISR에서 한글자씩 담다가 \n 들어오면 문자열 완성.



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

        if (flag_trigger && shot_state == SHOT_IDLE)   //IDLE 상태일때만
        {
            flag_trigger = 0;

            sequence_num++;
            current_shot_seq = sequence_num;
            shot_state = SHOT_WAIT_ACK;      //ACK로 변경

            // 진동 모터 짧게 ON
            Motor_Control(1);     //PA1 핀 HIGH
            Delay(50);
            Motor_Control(0);     //모터 off

            // DMA 버퍼에서 최신 샘플 8개 평균 → ambient 조도값으로 사용
            uint16_t ambient = ADC_GetAmbientAverage();   

            // TRIG,seq=...,ambient=...
            sprintf(tx_buf, "TRIG,seq=%lu,ambient=%u\r\n",
                (unsigned long)current_shot_seq, ambient);
            USART_SendString(USART1, tx_buf);   //PC로 전송
        }

        // PC 메시지 처리 (ACK, STATUS)
        if (flag_pc_msg_ready)
        {
            flag_pc_msg_ready = 0;
            Process_PC_Message(usart1_rx_buf);
        }

        // ACK 수신 후 샘플링 시작
        if (flag_ack_matched && shot_state == SHOT_WAIT_ACK)
        {
            flag_ack_matched = 0;
            shot_state = SHOT_SAMPLING;
            sampling_start_time = millis();
            sampling_start_index = ADC_GetWriteIndex();
            min_peak_value = 4095;
        }

         // 샘플링 윈도우 끝 → peak 계산 & RESULT 전송

        if (shot_state == SHOT_SAMPLING)
        {
            if ((millis() - sampling_start_time) >= SAMPLE_WINDOW_MS) // 120ms 지났으면 샘플링 구간 끝난 것으로 봄
            {
                uint16_t samples = SAMPLE_WINDOW_SAMPLES;
                if (samples > ADC_BUF_LEN) samples = ADC_BUF_LEN;

                min_peak_value = 4095;

                for (uint16_t i = 0; i < samples; i++)        // DMA 버퍼 구간 스캔
                {
                    uint16_t idx = (sampling_start_index + i) % ADC_BUF_LEN; // 원형 버퍼 인덱스 계산
                    uint16_t v = adc_buf[idx];   // 각 샘플 값 읽고
                    if (v < min_peak_value)      // 최소값 갱신
                        min_peak_value = v;     
                }

                int is_hit = (min_peak_value < HIT_THRESHOLD) ? 1 : 0; 

                sprintf(tx_buf, "RESULT,seq=%lu,peak=%u,hit=%d\r\n",             //RESULT 메시지 생성 & 전송
                    (unsigned long)current_shot_seq, min_peak_value, is_hit);
                USART_SendString(USART1, tx_buf);

                shot_state = SHOT_IDLE;    // 다시 IDLE 상태로 복귀 -> 다음 샷 대기
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
    TIM2_Config();      // ADC 트리거용 2kHz 타이머
    ADC_DMA_Config();   // 조도센서 ADC+DMA
    EXTI_Configure();   // 트리거 버튼
    NVIC_Configure();   // 인터럽트 우선순위
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

    // ADC 클럭= PCLK2(72MHz)/6 = 12MHz 로 설정   >> F1 ADC 최대 클럭 (14MHz) 이하로 맞추기 위해서
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // PA0: ADC 입력 (조도 센서)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PC4: 트리거 (풀업 입력, 눌렀을 때 LOW로 가정)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // PA1: 진동 모터 (출력)
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

    // USART2: PD5(TX), PD6(RX) 리맵
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

    // CH2 PWM (ADC 트리거용)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 500;  // 듀티 50%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void ADC_DMA_Config(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    // DMA1 Channel1: ADC1 → adc_buf[]
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

    // ADC1 설정: TIM2 CC2 트리거, 연속 변환 OFF
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 중요
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

    // 소프트웨어 트리거 없이 TIM2 CC2에 의해 변환 시작
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
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 회로에 맞게 조정
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

 // PC(USART1) RX: 한 줄 완성되면 flag만 세움
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

// 트리거 버튼(PC4)
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
        // 블루투스로 그대로 포워딩
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
        while (1); // 에러 시 멈춤
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

/* DMA 현재 write index 계산
 * CNDTR = 남은 카운트 → (총 - 남은) = 쓴 개수
 */
static inline uint16_t ADC_GetWriteIndex(void)
{
    uint16_t remaining = DMA_GetCurrDataCounter(DMA1_Channel1);
    uint16_t written = (uint16_t)(ADC_BUF_LEN - remaining);
    if (written >= ADC_BUF_LEN)
        written -= ADC_BUF_LEN;
    return written;
}

/* ambient = 최신 샘플 기준으로 뒤에서 N개 평균 */
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

/* "...,seq=23,..." 에서 23 파싱 */
int Parse_Seq_From_Message(const char* msg)
{
    const char* p = strstr(msg, "seq=");
    if (!p) return -1;
    return atoi(p + 4);
}
