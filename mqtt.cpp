#include "mqtt.h"
#include "exit.h"
#include "adc.h"
#include "tmp.h"
#include "led.h"
#include "relay.h"
#include "tasks.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

char mqtt_host[64] = MQTT_DEFAULT_HOST;
uint16_t mqtt_port = MQTT_DEFAULT_PORT;
char mqtt_user[32] = MQTT_DEFAULT_USER;
char mqtt_pass[32] = MQTT_DEFAULT_PASS;

static WiFiClient wifi_client;
static PubSubClient client(wifi_client);
static char status_topic[64];
static char cmd_topic[64];
static unsigned long last_publish = 0;
static volatile uint8_t publish_requested = 0;
static uint8_t initialized = 0;

static void rebuild_topics(void)
{
  snprintf(status_topic, sizeof(status_topic), "chemctrl/%s/status", device_id);
  snprintf(cmd_topic, sizeof(cmd_topic), "chemctrl/%s/command", device_id);
}

static void callback(char* topic, byte* payload, unsigned int length)
{
  char buf[256];
  unsigned int len = min(length, sizeof(buf) - 1);
  memcpy(buf, payload, len);
  buf[len] = 0;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (err) {
    Serial.print("[MQTT] JSON解析失败: ");
    Serial.println(err.c_str());
    return;
  }

  const char* cmd = doc["cmd"];

  if (strcmp(cmd, "set_relay") == 0) {
    if (!system_enabled) {
      Serial.println("[MQTT] 总闸关闭，忽略继电器控制");
      return;
    }
    int relay = doc["relay"];
    bool value = doc["value"];
    Serial.printf("[MQTT] 设置 relay%d = %s\r\n", relay, value ? "ON" : "OFF");
    auto_mode = 0;
    if (relay == 3) {
      LED(value ? HIGH : LOW);
      function_mode = (value ? 1 : 0) | (digitalRead(RELAY_PIN) ? 2 : 0);
    } else if (relay == 4) {
      relay_control(value ? HIGH : LOW);
      function_mode = (digitalRead(LED_PIN) ? 1 : 0) | (value ? 2 : 0);
    }
  } else if (strcmp(cmd, "set_mode") == 0) {
    const char* mode = doc["mode"];
    auto_mode = (strcmp(mode, "auto") == 0) ? 1 : 0;
    Serial.printf("[MQTT] 设置模式 = %s\r\n", mode);
  } else if (strcmp(cmd, "get_status") == 0) {
    Serial.println("[MQTT] 收到状态查询");
    mqtt_publish_status();
  } else if (strcmp(cmd, "set_threshold") == 0) {
    if (doc.containsKey("temp")) {
      temp_threshold = doc["temp"].as<float>();
      tmp_save_threshold();
      Serial.printf("[MQTT] 设置温度阈值 = %.1f\r\n", temp_threshold);
    }
    if (doc.containsKey("light")) {
      uint16_t val = doc["light"].as<uint16_t>();
      light_threshold = 4095 - (val * 4095UL / 1000);
      adc_save_threshold();
      Serial.printf("[MQTT] 设置光照阈值 = %d (ADC: %d)\r\n", val, light_threshold);
    }
    mqtt_request_publish();
  } else if (strcmp(cmd, "reboot") == 0) {
    Serial.println("[MQTT] 收到重启命令");
    delay(100);
    ESP.restart();
  } else {
    Serial.print("[MQTT] 未知命令: ");
    Serial.println(cmd);
  }
}

static void load_config(void)
{
  Preferences prefs;
  prefs.begin("mqtt", true);

  String h = prefs.getString("host", "");
  if (h.length() > 0) {
    strncpy(mqtt_host, h.c_str(), sizeof(mqtt_host) - 1);
  }
  mqtt_port = prefs.getUShort("port", MQTT_DEFAULT_PORT);

  String u = prefs.getString("user", "");
  if (u.length() > 0) {
    strncpy(mqtt_user, u.c_str(), sizeof(mqtt_user) - 1);
  }

  String p = prefs.getString("pass", "");
  if (p.length() > 0) {
    strncpy(mqtt_pass, p.c_str(), sizeof(mqtt_pass) - 1);
  }

  prefs.end();
}

uint8_t mqtt_client_connected(void)
{
  return client.connected();
}

void mqtt_request_publish(void)
{
  publish_requested = 1;
}

void mqtt_save_config(void)
{
  Preferences prefs;
  prefs.begin("mqtt", false);
  prefs.putString("host", mqtt_host);
  prefs.putUShort("port", mqtt_port);
  prefs.putString("user", mqtt_user);
  prefs.putString("pass", mqtt_pass);
  prefs.end();
  Serial.println("[MQTT] 配置已保存到Flash");
}

void mqtt_disconnect(void)
{
  if (client.connected()) {
    client.disconnect();
  }
  Serial.println("[MQTT] 已断开");
}

