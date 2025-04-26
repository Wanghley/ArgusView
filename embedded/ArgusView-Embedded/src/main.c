#include "esp32/wifi.h"
#include "image_handler.h"
#include "esp32/logger.h"
#include "esp32/camera_flash.h"
#include "esp32/connectivity.h"
#include "esp32/button.h"
#include "esp32/audio_recorder.h"
#include "esp32/http_client.h"
#include "esp_timer.h"
#include "esp_log.h"

#define LONG_PRESS_THRESHOLD_MS 2000 // 2 seconds for long press
#define BACKEND_URL_IMAGE_ONLY "http://vcm-46428.vm.duke.edu:3000/describe?wordCount=20"
#define BACKEND_URL_IMAGE_AND_AUDIO "http://vcm-46428.vm.duke.edu:3000/describe_with_context?wordCount=20"
#define BACKEND_ALIVE_URL "http://vcm-46428.vm.duke.edu:3000/alive"
#define BACKEND_LOG_URL "http://vcm-46428.vm.duke.edu:3000/log"

static int64_t press_start_time = 0;
static bool recording_audio = false;

static const char *TAG = "ArgusView";

void button_press_handler() {
    press_start_time = esp_timer_get_time(); // Record the press start time
    recording_audio = true;

    log_info(TAG, "Long press detected, starting audio recording...");
    if (start_audio_recording() != 0) {
        log_error(TAG, "Failed to start audio recording.");
        recording_audio = false;
    }
}

void button_release_handler() {
    int64_t press_duration = (esp_timer_get_time() - press_start_time) / 1000; // Calculate press duration in ms

    uint8_t *audio_data = NULL;
    size_t audio_size = 0;

    if (recording_audio) {
        log_info(TAG, "Stopping audio recording...");
        if (stop_audio_recording(&audio_data, &audio_size) != 0) {
            log_error(TAG, "Failed to stop audio recording.");
            return;
        }
        recording_audio = false;
    }

    if (press_duration < LONG_PRESS_THRESHOLD_MS) {
        log_info(TAG, "Short press detected, capturing and sending image...");
        if (capture_and_send_image(BACKEND_URL_IMAGE_ONLY) != 0) {
            log_error(TAG, "Failed to capture and send image.");
        }
    } else {
        log_info(TAG, "Long press detected, sending image and audio...");
        if (capture_and_send_image_with_audio(BACKEND_URL_IMAGE_AND_AUDIO, audio_data, audio_size) != 0) {
            log_error(TAG, "Failed to send image and audio.");
        }
    }

    if (audio_data) {
        free(audio_data); // Free the allocated audio buffer if it exists
    }
}

void app_main() {
    ESP_LOGI(TAG, "Starting application...");

    wifi_init_sta("DukeVisitor", ""); // Connect to Wi-Fi
    log_info(TAG, "Wi-Fi initialized.");

    if (!check_connectivity("www.google.com")) {
        log_error(TAG, "Google is not reachable.");
    }

    // Notify backend that the device is alive
    if (send_alive_request(BACKEND_ALIVE_URL) == 0) {
        log_info(TAG, "Alive request sent successfully.");
    } else {
        log_error(TAG, "Failed to send alive request.");
    }

    // Initialize button listener with press and release handlers
    init_button(button_press_handler, button_release_handler);
    log_info(TAG, "Button listener initialized.");

    // Main loop
    // while (1) {
    //     vTaskDelay(pdMS_TO_TICKS(1000)); // Keep the application running
    // }
}