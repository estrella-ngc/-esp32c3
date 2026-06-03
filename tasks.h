#ifndef __TASKS_H
#define __TASKS_H

#include "Arduino.h"

#define TASK_STACK_SIZE 8192

extern portMUX_TYPE shared_mux;

void tasks_init(void);

#endif
