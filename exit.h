#ifndef __EXIT_H
#define __EXIT_H

#include "Arduino.h"

extern uint8_t system_enabled;
extern uint8_t auto_mode;
extern uint8_t function_mode;
extern const char* device_id;

void exit_init(void);

#endif