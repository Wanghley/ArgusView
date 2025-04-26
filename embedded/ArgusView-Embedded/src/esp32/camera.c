#include "camera.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "logger.h"

#define PWDN_PIN 32 // Power down pin

static const char *TAG = "Camera";
static bool camera_initialized = false;

// Function to turn the camera on
void turn_camera_on() {
    if (PWDN_PIN != -1) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << PWDN_PIN,
            .mode = GPIO_MODE_OUTPUT,
        };
        gpio_config(&io_conf);
        gpio_set_level(PWDN_PIN, 0); // Set PWDN pin to LOW to power up the camera
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Function to turn the camera off
void turn_camera_off() {
    if (PWDN_PIN != -1) {
        gpio_set_level(PWDN_PIN, 1); // Set PWDN pin to HIGH to power down the camera
    }
}

void camera_init() {
    camera_config_t config = {
        .pin_pwdn = PWDN_PIN,          // Power down pin
        .pin_reset = -1,         // Reset pin (not used)
        .pin_xclk = 0,           // XCLK pin
        .pin_sccb_sda = 26,      // SDA pin
        .pin_sccb_scl = 27,      // SCL pin
        .pin_d7 = 35,            // Y9 pin
        .pin_d6 = 34,            // Y8 pin
        .pin_d5 = 39,            // Y7 pin
        .pin_d4 = 36,            // Y6 pin
        .pin_d3 = 21,            // Y5 pin
        .pin_d2 = 19,            // Y4 pin
        .pin_d1 = 18,            // Y3 pin
        .pin_d0 = 5,             // Y2 pin
        .pin_vsync = 25,         // VSYNC pin
        .pin_href = 23,          // HREF pin
        .pin_pclk = 22,          // PCLK pin
        .xclk_freq_hz = 20000000, // XCLK frequency
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG, // Output format
        .frame_size = FRAMESIZE_HD,   // Frame size
        .jpeg_quality = 5,            // JPEG quality (lower is better)
        .fb_count = 1,                  // Frame buffer count
        .fb_location = CAMERA_FB_IN_PSRAM, // Frame buffer location
        // .grab_mode = CAMERA_GRAB_WHEN_EMPTY
    };

    // Power up the camera
    turn_camera_on();

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        log_error(TAG, "Camera init failed with error 0x%x: %s", err, esp_err_to_name(err));
    } else {
        camera_initialized = true;
    }
}

camera_fb_t* capture_image() {
    // Activate logs for "camera" tag
    esp_log_level_set("camera", ESP_LOG_DEBUG);

     // Check if the camera is initialized

    if (!camera_initialized) {
        log_warn(TAG, "Camera not initialized. Initializing now...");
        camera_init();
        log_info(TAG, "Waiting for camera to stabilize.");
        vTaskDelay(2000 / portTICK_PERIOD_MS); // wait for 2 seconds
        if (!camera_initialized) {
            log_error(TAG, "Failed to initialize camera.");
            return NULL;
        }
    }

    turn_camera_on(); // Power up the camera

    camera_fb_t *fb = NULL;
    int attempts = 0;
    while (attempts < 5) { // try 5 times
        fb = esp_camera_fb_get();
        if (fb) {
            break;
        } else {
            log_warn(TAG, "Capture failed, retrying (%d)...", attempts + 1);
            vTaskDelay(500 / portTICK_PERIOD_MS); // wait 500ms before retry
        }
        attempts++;
    }
    if (!fb) {
        log_error(TAG, "Failed to capture image after retries.");
    }
    return fb;
}

void release_image(camera_fb_t *fb) {
    esp_camera_fb_return(fb);
}
