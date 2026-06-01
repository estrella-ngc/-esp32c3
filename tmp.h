#ifndef __TMP_H
#define __TMP_H

#include "Arduino.h"

#define ONE_WIRE_BUS 10

void tmp_init(void);
void read_temperature(void);
float get_temperature(void);
uint8_t get_fan_state(void);

#endif
