#ifndef __MQTT_H
#define __MQTT_H

#include "Arduino.h"

#define MQTT_HOST "47.98.170.180"
#define MQTT_PORT 8081
#define MQTT_USER "dzdx_emqx"
#define MQTT_PASS "Jp4!sQ7$"

void mqtt_init(void);
void mqtt_update(void);
void mqtt_publish_status(void);

#endif
