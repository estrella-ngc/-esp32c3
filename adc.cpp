#include "adc.h"

static uint16_t light_sensor_value = 0;
static uint8_t light_sensor_state = 0;

void adc_init(void)
{
  pinMode(ADC_PIN, INPUT);
}

void read_light_sensor(void)
{
  static unsigned long last_read = 0;
  if (millis() - last_read < 200) return;
  last_read = millis();

  light_sensor_value = analogRead(ADC_PIN);

  if (light_sensor_value > LIGHT_OFF_THRESHOLD) {
    light_sensor_state = 1;
  } else if (light_sensor_value < LIGHT_ON_THRESHOLD) {
    light_sensor_state = 0;
  }
}

uint8_t get_light_state(void)
{
  return light_sensor_state;
}

uint16_t get_adc_value(void)
{
  return light_sensor_value;
}