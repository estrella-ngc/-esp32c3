#include "wifi_manager.h"
#include <WiFi.h>
#include <Preferences.h>

#define WIFI_SSID "estrella"
#define WIFI_PASS "1234qwer"

static char saved_ssid[32] = "";
static char saved_pass[64] = "";
static char display_ssid[32] = "";
static char display_ip[16] = "0.0.0.0";
static uint8_t is_connected = 0;
static unsigned long last_reconnect = 0;
static uint8_t connecting = 0;
static uint8_t ap_used = 0;

static void load_creds(void)
{
  Preferences prefs;
  prefs.begin("wifi", true);
  String s = prefs.getString("ssid", "");
  String p = prefs.getString("pass", "");
  prefs.end();

  if (s.length() > 0) {
    strncpy(saved_ssid, s.c_str(), sizeof(saved_ssid) - 1);
    strncpy(saved_pass, p.c_str(), sizeof(saved_pass) - 1);
    Serial.print("[WiFi] 从Flash读取保存的凭据: ");
    Serial.println(saved_ssid);
  } else {
    strncpy(saved_ssid, WIFI_SSID, sizeof(saved_ssid) - 1);
    strncpy(saved_pass, WIFI_PASS, sizeof(saved_pass) - 1);
    Serial.println("[WiFi] 使用默认凭据");
  }
}

static void save_creds(const char* ssid, const char* pass)
{
  Preferences prefs;
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  strncpy(saved_ssid, ssid, sizeof(saved_ssid) - 1);
  strncpy(saved_pass, pass, sizeof(saved_pass) - 1);
  Serial.println("[WiFi] 凭据已保存到Flash");
}

static void do_connect(const char* ssid, const char* pass)
{
  connecting = 1;
  is_connected = 0;
  strncpy(display_ssid, ssid, sizeof(display_ssid) - 1);
  Serial.printf("[WiFi] 正在连接 %s ...\r\n", ssid);

  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 80) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    is_connected = 1;
    connecting = 0;
    strncpy(display_ip, WiFi.localIP().toString().c_str(), sizeof(display_ip) - 1);
    Serial.printf("[WiFi] 连接成功! IP: %s\r\n", display_ip);
    save_creds(ssid, pass);
  } else {
    is_connected = 0;
    connecting = 0;
    Serial.println("[WiFi] 连接失败");
  }
}

void wifi_init(void)
{
  Serial.println("[WiFi] 初始化...");
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);

  load_creds();
  do_connect(saved_ssid, saved_pass);

  if (!is_connected) {
    wifi_start_ap();
  }
}

void wifi_update(void)
{
  if (WiFi.getMode() == WIFI_MODE_AP) return;

  if (WiFi.status() == WL_CONNECTED) {
    if (!is_connected) {
      is_connected = 1;
      strncpy(display_ip, WiFi.localIP().toString().c_str(), sizeof(display_ip) - 1);
      Serial.printf("[WiFi] 已重新连接. IP: %s\r\n", display_ip);
    }
    return;
  }

  if (is_connected) {
    is_connected = 0;
    Serial.println("[WiFi] 连接断开，尝试重连...");
  }

  if (millis() - last_reconnect < 5000) return;
  last_reconnect = millis();

  if (saved_ssid[0] == 0) {
    Serial.println("[WiFi] 无保存的凭据，切换到 AP 模式");
    wifi_start_ap();
    return;
  }

  Serial.printf("[WiFi] 重连 %s ...\r\n", saved_ssid);
  WiFi.begin(saved_ssid, saved_pass);
}

uint8_t wifi_is_connected(void)
{
  return is_connected;
}

const char* wifi_get_ip(void)
{
  return display_ip;
}

const char* wifi_get_ssid(void)
{
  return display_ssid;
}

void wifi_scan(void)
{
  Serial.println("[WiFi] 开始扫描...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("[WiFi] 未发现网络");
  } else {
    Serial.printf("[WiFi] 发现 %d 个网络:\r\n", n);
    for (int i = 0; i < n; i++) {
      Serial.printf("  %2d: %-32s RSSI:%4d dBm  CH:%2d  %s\r\n",
        i, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
        WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "开放" : "加密");
    }
  }
  WiFi.scanDelete();
}

