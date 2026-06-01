#include "exit.h"
#include "key.h"
#include "led.h"
#include "relay.h"

uint8_t system_enabled = 0;
uint8_t auto_mode = 1;
uint8_t function_mode = 0;


void exit_init(void)
{
  key_init();
  led_init();
  relay_init();
}