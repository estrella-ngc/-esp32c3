#ifndef __MQTT_H
#define __MQTT_H

#include "Arduino.h"

#define MQTT_DEFAULT_HOST  "47.98.170.180"
#define MQTT_DEFAULT_PORT  8081
#define MQTT_DEFAULT_USER  "dzdx_emqx"
#define MQTT_DEFAULT_PASS  "Jp4!sQ7$"

extern char mqtt_host[64];
extern uint16_t mqtt_port;
extern char mqtt_user[32];
extern char mqtt_pass[32];

void mqtt_init(void);
void mqtt_update(void);
uint8_t mqtt_client_connected(void);
void mqtt_publish_status(void);
void mqtt_request_publish(void);
void mqtt_save_config(void);
void mqtt_disconnect(void);
void mqtt_forget_config(void);
void mqtt_print_config(void);

#endif