void wifi_connect(const char* ssid, const char* pass)
{
  do_connect(ssid, pass);
}

void wifi_print_info(void)
{
  if (!is_connected) {
    Serial.println("WiFi 未连接");
    return;
  }
  if (WiFi.getMode() == WIFI_MODE_AP) {
    Serial.printf("模式:        热点\r\n");
    Serial.printf("SSID:        PCT-100-003\r\n");
    Serial.printf("密码:        12345678\r\n");
    Serial.printf("IPv4:        %s\r\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("模式:        客户端\r\n");
    Serial.printf("SSID:        %s\r\n", display_ssid);
    Serial.printf("IPv4:        %s\r\n", WiFi.localIP().toString().c_str());
    Serial.printf("子网掩码:    %s\r\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("默认网关:    %s\r\n", WiFi.gatewayIP().toString().c_str());
  }
}

uint8_t wifi_ap_has_client(void)
{
  return WiFi.softAPgetStationNum() > 0;
}

uint8_t wifi_ap_is_used(void)
{
  return ap_used;
}

void wifi_ap_mark_used(void)
{
  ap_used = 1;
}

void wifi_start_ap(void)
{
  ap_used = 0;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP("PCT-100-003", "12345678");
  strncpy(display_ip, WiFi.softAPIP().toString().c_str(), sizeof(display_ip) - 1);
  strncpy(display_ssid, "PCT-100-003", sizeof(display_ssid) - 1);
  is_connected = 1;
  connecting = 0;

  Serial.println();
  Serial.println("====================================");
  Serial.println("  可以连接热点：PCT-100-003");
  Serial.println("  密码：12345678");
  Serial.print("  通过");
  Serial.print(WiFi.softAPIP().toString());
  Serial.println(" 访问控制器");
  Serial.println("====================================");
}

uint8_t wifi_is_ap_mode(void)
{
  return WiFi.getMode() == WIFI_MODE_AP ? 1 : 0;
}

void wifi_stop_ap(void)
{
  if (WiFi.getMode() != WIFI_MODE_AP) {
    Serial.println("[WiFi] 当前不是热点模式");
    return;
  }
  Serial.println("[WiFi] 关闭热点模式");
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_STA);
  is_connected = 0;
  connecting = 0;
  strncpy(display_ip, "0.0.0.0", sizeof(display_ip) - 1);
  strncpy(display_ssid, "", sizeof(display_ssid) - 1);

  if (saved_ssid[0]) {
    Serial.printf("[WiFi] 尝试连接 %s ...\r\n", saved_ssid);
    do_connect(saved_ssid, saved_pass);
  }

  if (!is_connected) {
    Serial.println("[WiFi] 未能连接任何网络");
  }
}

void wifi_reconnect_default(void)
{
  strncpy(saved_ssid, WIFI_SSID, sizeof(saved_ssid) - 1);
  strncpy(saved_pass, WIFI_PASS, sizeof(saved_pass) - 1);
  Serial.printf("[WiFi] 重新连接默认网络 %s ...\r\n", WIFI_SSID);

  if (WiFi.getMode() == WIFI_MODE_AP) {
    WiFi.softAPdisconnect(true);
  }

  WiFi.mode(WIFI_MODE_STA);
  do_connect(WIFI_SSID, WIFI_PASS);

  if (!is_connected) {
    Serial.println("[WiFi] 连接失败，切换到 AP 模式");
    wifi_start_ap();
  } else {
    save_creds(WIFI_SSID, WIFI_PASS);
  }
}

void wifi_forget(void)
{
  Preferences prefs;
  prefs.begin("wifi", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();

  saved_ssid[0] = 0;
  saved_pass[0] = 0;
  is_connected = 0;
  connecting = 0;

  WiFi.disconnect(true);
  strncpy(display_ip, "0.0.0.0", sizeof(display_ip) - 1);
  strncpy(display_ssid, "", sizeof(display_ssid) - 1);

  Serial.println("[WiFi] 凭据已清除，下次重启不再自动连接");
}
