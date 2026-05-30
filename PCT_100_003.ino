#include "exit.h"
#include "key.h"
#include "led.h"
#include "relay.h"

#define LONG_PRESS_THRESHOLD 2000

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println();
  Serial.println("=== 系统启动 ===");
  Serial.println("初始化完成，等待总开关...");

  exit_init();
}

void loop() {
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
      Serial.println(">>> 模式：自动");
      // Serial.println("    长按辅助开关 2 秒进入手动模式");
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
        Serial.print("    状态 ");
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
        Serial.println(">>> 模式：手动");
      } else {
        Serial.println(">>> 模式：手动 状态0");
        // Serial.println("    状态0");
      }
    }
  }
}

void update_outputs() {
  if (system_enabled) {
    if (auto_mode) {
      LED(LOW);
      relay_control(LOW);
    } else {
      switch (function_mode) {
        case 0:
          LED(LOW);
          relay_control(LOW);
          break;
        case 1:
          LED(HIGH);
          relay_control(LOW);
          break;
        case 2:
          LED(LOW);
          relay_control(HIGH);
          break;
        case 3:
          LED(HIGH);
          relay_control(HIGH);
          break;
      }
    }
  } else {
    LED(LOW);
    relay_control(LOW);
  }
}