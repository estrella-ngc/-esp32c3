#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

#include "Arduino.h"

void wifi_init(void);
void wifi_update(void);
uint8_t wifi_is_connected(void);
const char* wifi_get_ip(void);
const char* wifi_get_ssid(void);
void wifi_scan(void);
void wifi_connect(const char* ssid, const char* pass);
void wifi_forget(void);
void wifi_print_info(void);
void wifi_start_ap(void);
void wifi_stop_ap(void);
void wifi_reconnect_default(void);
uint8_t wifi_ap_has_client(void);
uint8_t wifi_ap_is_used(void);
void wifi_ap_mark_used(void);
uint8_t wifi_is_ap_mode(void);

#endif
