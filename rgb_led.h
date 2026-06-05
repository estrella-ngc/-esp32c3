#ifndef __RGB_LED_H
#define __RGB_LED_H

#include "Arduino.h"

#define RGB_LED_PIN  0
#define RGB_LED_NUM  1

void rgb_led_init(void);
void rgb_led_boot_flash(void);
void rgb_led_poweron_flash(void);
void rgb_led_set(uint8_t r, uint8_t g, uint8_t b);
void rgb_led_off(void);
void rgb_led_task(void);

#endif
