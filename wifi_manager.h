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

#endif
