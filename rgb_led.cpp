#include "rgb_led.h"
#include "exit.h"
#include "led.h"
#include "relay.h"
#include "tasks.h"
#include "wifi_manager.h"
#include "mqtt.h"
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

// ----- Animation patterns -----

static uint8_t lerp(uint8_t a, uint8_t b, uint32_t t, uint32_t dur)
{
  if (t >= dur) return b;
  return a + (int16_t)(b - a) * t / dur;
}

struct AnimState {
  uint8_t step;
  unsigned long step_start;
};

static AnimState anim = {0, 0};
static uint8_t last_pattern = 0xFF;

static void reset_anim(void)
{
  anim.step = 0;
  anim.step_start = millis();
}

static void check_step(const uint16_t* durations, uint8_t num_steps)
{
  uint32_t now = millis();
  if (now - anim.step_start >= durations[anim.step]) {
    anim.step = (anim.step + 1) % num_steps;
    anim.step_start = now;
  }
}

void rgb_led_task(void)
{
  uint8_t led_st = digitalRead(LED_PIN);
  uint8_t relay_st = digitalRead(RELAY_PIN);
  uint8_t wifi_ok = wifi_is_connected();
  uint8_t mqtt_ok = mqtt_client_connected();

  uint8_t pattern;

  if (led_st && relay_st) {
    pattern = 1;
  } else if (led_st) {
    pattern = 2;
  } else if (relay_st) {
    pattern = 3;
  } else if (!mqtt_ok && wifi_ok) {
    pattern = 4;
  } else if (!wifi_ok) {
    pattern = 5;
  } else {
    uint8_t enabled;
    portENTER_CRITICAL(&shared_mux);
    enabled = system_enabled;
    portEXIT_CRITICAL(&shared_mux);
    if (enabled) {
      pattern = 6;
    } else {
      rgb_led_off();
      last_pattern = 0;
      return;
    }
  }

  if (pattern != last_pattern) {
    last_pattern = pattern;
    reset_anim();
  }

  uint32_t now = millis();
  uint32_t elapsed = now - anim.step_start;

  switch (pattern) {

    case 1: {
      static const uint8_t cols[12][3] = {
        {80,0,0},{0,0,0},{80,0,0},{0,0,0},
        {0,80,0},{0,0,0},{0,80,0},{0,0,0},
        {0,0,80},{0,0,0},{0,0,80},{0,0,0}
      };
      static const uint16_t durs[12] = {100,50,100,50,100,50,100,50,100,50,100,350};
      check_step(durs, 12);
      strip.setPixelColor(0, strip.Color(cols[anim.step][0], cols[anim.step][1], cols[anim.step][2]));
      break;
    }

    case 2: {
      static const uint8_t cols[6][3] = {
        {80,0,0},{0,0,0},{0,80,0},{0,0,0},{0,0,80},{0,0,0}
      };
      static const uint16_t durs[6] = {300,200,300,200,300,200};
      check_step(durs, 6);
      strip.setPixelColor(0, strip.Color(cols[anim.step][0], cols[anim.step][1], cols[anim.step][2]));
      break;
    }

    case 3: {
      static const uint8_t cols[4][3] = {
        {80,0,0},{0,80,0},{0,0,80},{0,0,0}
      };
      static const uint16_t durs[4] = {500,500,500,700};
      check_step(durs, 4);
      strip.setPixelColor(0, strip.Color(cols[anim.step][0], cols[anim.step][1], cols[anim.step][2]));
      break;
    }

    case 4: {
      static const uint8_t fade[4][6] = {
        {80,0,0, 0,80,0},
        {0,80,0, 0,0,0},
        {0,0,0, 0,0,80},
        {0,0,80, 0,0,0}
      };
      uint16_t step_dur = 500;
      if (elapsed >= step_dur) {
        anim.step = (anim.step + 1) % 4;
        anim.step_start = now;
        elapsed = 0;
      }
      uint8_t r = lerp(fade[anim.step][0], fade[anim.step][3], elapsed, step_dur);
      uint8_t g = lerp(fade[anim.step][1], fade[anim.step][4], elapsed, step_dur);
      uint8_t b = lerp(fade[anim.step][2], fade[anim.step][5], elapsed, step_dur);
      strip.setPixelColor(0, strip.Color(r, g, b));
      break;
    }

    case 5: {
      static const uint8_t cols[3][3] = {{80,0,0},{0,80,0},{0,0,0}};
      static const uint16_t durs[3] = {200,200,500};
      check_step(durs, 3);
      strip.setPixelColor(0, strip.Color(cols[anim.step][0], cols[anim.step][1], cols[anim.step][2]));
      break;
    }

    case 6: {
      static const uint16_t durs[5] = {1000,1000,1000,1000,200};
      if (elapsed >= durs[anim.step]) {
        anim.step = (anim.step + 1) % 5;
        anim.step_start = now;
        elapsed = 0;
      }
      if (anim.step == 4) {
        strip.setPixelColor(0, strip.Color(0, 0, 0));
      } else {
        static const uint8_t fade[4][6] = {
          {0,0,0, 80,0,0},
          {80,0,0, 0,80,0},
          {0,80,0, 0,0,80},
          {0,0,80, 0,0,0}
        };
        uint8_t r = lerp(fade[anim.step][0], fade[anim.step][3], elapsed, durs[anim.step]);
        uint8_t g = lerp(fade[anim.step][1], fade[anim.step][4], elapsed, durs[anim.step]);
        uint8_t b = lerp(fade[anim.step][2], fade[anim.step][5], elapsed, durs[anim.step]);
        strip.setPixelColor(0, strip.Color(r, g, b));
      }
      break;
    }
  }

  strip.show();
}
