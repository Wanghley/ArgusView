#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "logger.h"
#include "led.h"

static const char *TAG = "Button";

#define BUTTON_GPIO GPIO_NUM_14 // GPIO pin for the button

typedef enum {
    BUTTON_EVENT_PRESS,
    BUTTON_EVENT_RELEASE
} button_event_t;

static QueueHandle_t button_event_queue = NULL;
static void (*press_callback)() = NULL;
static void (*release_callback)() = NULL;

static bool button_pressed = false;
static const int64_t DEBOUNCE_TIME_US = 50000; // 50 ms
static int64_t last_interrupt_time = 0;

static void IRAM_ATTR button_isr_handler(void *arg) {
    int64_t now = esp_timer_get_time();
    if ((now - last_interrupt_time) < DEBOUNCE_TIME_US) {
        return;
    }
    last_interrupt_time = now;

    if (gpio_get_level(BUTTON_GPIO) == 1) { // Button pressed
        led(true);
        if (!button_pressed) {
            button_pressed = true;
            button_event_t event = BUTTON_EVENT_PRESS;
            xQueueSendFromISR(button_event_queue, &event, NULL);
        }
    } else { // Button released
        led(false);
        if (button_pressed) {
            button_pressed = false;
            button_event_t event = BUTTON_EVENT_RELEASE;
            xQueueSendFromISR(button_event_queue, &event, NULL);
        }
    }
}

static void button_task(void *arg) {
    button_event_t event;
    while (1) {
        if (xQueueReceive(button_event_queue, &event, portMAX_DELAY)) {
            if (event == BUTTON_EVENT_PRESS && press_callback != NULL) {
                press_callback();
            } else if (event == BUTTON_EVENT_RELEASE && release_callback != NULL) {
                release_callback();
            }
        }
    }
}

void init_button(void (*press_cb)(), void (*release_cb)()) {
    press_callback = press_cb;
    release_callback = release_cb;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    button_event_queue = xQueueCreate(10, sizeof(button_event_t));
    if (button_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button event queue");
        return;
    }

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);

    log_info(TAG, "Button initialized on GPIO %d", BUTTON_GPIO);
}
