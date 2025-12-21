#include "game.h"

// 120ms 분량의 데이터를 저장할 버퍼 크기 (1ms마다 저장 가정)
#define PEAK_WINDOW_SIZE 120 

static uint16_t sensor_buffer[PEAK_WINDOW_SIZE] = { 0, };
static int buf_head = 0;

// [필수] 메인 루프나 타이머에서 ADC 값을 읽을 때마다 계속 호출해줘야 함
void Game_UpdateSensor(uint16_t value)
{
    sensor_buffer[buf_head] = value;
    // 원형 버퍼 인덱스 관리 (0 ~ 119)
    buf_head = (buf_head + 1) % PEAK_WINDOW_SIZE;
}

// [trigger.c에서 호출] 최근 저장된 값 중 최대값(Peak) 리턴
uint32_t Game_GetRecentPeak(void)
{
    uint32_t max_val = 0;

    // 버퍼 전체를 훑어서 최대값 찾기
    for (int i = 0; i < PEAK_WINDOW_SIZE; i++)
    {
        if (sensor_buffer[i] > max_val)
        {
            max_val = sensor_buffer[i];
        }
    }

    return max_val;
}