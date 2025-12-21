#include "game.h"
#include "sensor.h"
#include "time.h"
#include "uart.h"  
#include <stdio.h>  


static int is_capturing = 0;       // 현재 측정 중인지 확인하는 플래그
static uint32_t capture_start_time = 0; // 측정 시작 시간
static uint32_t current_peak = 0;  // 측정 구간 내 최대값 저장



void Game_Init(void)
{
    is_capturing = 0;
    current_peak = 0;
}

// Trigger.c 에서 호출
void Game_StartCapture(void)
{
    is_capturing = 1;
    capture_start_time = millis();
    current_peak = 0; // 피크값 초기화
}

void Game_Loop(void)
{
    // 측정 모드가 아니면 아무것도 안 함
    if (is_capturing == 0) return;

    // 1. 현재 센서값 읽기 (sensor.c의 최신값)
    uint16_t currentIdx = ADC_GetWriteIndex();
    uint16_t dataIdx = (currentIdx == 0) ? (ADC_BUF_LEN - 1) : (currentIdx - 1);
    uint16_t sensorVal = adc_buf[dataIdx];

    // 2. 최대값 갱신 (Peak Hold)
    if (sensorVal > current_peak)
    {
        current_peak = sensorVal;
    }

    // 3. 시간 체크 (120ms 지났는지?)
    if (millis() - capture_start_time >= 120)
    {
        // 120ms 지남 -> 결과 전송
        char buf[32];
        snprintf(buf, sizeof(buf), "PEAK=%lu\r\n", current_peak);
        USART1_SendString(buf);

        // 측정 종료
        is_capturing = 0;
    }
}

