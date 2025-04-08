#include "esp32/wifi.h"
#include "image_handler.h"
#include "esp32/logger.h" // Include the new logging library
#include "esp32/camera_flash.h" // Include the camera flash control API
#include "esp32/connectivity.h" // Include the connectivity API
// #include "esp_log.h"
// #include "esp_heap_caps.h"

#define BACKEND_URL "http://vcm-46428.vm.duke.edu:3000/describe?wordCount=20"

void app_main() {
    // if (psramFound()) {
    //     ESP_LOGI("PSRAM", "PSRAM is enabled and available.");
    // } else {
    //     ESP_LOGE("PSRAM", "PSRAM is not available. Check your configuration.");
    // }

    // size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    // ESP_LOGI("PSRAM", "Free PSRAM: %d bytes", free_psram);

    log_info("Starting application..."); // Log application start

    wifi_init_sta("DukeVisitor", ""); // Connect to Wi-Fi
    log_info("Wi-Fi initialized.");

    if (!check_connectivity("www.google.com")) { // Test connectivity with Google
        log_error("Google is not reachable.");
    }

    // Capture and send the image
    if (capture_and_send_image(BACKEND_URL) != 0) {
        log_error("Failed to capture and send image."); // Log error
    }
}