#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "logger.h"
#include "led.h"
#include "iot_button.h"
#include "button_interface.h"
#include "button_gpio.h"

static const char *TAG = "Button";

#define BUTTON_GPIO GPIO_NUM_14
#define BUTTON_ACTIVE_LEVEL 1  // Active HIGH (typical for pull-down buttons)

static void (*press_callback)() = NULL;
static void (*release_callback)() = NULL;
static button_handle_t s_btn_handle = NULL;

// Correct signatures for v4.x
static void press_handler_internal(void *arg, void *usr_data) {
    led(true);
    if (press_callback) {
        press_callback();
    }
}

static void release_handler_internal(void *arg, void *usr_data) {
    led(false);
    if (release_callback) {
        release_callback();
    }
}

void init_button(void (*press_cb)(), void (*release_cb)()) {
    press_callback = press_cb;
    release_callback = release_cb;

    button_config_t btn_cfg = {0}; // General button config
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BUTTON_GPIO,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .enable_power_save = false, // You can set true if you want power saving
    };

    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &s_btn_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button device");
        return;
    }

    if (iot_button_register_cb(s_btn_handle, BUTTON_PRESS_DOWN, NULL, press_handler_internal, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register press callback");
    }

    if (iot_button_register_cb(s_btn_handle, BUTTON_PRESS_UP, NULL, release_handler_internal, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register release callback");
    }

    log_info(TAG, "Button initialized on GPIO %d", BUTTON_GPIO);
}
