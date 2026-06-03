#ifndef __EXIT_H
#define __EXIT_H

#include "Arduino.h"

extern uint8_t system_enabled;
extern uint8_t auto_mode;
extern uint8_t function_mode;
extern char device_id[32];

void exit_init(void);
void exit_load_id(void);
void exit_save_id(void);

#endif