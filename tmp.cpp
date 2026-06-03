#include "tmp.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

float temp_threshold = 34.0f;

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);
static float temperature = 0.0f;
static uint8_t sensor_ok = 0;
static uint8_t fan_state = 0;

static void load_threshold(void)
{
  Preferences prefs;
  prefs.begin("threshold", true);
  temp_threshold = prefs.getFloat("temp", 34.0f);
  prefs.end();
}

void tmp_save_threshold(void)
{
  Preferences prefs;
  prefs.begin("threshold", false);
  prefs.putFloat("temp", temp_threshold);
  prefs.end();
  Serial.printf("[阈值] 温度阈值已保存: %.1f\r\n", temp_threshold);
}

static uint8_t converting = 0;
static unsigned long convert_start = 0;

void tmp_init(void)
{
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensor_ok = 1;
  load_threshold();
}

void read_temperature(void)
{
  unsigned long now = millis();

  if (!converting) {
    if (now - convert_start < 5000) return;
    sensors.requestTemperatures();
    converting = 1;
    return;
  }

  if (!sensors.isConversionComplete()) return;

  converting = 0;
  convert_start = now;
  temperature = sensors.getTempCByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    temperature = -127.0f;
    sensor_ok = 0;
    fan_state = 0;
    return;
  }

  sensor_ok = 1;

  if (!fan_state && temperature > temp_threshold) {
    fan_state = 1;
  } else if (fan_state && temperature < temp_threshold - 0.5f) {
    fan_state = 0;
  }
}

float get_temperature(void)
{
  return temperature;
}

uint8_t get_fan_state(void)
{
  return fan_state;
}
