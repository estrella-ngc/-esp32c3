#include "exit.h"
#include "key.h"
#include "led.h"
#include "relay.h"
#include "version.h"
#include <Preferences.h>

uint8_t system_enabled = 0;
uint8_t auto_mode = 1;
uint8_t function_mode = 0;
char device_id[32] = DEVICE_SERIAL;

void exit_load_id(void)
{
  Preferences prefs;
  prefs.begin("device", true);
  String s = prefs.getString("id", "");
  prefs.end();

  if (s.length() > 0 && s.length() < sizeof(device_id)) {
    strncpy(device_id, s.c_str(), sizeof(device_id) - 1);
    Serial.print("[DEVICE] 从Flash读取ID: ");
    Serial.println(device_id);
  } else {
    Serial.print("[DEVICE] 使用默认ID: ");
    Serial.println(device_id);
  }
}

void exit_save_id(void)
{
  Preferences prefs;
  prefs.begin("device", false);
  prefs.putString("id", device_id);
  prefs.end();
  Serial.print("[DEVICE] ID已保存: ");
  Serial.println(device_id);
}

void exit_init(void)
{
  key_init();
  led_init();
  relay_init();
  exit_load_id();
}