#include "game.h"
#include "sensor.h" 
#include "time.h"   

#define PEAK_WINDOW_SIZE 120 

static uint16_t sensor_buffer[PEAK_WINDOW_SIZE] = { 0, };
static int buf_head = 0;

void Game_UpdateSensor(uint16_t value)
{
    sensor_buffer[buf_head] = value;
    buf_head = (buf_head + 1) % PEAK_WINDOW_SIZE;
}

void Game_Init(void)
{
    for (int i = 0; i < PEAK_WINDOW_SIZE; i++) sensor_buffer[i] = 0;
    buf_head = 0;
}

void Game_Loop(void)
{
    static uint32_t last_update_time = 0;

    // 1ms마다 센서 값을 읽어서 버퍼에 저장
    if (millis() - last_update_time >= 1)
    {
        last_update_time = millis();

        uint16_t currentIdx = ADC_GetWriteIndex();
        uint16_t dataIdx = (currentIdx == 0) ? (ADC_BUF_LEN - 1) : (currentIdx - 1);

        Game_UpdateSensor(adc_buf[dataIdx]);
    }
}

uint32_t Game_GetRecentPeak(void)
{
    uint32_t max_val = 0;
    for (int i = 0; i < PEAK_WINDOW_SIZE; i++)
    {
        if (sensor_buffer[i] > max_val)
        {
            max_val = sensor_buffer[i];
        }
    }
    return max_val;
}