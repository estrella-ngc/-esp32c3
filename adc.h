#ifndef __ADC_H
#define __ADC_H

#include "Arduino.h"

#define ADC_PIN 1

// 光感阈值（ESP32-C3 12 位 ADC，范围 0~4095）
#define LIGHT_ON_THRESHOLD  1800
#define LIGHT_OFF_THRESHOLD 2200

void adc_init(void);
void read_light_sensor(void);
uint8_t get_light_state(void);
uint16_t get_adc_value(void);

#endif