void mqtt_forget_config(void)
{
  mqtt_disconnect();
  Preferences prefs;
  prefs.begin("mqtt", false);
  prefs.remove("host");
  prefs.remove("port");
  prefs.remove("user");
  prefs.remove("pass");
  prefs.end();
  strncpy(mqtt_host, MQTT_DEFAULT_HOST, sizeof(mqtt_host) - 1);
  mqtt_port = MQTT_DEFAULT_PORT;
  strncpy(mqtt_user, MQTT_DEFAULT_USER, sizeof(mqtt_user) - 1);
  strncpy(mqtt_pass, MQTT_DEFAULT_PASS, sizeof(mqtt_pass) - 1);
  Serial.println("[MQTT] 配置已清除，恢复默认值");
}

void mqtt_print_config(void)
{
  Serial.println("--- MQTT 配置 ---");
  Serial.print("  服务器: ");
  Serial.print(mqtt_host);
  Serial.print(":");
  Serial.println(mqtt_port);
  Serial.print("  用户名: ");
  Serial.println(mqtt_user);
  Serial.print("  密码: ");
  Serial.println(mqtt_pass);
  Serial.print("  DeviceID: ");
  Serial.println(device_id);
  Serial.print("  状态Topic: ");
  Serial.println(status_topic);
  Serial.print("  命令Topic: ");
  Serial.println(cmd_topic);
  Serial.print("  WiFi: ");
  Serial.println(wifi_is_connected() ? "已连接" : "未连接");
  Serial.print("  MQTT: ");
  Serial.println(client.connected() ? "已连接" : "未连接");
  Serial.println("------------------");
}

static void mqtt_connect(void)
{
  char client_id[32];
  snprintf(client_id, sizeof(client_id), "PCT_%s", device_id);

  Serial.printf("[MQTT] 连接 %s:%d ... client_id=%s\r\n", mqtt_host, mqtt_port, client_id);
  if (client.connect(client_id, mqtt_user, mqtt_pass)) {
    Serial.println("[MQTT] 连接成功");
    client.subscribe(cmd_topic);
    Serial.print("[MQTT] 已订阅: ");
    Serial.println(cmd_topic);
    mqtt_publish_status();
  } else {
    Serial.print("[MQTT] 连接失败, rc=");
    Serial.println(client.state());
  }
}

void mqtt_init(void)
{
  if (client.connected()) {
    client.disconnect();
  }

  load_config();
  rebuild_topics();

  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(callback);
  initialized = 1;

  Serial.println("[MQTT] 初始化完成");
  mqtt_print_config();
}

void mqtt_update(void)
{
  if (!initialized) return;
  if (!wifi_is_connected()) return;

  if (!client.connected()) {
    static unsigned long last_reconnect = 0;
    if (millis() - last_reconnect >= 5000) {
      last_reconnect = millis();
      mqtt_connect();
    }
    return;
  }

  client.loop();

  static uint8_t prev_enabled = 0;
  static uint8_t prev_auto = 1;
  static uint8_t prev_fn = 0;
  static uint8_t prev_led = 0;
  static uint8_t prev_relay = 0;

  uint8_t cur_enabled, cur_auto, cur_fn;
  portENTER_CRITICAL(&shared_mux);
  cur_enabled = system_enabled;
  cur_auto = auto_mode;
  cur_fn = function_mode;
  portEXIT_CRITICAL(&shared_mux);

  uint8_t cur_led = digitalRead(LED_PIN);
  uint8_t cur_relay = digitalRead(RELAY_PIN);

  uint8_t changed = (prev_enabled != cur_enabled || prev_auto != cur_auto ||
                     prev_fn != cur_fn || prev_led != cur_led || prev_relay != cur_relay);
  if (changed) {
    prev_enabled = cur_enabled;
    prev_auto = cur_auto;
    prev_fn = cur_fn;
    prev_led = cur_led;
    prev_relay = cur_relay;
  }

  if (publish_requested) {
    publish_requested = 0;
    last_publish = millis();
    mqtt_publish_status();
  } else if (changed) {
    last_publish = millis();
    mqtt_publish_status();
  } else if (millis() - last_publish >= 5000) {
    last_publish = millis();
    mqtt_publish_status();
  }
}

void mqtt_publish_status(void)
{
  if (!initialized || !client.connected()) return;

  uint16_t adc = get_adc_value();
  unsigned int brightness = (4095UL - adc) * 1000 / 4095;
  unsigned int threshold = (4095UL - light_threshold) * 1000 / 4095;

  JsonDocument doc;
  doc["temperature"] = get_temperature();
  doc["light"] = brightness;
  doc["mode"] = auto_mode ? "auto" : "manual";
  doc["key1_lock"] = (bool)system_enabled;
  doc["relay3"] = (bool)digitalRead(LED_PIN);
  doc["relay4"] = (bool)digitalRead(RELAY_PIN);
  doc["temp_threshold"] = temp_threshold;
  doc["light_threshold"] = threshold;

  char json[256];
  size_t n = serializeJson(doc, json, sizeof(json));
  if (client.publish(status_topic, json, n)) {
    Serial.print("[MQTT] 已上报: ");
    Serial.println(json);
  }
}
