#include "led.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FLASH_LIGHT_GPIO GPIO_NUM_4 // Replace with the actual GPIO for the flash light
#define LED_GPIO GPIO_NUM_2

void init_led_and_flash_light() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLASH_LIGHT_GPIO) | (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Set initial states to OFF
    gpio_set_level(FLASH_LIGHT_GPIO, 0);
    gpio_set_level(LED_GPIO, 0);
}

void flash_light(bool state) {
    gpio_set_level(FLASH_LIGHT_GPIO, state ? 1 : 0);
}

void led(bool state) {
    gpio_set_level(LED_GPIO, state ? 1 : 0);
}

void blink_flash_light() {
    for (int i = 0; i < 3; i++) {
        flash_light(true);
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms ON
        flash_light(false);
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms OFF
    }
}

void blink_led() {
    for (int i = 0; i < 3; i++) {
        led(true);
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms ON
        led(false);
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms OFF
    }
}
