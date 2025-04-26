#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "audio_player.h"
#include "esp_heap_caps.h"

static const char *TAG = "HttpClient";

static esp_err_t http_client_write_all(esp_http_client_handle_t client, const void *data, size_t len) {
    size_t total_written = 0;
    while (total_written < len) {
        int written = esp_http_client_write(client, (const char *)data + total_written, len - total_written);
        if (written <= 0) {
            ESP_LOGE(TAG, "Failed to write data");
            return ESP_FAIL;
        }
        total_written += written;
    }
    return ESP_OK;
}

static int do_send_request(const char *url, const uint8_t *image_data, size_t image_size, const uint8_t *audio_data, size_t audio_size) {
    ESP_LOGI(TAG, "Sending request to URL: %s", url);
    esp_http_client_config_t config = {
        .url = url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);

    char boundary[] = "----ESP32Boundary";
    char content_type[64];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);

    char body_start_image[128];
    snprintf(body_start_image, sizeof(body_start_image),
             "--%s\r\nContent-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n",
             boundary);

    char body_start_audio[128] = "";
    if (audio_data) {
        snprintf(body_start_audio, sizeof(body_start_audio),
                 "\r\n--%s\r\nContent-Disposition: form-data; name=\"audio\"; filename=\"audio.wav\"\r\nContent-Type: audio/wav\r\n\r\n",
                 boundary);
    }

    char body_end[64];
    snprintf(body_end, sizeof(body_end), "\r\n--%s--\r\n", boundary);

    // Calculate the total content length
    size_t content_length = strlen(body_start_image) + image_size + strlen(body_start_audio) + (audio_data ? audio_size : 0) + strlen(body_end);
    char content_length_str[32];
    snprintf(content_length_str, sizeof(content_length_str), "%zu", content_length);
    esp_http_client_set_header(client, "Content-Length", content_length_str);

    // Open the HTTP connection
    if (esp_http_client_open(client, content_length) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        esp_http_client_cleanup(client);
        return -1;
    }

    // Write the multipart form data
    ESP_LOGI(TAG, "Writing multipart form data for image");
    if (http_client_write_all(client, body_start_image, strlen(body_start_image)) != ESP_OK) goto cleanup;
    if (http_client_write_all(client, image_data, image_size) != ESP_OK) goto cleanup;

    if (audio_data) {
        ESP_LOGI(TAG, "Writing multipart form data for audio");
        if (http_client_write_all(client, body_start_audio, strlen(body_start_audio)) != ESP_OK) goto cleanup;
        if (http_client_write_all(client, audio_data, audio_size) != ESP_OK) goto cleanup;
    }

    ESP_LOGI(TAG, "Writing end boundary");
    if (http_client_write_all(client, body_end, strlen(body_end)) != ESP_OK) goto cleanup;

    // Check the HTTP response status code
    ESP_LOGI(TAG, "Fetching headers");
    int header_bytes = esp_http_client_fetch_headers(client);
    if (header_bytes < 0) {
        ESP_LOGE(TAG, "Failed to fetch headers (code: %d)", header_bytes);
        goto cleanup;
    }

    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status code: %d", status_code);
    if (status_code != 200) {
        ESP_LOGE(TAG, "Unexpected HTTP status code: %d", status_code);
        goto cleanup;
    }

    // Allocate buffer in PSRAM for the audio response
    size_t psram_buffer_size = 1024 * 1024; // 1 MB buffer (adjust as needed)
    uint8_t *psram_buffer = heap_caps_malloc(psram_buffer_size, MALLOC_CAP_SPIRAM);
    if (!psram_buffer) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM buffer");
        goto cleanup;
    }

    size_t total_read = 0;
    ESP_LOGI(TAG, "Content-Length: %d", ((int) esp_http_client_get_content_length(client)));
    while (1) {
        if (total_read >= psram_buffer_size) {
            ESP_LOGW(TAG, "PSRAM buffer full, truncating response");
            break;
        }

        int read_len = esp_http_client_read(client, (char *)(psram_buffer + total_read), psram_buffer_size - total_read);
        if (read_len == 0) {
            ESP_LOGI(TAG, "End of response body reached");
            break; // End of response body
        }
        if (read_len < 0) {
            ESP_LOGE(TAG, "Error reading HTTP response body: %s", esp_err_to_name(read_len));
            free(psram_buffer);
            goto cleanup;
        }

        ESP_LOGI(TAG, "Read %d bytes from response body", read_len);
        total_read += read_len;
    }

    if (total_read > 0) {
        ESP_LOGI(TAG, "Response body received, size: %d bytes", total_read);
        // Play audio from PSRAM
        if (play_audio(psram_buffer, total_read) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to play audio");
        }
    } else {
        ESP_LOGE(TAG, "No response body received");
    }

    free(psram_buffer); // Free the PSRAM buffer
    esp_http_client_cleanup(client);
    return 0;

cleanup:
    esp_http_client_cleanup(client);
    return -1;
}

int send_image_http(const char *url, const uint8_t *image_data, size_t image_size) {
    return do_send_request(url, image_data, image_size, NULL, 0);
}

int send_image_with_audio(const char *url, const uint8_t *image_data, size_t image_size, const uint8_t *audio_data, size_t audio_size) {
    return do_send_request(url, image_data, image_size, audio_data, audio_size);
}
