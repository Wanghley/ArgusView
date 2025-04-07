#include "logger.h"
#include "esp_log.h"

static const char *TAG = "ArgusView";

void log_info(const char *message) {
    ESP_LOGI(TAG, "%s", message);
}

void log_error(const char *message) {
    ESP_LOGE(TAG, "%s", message);
}

void log_debug(const char *message) {
    ESP_LOGD(TAG, "%s", message);
}
