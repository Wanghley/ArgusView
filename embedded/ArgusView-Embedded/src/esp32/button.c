#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "logger.h"

static const char *TAG = "Button";

#define BUTTON_GPIO GPIO_NUM_14 // Define the GPIO pin for the button

static void IRAM_ATTR button_isr_handler(void *arg) {
    static bool button_pressed = false;
    void (**callbacks)() = (void (**)())arg;

    if (gpio_get_level(BUTTON_GPIO) == 0) { // Button pressed
        if (!button_pressed) {
            button_pressed = true;
            callbacks[0](); // Call press handler
        }
    } else { // Button released
        if (button_pressed) {
            button_pressed = false;
            callbacks[1](); // Call release handler
        }
    }
}

void init_button(void (*press_callback)(), void (*release_callback)()) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    void (*callbacks[2])() = {press_callback, release_callback}; // Remove static keyword
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void *)callbacks);

    log_info(TAG, "Button initialized on GPIO %d", BUTTON_GPIO);
}
