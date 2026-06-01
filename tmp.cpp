#include "tmp.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define FAN_ON_TEMP  35.0f
#define FAN_OFF_TEMP 34.9f

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);
static float temperature = 0.0f;
static uint8_t sensor_ok = 0;
static uint8_t fan_state = 0;

void tmp_init(void)
{
  sensors.begin();
  sensor_ok = 1;
}

void read_temperature(void)
{
  static unsigned long last_read = 0;
  if (millis() - last_read < 5000) return;
  last_read = millis();

  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    temperature = -127.0f;
    sensor_ok = 0;
    fan_state = 0;
    return;
  }

  sensor_ok = 1;

  if (!fan_state && temperature > FAN_ON_TEMP) {
    fan_state = 1;
  } else if (fan_state && temperature < FAN_OFF_TEMP) {
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
