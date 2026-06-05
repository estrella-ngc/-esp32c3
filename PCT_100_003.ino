#include "exit.h"
#include "key.h"
#include <WiFi.h>
#include "led.h"
#include "relay.h"
#include "adc.h"
#include "tmp.h"
#include "oled.h"
#include "rgb_led.h"
#include "mqtt.h"
#include "wifi_manager.h"
#include "web_ui.h"
#include "tasks.h"

#define LONG_PRESS_THRESHOLD 2000

static char serial_buf[128];
static uint8_t serial_len = 0;

static void handle_serial(void)
{
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serial_len == 0) continue;
      serial_buf[serial_len] = 0;
      serial_len = 0;

      if (strcmp(serial_buf, "scan") == 0) {
        wifi_scan();
      } else if (strncmp(serial_buf, "connect ", 8) == 0) {
        char ssid[32], pass[64];
        if (sscanf(serial_buf + 8, "%31s %63s", ssid, pass) >= 1) {
          wifi_connect(ssid, pass);
        } else {
          Serial.println("格式: connect <ssid> <password>");
        }
      } else if (strcmp(serial_buf, "forget") == 0) {
        wifi_forget();
      } else if (strcmp(serial_buf, "ip") == 0) {
        wifi_print_info();
      } else if (strcmp(serial_buf, "help") == 0) {
        Serial.println("WiFi: scan / connect <ssid> <pass> / ip / forget");
        Serial.println("MQTT: mqtt_info / connectmqtt / forgetmqtt / mqtt_set <host> <port> <user> <pass> / mqtt_id <id>");
        Serial.println("      输入 mqtt_set 后自动重连, 配置保存至Flash");
      } else if (strncmp(serial_buf, "mqtt_set ", 9) == 0) {
        char host[64], user[32], pass[32];
        int port;
        if (sscanf(serial_buf + 9, "%63s %d %31s %31s", host, &port, user, pass) >= 4) {
          strncpy(mqtt_host, host, sizeof(mqtt_host) - 1);
          mqtt_port = port;
          strncpy(mqtt_user, user, sizeof(mqtt_user) - 1);
          strncpy(mqtt_pass, pass, sizeof(mqtt_pass) - 1);
          mqtt_save_config();
          Serial.println("[MQTT] 配置已更新，重新连接...");
          mqtt_init();
        } else {
          Serial.println("格式: mqtt_set <host> <port> <user> <pass>");
        }
      } else if (strncmp(serial_buf, "mqtt_id ", 8) == 0) {
        char id[32];
        if (sscanf(serial_buf + 8, "%31s", id) >= 1) {
          strncpy(device_id, id, sizeof(device_id) - 1);
          exit_save_id();
          Serial.println("[MQTT] DeviceID已更新，重新连接...");
          mqtt_init();
        } else {
          Serial.println("格式: mqtt_id <device_id>");
        }
      } else if (strcmp(serial_buf, "connectmqtt") == 0) {
        Serial.println("[MQTT] 手动连接...");
        mqtt_init();
      } else if (strcmp(serial_buf, "disconnectmqtt") == 0) {
        mqtt_disconnect();
      } else if (strcmp(serial_buf, "forgetmqtt") == 0) {
        mqtt_forget_config();
        Serial.println("[MQTT] 已断开，下次开机使用默认配置");
      } else if (strcmp(serial_buf, "mqtt_info") == 0) {
        mqtt_print_config();
      } else if (serial_buf[0]) {
        Serial.print("未知命令: ");
        Serial.println(serial_buf);
        Serial.println("输入 help 查看命令列表");
      }
    } else if (serial_len < sizeof(serial_buf) - 1) {
      serial_buf[serial_len++] = c;
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println();
  Serial.println("=== 系统启动 ===");

  rgb_led_init();
  rgb_led_boot_flash();

  exit_init();
  adc_init();
  tmp_init();
  oled_init();
  wifi_init();
  web_init();
  mqtt_init();

  if (wifi_is_connected()) {
    Serial.print("网页控制: http://");
    Serial.println(wifi_get_ip());
  }
  tasks_init();
  Serial.println("初始化完成");
  Serial.println("输入 help 查看命令");
}

void loop() {
  handle_key1();
  handle_key2();
  handle_serial();
  read_light_sensor();
  read_temperature();
  update_outputs();
  web_update();
  oled_update();

  if (system_enabled && auto_mode) {
    static unsigned long t = 0;
    if (millis() - t >= 1000) {
      t = millis();
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
  }

  delay(10);
}

void handle_key1() {
  uint8_t current = KEY1;

  if (current == 1) {
    if (!system_enabled) {
      system_enabled = 1;
      auto_mode = 1;
      function_mode = 0;
      mqtt_request_publish();
      rgb_led_poweron_flash();
      if (!wifi_is_connected()) {
        Serial.println("[总开关] WiFi未连接，尝试重连...");
        WiFi.reconnect();
      }

      Serial.println("[总开关] 已开启");
      Serial.println(">>> 模式：自动（光感自动控制灯）");
      Serial.println("    长按辅助开关 2 秒进入手动模式");
    }
  } else {
    if (system_enabled) {
      system_enabled = 0;
      mqtt_request_publish();
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
        mqtt_request_publish();
        Serial.print("[辅助开关] 按下 -> 状态 ");
        Serial.println(function_mode);
      }
    }
    key2_last = current;
  }

  if (current == 1 && !long_press_triggered && system_enabled) {
    if (millis() - key2_press_time >= LONG_PRESS_THRESHOLD) {
      long_press_triggered = 1;
      auto_mode = !auto_mode;
      mqtt_request_publish();
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
