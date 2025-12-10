#ifndef ESDL_SENSOR_H
#define ESDL_SENSOR_H

#include <stdint.h>

#define ADC_BUF_LEN 256
#define AMBIENT_AVG_SAMPLES 8

void Sensor_ADC_Init(void);

uint16_t ReadAmbient(void);
uint16_t ADC_GetAmbientAverage(void);
uint16_t ADC_GetWriteIndex(void);

extern volatile uint16_t adc_buf[ADC_BUF_LEN];

uint16_t ReadPeak(uint16_t start_idx, uint16_t len);
#endif
