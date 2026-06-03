#ifndef __ADC_H
#define __ADC_H

#include "Arduino.h"

#define ADC_PIN 1

extern uint16_t light_threshold;

void adc_init(void);
void read_light_sensor(void);
uint8_t get_light_state(void);
uint16_t get_adc_value(void);

#endif