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

void oled_init(void)
{
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(800000);
  u8g2.begin();
  u8g2.setContrast(0x60);
  u8g2.setPowerSave(0);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
}

static void draw_status(void)
{
  char buf[32];
  if (wifi_is_connected()) {
    snprintf(buf, sizeof(buf), "WiFi:%s", wifi_get_ssid());
  } else {
    snprintf(buf, sizeof(buf), "WiFi未连接");
  }
  u8g2.drawUTF8(0, 10, buf);

  snprintf(buf, sizeof(buf), "总闸:%s  模式:%s",
    system_enabled ? "ON" : "OFF",
    auto_mode ? "自动" : "手动");
  u8g2.drawUTF8(0, 22, buf);

  uint16_t adc = get_adc_value();
  unsigned int brightness = (4095UL - adc) * 1000 / 4095;
  unsigned int th = (4095UL - light_threshold) * 1000 / 4095;
  snprintf(buf, sizeof(buf), "光照:%d/%d", brightness, th);
  u8g2.drawUTF8(0, 34, buf);

  snprintf(buf, sizeof(buf), "温度:%.1f/%.0f", get_temperature(), temp_threshold);
  u8g2.drawUTF8(0, 46, buf);

  snprintf(buf, sizeof(buf), "灯光:%s  风扇:%s",
    digitalRead(LED_PIN) ? "ON" : "OFF",
    digitalRead(RELAY_PIN) ? "ON" : "OFF");
  u8g2.drawUTF8(0, 58, buf);
}

void oled_update(void)
{
  static unsigned long last_update = 0;
  if (millis() - last_update < 200) return;
  last_update = millis();

  u8g2.clearBuffer();
  draw_status();
  u8g2.sendBuffer();
}
