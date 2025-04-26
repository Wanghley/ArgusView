#ifndef LED_H
#define LED_H

#include <stdbool.h>

// Initialize the LED and flash light GPIOs
void init_led_and_flash_light();

// Control the flash light (ON/OFF)
void flash_light(bool state);

// Control the LED (ON/OFF)
void led(bool state);

// Blink the flash light 3 times
void blink_flash_light();

// Blink the LED 3 times
void blink_led();

#endif // LED_H
