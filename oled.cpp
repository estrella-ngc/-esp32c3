#include "oled.h"
#include "exit.h"
#include "adc.h"
#include "tmp.h"
#include "led.h"
#include "relay.h"
#include "wifi_manager.h"
#include <U8g2lib.h>
#include <Wire.h>

#define I2C_SDA 4
#define I2C_SCL 5

static U8G2_SH1106_128X64_VCOMH0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define CYCLE_NORMAL 0
#define CYCLE_HOTSPOT 1

static uint8_t cycle_state = CYCLE_NORMAL;
static unsigned long cycle_start = 0;
static uint8_t cycle_serial_printed = 0;
static unsigned long ap_show_time = 0;

void oled_init(void)
{
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(800000);
  u8g2.begin();
  u8g2.setContrast(0x60);
  u8g2.setPowerSave(0);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
}

static uint8_t is_online(void)
{
  if (wifi_is_connected()) return 1;
  if (wifi_is_ap_mode()) return 1;
  return 0;
}

static void draw_normal(const char* wifi_label, int countdown)
{
  char buf[32];
  if (countdown > 0) {
    snprintf(buf, sizeof(buf), "WiFi已连:%s %ds", wifi_label, countdown);
  } else {
    snprintf(buf, sizeof(buf), "WiFi已连:%s", wifi_label);
  }
  u8g2.drawUTF8(0, 10, buf);

  snprintf(buf, sizeof(buf), "总闸:%s  模式:%s",
    system_enabled ? "ON" : "OFF",
    auto_mode ? "自动" : "手动");
  u8g2.drawUTF8(0, 22, buf);

  uint16_t adc = get_adc_value();
  unsigned int brightness = (4095UL - adc) * 1000 / 4095;
  snprintf(buf, sizeof(buf), "光照:%d/463", brightness);
  u8g2.drawUTF8(0, 34, buf);

  snprintf(buf, sizeof(buf), "温度:%.1f/34.0", get_temperature());
  u8g2.drawUTF8(0, 46, buf);

  snprintf(buf, sizeof(buf), "灯光:%s  风扇:%s",
    digitalRead(LED_PIN) ? "ON" : "OFF",
    digitalRead(RELAY_PIN) ? "ON" : "OFF");
  u8g2.drawUTF8(0, 58, buf);
}

static void draw_ap_info(int remaining)
{
  char buf[32];
  u8g2.drawUTF8(0, 10, "可以连接热点:");
  u8g2.drawUTF8(0, 22, "PCT-100-003");
  u8g2.drawUTF8(0, 34, "密码:12345678");
  u8g2.drawUTF8(0, 46, "通过192.168.4.1");
  snprintf(buf, sizeof(buf), "访问控制器  %ds", remaining);
  u8g2.drawUTF8(0, 58, buf);
}

void oled_update(void)
{
  static unsigned long last_update = 0;
  if (millis() - last_update < 200) return;
  last_update = millis();

  char buf[32];
  u8g2.clearBuffer();

  if (wifi_is_ap_mode()) {
    if (ap_show_time == 0) {
      ap_show_time = millis();
      Serial.println();
      Serial.println("====================================");
      Serial.println("  可以连接热点：PCT-100-003");
      Serial.println("  密码：12345678");
      Serial.println("  通过192.168.4.1 访问控制器");
      Serial.println("====================================");
    }
    unsigned long t = millis() - ap_show_time;
    int phase = (t / 15000) % 2;
    int remaining = 15 - (t % 15000) / 1000;
    if (phase == 0) {
      draw_normal("PCT-100-003", remaining);
    } else {
      draw_ap_info(remaining);
    }
    u8g2.sendBuffer();
    return;
  }
  ap_show_time = 0;

  if (wifi_is_connected()) {
    draw_normal(wifi_get_ssid(), 0);
    u8g2.sendBuffer();
    return;
  }

  if (cycle_start == 0) {
    cycle_start = millis();
    cycle_state = CYCLE_NORMAL;
    cycle_serial_printed = 0;
  }

  unsigned long elapsed = millis() - cycle_start;

  if (cycle_state == CYCLE_NORMAL) {
    int remaining = 10 - elapsed / 1000;
    if (remaining <= 0) {
      cycle_state = CYCLE_HOTSPOT;
      cycle_start = millis();
      cycle_serial_printed = 0;
      remaining = 0;
    }
    snprintf(buf, sizeof(buf), "WiFi未连接 %ds", remaining);
    u8g2.drawUTF8(0, 10, buf);

    snprintf(buf, sizeof(buf), "总闸:%s  模式:%s",
      system_enabled ? "ON" : "OFF",
      auto_mode ? "自动" : "手动");
    u8g2.drawUTF8(0, 22, buf);

    uint16_t adc = get_adc_value();
    unsigned int brightness = (4095UL - adc) * 1000 / 4095;
    snprintf(buf, sizeof(buf), "光照:%d/463", brightness);
    u8g2.drawUTF8(0, 34, buf);

    snprintf(buf, sizeof(buf), "温度:%.1f/34.0", get_temperature());
    u8g2.drawUTF8(0, 46, buf);

    snprintf(buf, sizeof(buf), "灯光:%s  风扇:%s",
      digitalRead(LED_PIN) ? "ON" : "OFF",
      digitalRead(RELAY_PIN) ? "ON" : "OFF");
    u8g2.drawUTF8(0, 58, buf);
  } else {
    int remaining = 30 - elapsed / 1000;
    if (remaining <= 0) {
      cycle_state = CYCLE_NORMAL;
      cycle_start = millis();
      cycle_serial_printed = 0;
      remaining = 0;
    }

    if (!cycle_serial_printed) {
      cycle_serial_printed = 1;
      Serial.println();
      Serial.println("====================================");
      Serial.println("  可以连接热点：PCT-100-003");
      Serial.println("  密码：12345678");
      Serial.println("  通过192.168.4.1 访问控制器");
      Serial.println("====================================");
    }
    draw_ap_info(remaining);
  }

  u8g2.sendBuffer();
}
