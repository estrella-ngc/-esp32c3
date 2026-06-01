#include "exit.h"
#include "key.h"
#include "led.h"
#include "relay.h"
#include "adc.h"
#include "tmp.h"

#define LONG_PRESS_THRESHOLD 2000

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println();
  Serial.println("=== 系统启动 ===");
  Serial.println("初始化完成，等待总开关...");

  exit_init();
  adc_init();
  tmp_init();
}

void loop() {
  read_light_sensor();
  read_temperature();

  if (system_enabled)
  {
    Serial.print("[光感] ADC=");
    Serial.print(get_adc_value());
    Serial.print(" [灯");
    Serial.print(get_light_state() ? "亮" : "灭");
    Serial.print("]  [温度] ");
    Serial.print(get_temperature());
    Serial.print("C [风扇");
    Serial.print(get_fan_state() ? "开" : "关");
    Serial.println("]");
  }

  handle_key1();
  handle_key2();
  update_outputs();
  delay(10);
}

void handle_key1() {
  uint8_t current = KEY1;

  if (current == 1) {
    if (!system_enabled) {
      system_enabled = 1;
      auto_mode = 1;
      function_mode = 0;
      Serial.println("[总开关] 已开启");
      Serial.println(">>> 模式：自动（光感自动控制灯）");
      Serial.println("    长按辅助开关 2 秒进入手动模式");
    }
  } else {
    if (system_enabled) {
      system_enabled = 0;
      Serial.println("[总开关] 已关闭");
    }
  }
}

void handle_key2() {
  static uint8_t key2_last = 0;
  static unsigned long key2_press_time = 0;
  static uint8_t long_press_triggered = 0;

  uint8_t current = KEY2;

  if (current != key2_last) {
    if (current == 1) {
      key2_press_time = millis();
      long_press_triggered = 0;
    } else {
      if (!long_press_triggered && system_enabled && !auto_mode) {
        function_mode = (function_mode + 1) % 4;
        Serial.print("[辅助开关] 短按 -> 状态 ");
        Serial.println(function_mode);
      }
    }
    key2_last = current;
  }

  if (current == 1 && !long_press_triggered && system_enabled) {
    unsigned long press_duration = millis() - key2_press_time;
    if (press_duration >= LONG_PRESS_THRESHOLD) {
      long_press_triggered = 1;
      auto_mode = !auto_mode;
      if (auto_mode) {
        Serial.println("[辅助开关] 长按 2 秒 -> 自动模式（光感控灯）");
      } else {
        Serial.println("[辅助开关] 长按 2 秒 -> 手动模式");
        Serial.println("    短按循环：0=全关  1=灯亮  2=风扇转  3=灯+风扇");
      }
    }
  }
}

void update_outputs() {
  if (!system_enabled) {
    LED(LOW);
    relay_control(LOW);
  } else if (auto_mode) {
    LED(get_light_state());
    relay_control(get_fan_state());
  } else {
    switch (function_mode) {
      case 0: LED(LOW);            relay_control(LOW);   break;
      case 1: LED(HIGH);           relay_control(LOW);   break;
      case 2: LED(LOW);            relay_control(HIGH);  break;
      case 3: LED(HIGH);           relay_control(HIGH);  break;
    }
  }
}