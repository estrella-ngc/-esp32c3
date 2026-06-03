#include "web_ui.h"
#include "exit.h"
#include "adc.h"
#include "tmp.h"
#include "led.h"
#include "relay.h"
#include "wifi_manager.h"
#include <WebServer.h>
#include <WiFi.h>

static WebServer server(80);

static void handle_root(void)
{
  server.send(200, "text/plain", "PCT-100-003 OK");
}

static void handle_status(void)
{
  String json = "{";
  json += "\"system\":" + String(system_enabled) + ",";
  json += "\"auto\":" + String(auto_mode) + ",";
  json += "\"fn\":" + String(function_mode) + ",";
  json += "\"light\":" + String(get_adc_value()) + ",";
  json += "\"temp\":" + String(get_temperature()) + ",";
  json += "\"led\":" + String(digitalRead(LED_PIN)) + ",";
  json += "\"fan\":" + String(digitalRead(RELAY_PIN)) + "";
  json += "}";
  server.send(200, "application/json", json);
}

static void handle_system(void)
{
  system_enabled = !system_enabled;
  if (!system_enabled) { LED(LOW); relay_control(LOW); }
  handle_status();
}

static void handle_mode(void)
{
  auto_mode = !auto_mode;
  handle_status();
}

static void handle_function(void)
{
  if (server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    if (m >= 0 && m <= 3) function_mode = m;
  }
  handle_status();
}

static void handle_led(void)
{
  if (server.hasArg("state")) {
    LED(server.arg("state").toInt() ? HIGH : LOW);
  }
  handle_status();
}

static void handle_fan(void)
{
  if (server.hasArg("state")) {
    relay_control(server.arg("state").toInt() ? HIGH : LOW);
  }
  handle_status();
}

static void handle_network(void)
{
  String json = "{";
  json += "\"connected\":" + String(wifi_is_connected() ? "true" : "false") + ",";
  json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mask\":\"" + WiFi.subnetMask().toString() + "\",";
  json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

static void handle_scan(void)
{
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"open\":" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

static void handle_wifi_connect(void)
{
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    wifi_connect(server.arg("ssid").c_str(), server.arg("pass").c_str());
    String json = "{\"ok\":" + String(wifi_is_connected() ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"ok\":false}");
  }
}

static void handle_wifi_forget(void)
{
  wifi_forget();
  server.send(200, "application/json", "{\"ok\":true}");
}

void web_init(void)
{
  server.on("/", handle_root);
  server.on("/api/status", handle_status);
  server.on("/api/system", HTTP_POST, handle_system);
  server.on("/api/mode", HTTP_POST, handle_mode);
  server.on("/api/function", HTTP_POST, handle_function);
  server.on("/api/led", HTTP_POST, handle_led);
  server.on("/api/fan", HTTP_POST, handle_fan);
  server.on("/api/network", handle_network);
  server.on("/api/scan", handle_scan);
  server.on("/api/wifi_connect", HTTP_POST, handle_wifi_connect);
  server.on("/api/wifi_forget", HTTP_POST, handle_wifi_forget);
  server.begin();
  Serial.println("[Web] HTTP server started on port 80");
}

void web_update(void)
{
  server.handleClient();
}
