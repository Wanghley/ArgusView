#include "camera_flash.h"
#include "driver/gpio.h" // ESP-IDF GPIO driver

#define FLASH_GPIO_PIN 4 // Define the GPIO pin for the flash (adjust as needed)

int camera_flash_on() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLASH_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        return -1; // Return error if GPIO configuration fails
    }

    if (gpio_set_level(FLASH_GPIO_PIN, 1) != ESP_OK) {
        return -1; // Return error if setting GPIO level fails
    }

    return 0; // Success
}
