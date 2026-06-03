#include "mqtt.h"
#include "exit.h"
#include "adc.h"
#include "tmp.h"
#include "led.h"
#include "relay.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient wifi_client;
static PubSubClient client(wifi_client);
static char status_topic[64];
static char cmd_topic[64];
static unsigned long last_publish = 0;
static uint8_t initialized = 0;

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
    if (relay == 3) {
      LED(value ? HIGH : LOW);
    } else if (relay == 4) {
      relay_control(value ? HIGH : LOW);
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
      Serial.printf("[MQTT] 设置温度阈值 = %.1f\r\n", temp_threshold);
    }
    if (doc.containsKey("light")) {
      light_threshold = doc["light"].as<uint16_t>();
      Serial.printf("[MQTT] 设置光照阈值 = %d\r\n", light_threshold);
    }
  } else if (strcmp(cmd, "reboot") == 0) {
    Serial.println("[MQTT] 收到重启命令");
    delay(100);
    ESP.restart();
  } else {
    Serial.print("[MQTT] 未知命令: ");
    Serial.println(cmd);
  }
}

static void reconnect(void)
{
  char client_id[32];
  snprintf(client_id, sizeof(client_id), "PCT_%s", device_id);

  Serial.printf("[MQTT] 尝试连接... client_id=%s\r\n", client_id);
  if (client.connect(client_id, MQTT_USER, MQTT_PASS)) {
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
  snprintf(status_topic, sizeof(status_topic), "chemctrl/%s/status", device_id);
  snprintf(cmd_topic, sizeof(cmd_topic), "chemctrl/%s/command", device_id);

  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
  initialized = 1;

  Serial.println("[MQTT] 初始化完成");
  Serial.print("  服务器: ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  Serial.print("  发布: ");
  Serial.println(status_topic);
  Serial.print("  订阅: ");
  Serial.println(cmd_topic);
}

void mqtt_update(void)
{
  if (!initialized) return;
  if (!wifi_is_connected()) return;

  if (!client.connected()) {
    static unsigned long last_reconnect = 0;
    if (millis() - last_reconnect >= 5000) {
      last_reconnect = millis();
      reconnect();
    }
    return;
  }

  client.loop();

  if (millis() - last_publish >= 5000) {
    last_publish = millis();
    mqtt_publish_status();
  }
}

void mqtt_publish_status(void)
{
  if (!initialized || !client.connected()) return;

  JsonDocument doc;
  doc["temperature"] = get_temperature();
  doc["light"] = get_adc_value();
  doc["mode"] = auto_mode ? "auto" : "manual";
  doc["key1_lock"] = (bool)system_enabled;
  doc["relay3"] = (bool)digitalRead(LED_PIN);
  doc["relay4"] = (bool)digitalRead(RELAY_PIN);
  doc["temp_threshold"] = temp_threshold;
  doc["light_threshold"] = light_threshold;

  char json[256];
  size_t n = serializeJson(doc, json, sizeof(json));
  if (client.publish(status_topic, json, n)) {
    Serial.print("[MQTT] 已上报: ");
    Serial.println(json);
  }
}
