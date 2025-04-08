#include "image_handler.h"
#include "esp32/http_client.h"
#include "esp32/camera.h"
#include "esp_log.h"

static const char *TAG = "ImageHandler";

int capture_and_send_image(const char *url) {
    camera_fb_t *fb = capture_image();
    if (!fb) {
        ESP_LOGE(TAG, "Failed to capture image");
        return -1;
    } else {
        ESP_LOGI(TAG, "Image captured successfully");
    }

    int result = send_image_http(url, fb->buf, fb->len);
    if (result != 0) {
        ESP_LOGE(TAG, "Failed to send image");
    } else {
        ESP_LOGI(TAG, "Image sent successfully");
    }

    release_image(fb);
    return 0;
}
