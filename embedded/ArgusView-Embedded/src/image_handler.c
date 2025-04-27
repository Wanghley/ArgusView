#include "image_handler.h"
#include "esp32/http_client.h"
#include "esp32/camera.h"
#include "esp_log.h"
#include "esp32/logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"

static const char *TAG = "ImageHandler";

typedef struct {
    const char *url;
    camera_fb_t *image_fb;
    uint8_t *audio_data;
    size_t audio_size;
} upload_task_params_t;

static void upload_task(void *params) {
    upload_task_params_t *task_params = (upload_task_params_t *)params;

    if (task_params->audio_data) {
        log_info(TAG, "Uploading image and audio...");
        send_image_with_audio(
            task_params->url,
            task_params->image_fb->buf,
            task_params->image_fb->len,
            task_params->audio_data,
            task_params->audio_size
        );
        free(task_params->audio_data); // Free audio data
    } else {
        log_info(TAG, "Uploading image...");
        send_image_http(task_params->url, task_params->image_fb->buf, task_params->image_fb->len);
    }

    release_image(task_params->image_fb); // Release the image buffer
    free(task_params); // Free task parameters
    vTaskDelete(NULL);
}

int capture_and_send_image(const char *url) {
    camera_fb_t *fb = capture_image();
    if (!fb) {
        log_error(TAG, "Failed to capture image");
        return -1;
    } else {
        log_info(TAG, "Image captured successfully");
    }

    upload_task_params_t *params = malloc(sizeof(upload_task_params_t));
    if (!params) {
        log_error(TAG, "Failed to allocate memory for task parameters");
        release_image(fb);
        return -1;
    }

    params->url = url;
    params->image_fb = fb;
    params->audio_data = NULL;
    params->audio_size = 0;

    if (xTaskCreate(upload_task, "upload_task", 8192, params, 5, NULL) != pdPASS) {
        log_error(TAG, "Failed to create upload task");
        release_image(fb);
        free(params);
        return -1;
    }

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

    upload_task_params_t *params = malloc(sizeof(upload_task_params_t));
    if (!params) {
        log_error(TAG, "Failed to allocate memory for task parameters");
        release_image(fb);
        return -1;
    }

    params->url = url;
    params->image_fb = fb;
    params->audio_data = audio_data; // Pass the audio data pointer directly
    params->audio_size = audio_size;

    log_info(TAG, "Pointer to audio data: %p", params->audio_data);
    log_info(TAG, "Audio data size: %d bytes", audio_size);

    if (xTaskCreate(upload_task, "upload_task", 8192, params, 5, NULL) != pdPASS) {
        log_error(TAG, "Failed to create upload task");
        release_image(fb);
        free(audio_data);
        free(params);
        return -1;
    }

    return 0;
}
