#include "rgb_led.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel strip(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

static uint32_t wheel(uint8_t pos)
{
  pos = 255 - pos;
  if (pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip.Color(pos * 3, 255 - pos * 3, 0);
}

void rgb_led_init(void)
{
  strip.begin();
  strip.clear();
  strip.show();
}

void rgb_led_boot_flash(void)
{
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 256; i += 16) {
      strip.setPixelColor(0, wheel(i & 255));
      strip.show();
      delay(30);
    }
  }
  rgb_led_off();
}

void rgb_led_poweron_flash(void)
{
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 256; i += 16) {
      strip.setPixelColor(0, wheel(i & 255));
      strip.show();
      delay(20);
    }
  }
  rgb_led_off();
}

void rgb_led_set(uint8_t r, uint8_t g, uint8_t b)
{
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void rgb_led_off(void)
{
  strip.clear();
  strip.show();
}
