#include "adc.h"
#include <Preferences.h>

uint16_t light_threshold = 1800;

static uint16_t light_sensor_value = 0;
static uint8_t light_sensor_state = 0;

static void load_threshold(void)
{
  Preferences prefs;
  prefs.begin("threshold", true);
  light_threshold = prefs.getUShort("light", 1800);
  prefs.end();
}

void adc_save_threshold(void)
{
  Preferences prefs;
  prefs.begin("threshold", false);
  prefs.putUShort("light", light_threshold);
  prefs.end();
  Serial.printf("[阈值] 光照阈值已保存: %d\r\n", light_threshold);
}

void adc_init(void)
{
  pinMode(ADC_PIN, INPUT);
  load_threshold();
}

void read_light_sensor(void)
{
  static unsigned long last_read = 0;
  if (millis() - last_read < 200) return;
  last_read = millis();

  light_sensor_value = analogRead(ADC_PIN);

  if (light_sensor_value > light_threshold + 400) {
    light_sensor_state = 1;
  } else if (light_sensor_value < light_threshold) {
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