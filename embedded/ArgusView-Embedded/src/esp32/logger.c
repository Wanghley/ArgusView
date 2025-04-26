#include "logger.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include <esp_wifi_types_generic.h>
#include "esp_wifi.h"

static const char *TAG = "ArgusView";
static const char *backend_log_url = "http://vcm-46428.vm.duke.edu:3000/log"; // Set as a constant
static _Thread_local bool is_logging = false; // Thread-local flag to prevent recursion

static int send_log_to_backend(const char *level, const char *message) {
    if (is_logging) {
        return 0; // Skip if already logging to avoid recursion
    }

    if (xPortInIsrContext()) {
        return 0; // Skip if inside an interrupt (ISR)
    }

    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return 0; // Skip if FreeRTOS scheduler is not running
    }

    if (!level || !message) {
        return 0; // Defensive: avoid NULL pointers
    }

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        return 0; // Not connected to Wi-Fi, skip
    }

    is_logging = true; // Set the flag
    esp_http_client_config_t config = {
        .url = backend_log_url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    char body[256];
    snprintf(body, sizeof(body), "{\"level\":\"%s\",\"message\":\"%s\"}", level, message);

    esp_http_client_set_post_field(client, body, strlen(body));

    int result = 0;
    if (esp_http_client_perform(client) != ESP_OK) {
        result = -1;
    }

    esp_http_client_cleanup(client);
    is_logging = false; // Clear the flag
    return result;
}

static int custom_vprintf(const char *fmt, va_list args) {
    if (is_logging) {
        return vprintf(fmt, args); // Skip sending to backend to prevent recursion
    }

    char log_buffer[256];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    // Extract log level from the format string
    const char *level = "INFO"; // Default to INFO
    if (strstr(fmt, "E") == fmt) {
        level = "ERROR";
    } else if (strstr(fmt, "W") == fmt) {
        level = "WARN";
    } else if (strstr(fmt, "D") == fmt) {
        level = "DEBUG";
    }

    // Send log to backend
    send_log_to_backend(level, log_buffer);

    // Print to console as well
    return vprintf(fmt, args);
}

void log_info(const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char log_buffer[256];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    char tagged_message[256];
    snprintf(tagged_message, sizeof(tagged_message), "[%.60s] %.192s", tag, log_buffer);

    send_log_to_backend("INFO", tagged_message);
    ESP_LOGI(tag, "%s", log_buffer);

    va_end(args);
}

void log_error(const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char log_buffer[256];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    char tagged_message[256];
    snprintf(tagged_message, sizeof(tagged_message), "[%.60s] %.192s", tag, log_buffer);

    send_log_to_backend("ERROR", tagged_message);
    ESP_LOGE(tag, "%s", log_buffer);

    va_end(args);
}

void log_debug(const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char log_buffer[256];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    char tagged_message[256];
    snprintf(tagged_message, sizeof(tagged_message), "[%.60s] %.192s", tag, log_buffer);

    send_log_to_backend("DEBUG", tagged_message);
    ESP_LOGD(tag, "%s", log_buffer);

    va_end(args);
}

void log_warn(const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char log_buffer[256];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    char tagged_message[256];
    snprintf(tagged_message, sizeof(tagged_message), "[%.60s] %.192s", tag, log_buffer);

    send_log_to_backend("WARN", tagged_message);
    ESP_LOGW(tag, "%s", log_buffer);

    va_end(args);
}