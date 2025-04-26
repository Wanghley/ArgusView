#include "image_handler.h"
#include "esp32/http_client.h"
#include "esp32/camera.h"
#include "esp_log.h"
#include "esp32/logger.h"

static const char *TAG = "ImageHandler";

int capture_and_send_image(const char *url) {
    camera_fb_t *fb = capture_image();
    if (!fb) {
        log_error(TAG, "Failed to capture image");
        return -1;
    } else {
        log_info(TAG, "Image captured successfully");
    }

    int result = send_image_http(url, fb->buf, fb->len);
    if (result != 0) {
        log_error(TAG, "Failed to send image");
    } else {
        log_info(TAG, "Image sent successfully");
    }

    release_image(fb);
    return 0;
}

int capture_and_send_image_with_audio(const char *url, const uint8_t *audio_data, size_t audio_size) {
    camera_fb_t *fb = capture_image();
    if (!fb) {
        log_error(TAG, "Failed to capture image");
        return -1;
    } else {
        log_info(TAG, "Image captured successfully");
    }

    int result = send_image_with_audio(url, fb->buf, fb->len, audio_data, audio_size);
    if (result != 0) {
        log_error(TAG, "Failed to send image with audio");
    } else {
        log_info(TAG, "Image and audio sent successfully");
    }

    release_image(fb);
    return result;
}
