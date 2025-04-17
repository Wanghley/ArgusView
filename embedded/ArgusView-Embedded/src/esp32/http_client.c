#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "audio_player.h" // Include the audio playback API

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

int send_image_http(const char *url, const uint8_t *image_data, size_t image_size) {
    ESP_LOGI(TAG, "Sending image to URL: %s", url);
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 20000,
        .buffer_size = 4096,
        .buffer_size_tx = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);

    char boundary[] = "----ESP32Boundary";
    char content_type[64];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);

    char body_start[128];
    snprintf(body_start, sizeof(body_start),
             "--%s\r\nContent-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n",
             boundary);

    char body_end[64];
    snprintf(body_end, sizeof(body_end), "\r\n--%s--\r\n", boundary);

    // Calculate the total content length
    size_t content_length = strlen(body_start) + image_size + strlen(body_end);
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
    ESP_LOGI(TAG, "Writing multipart form data");
    if (http_client_write_all(client, body_start, strlen(body_start)) != ESP_OK) goto cleanup;
    ESP_LOGI(TAG, "Writing image data");
    if (http_client_write_all(client, image_data, image_size) != ESP_OK) goto cleanup;
    ESP_LOGI(TAG, "Writing end boundary");
    if (http_client_write_all(client, body_end, strlen(body_end)) != ESP_OK) goto cleanup;

    // esp_err_t err = esp_http_client_perform(client);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    //     goto cleanup;
    // }

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

    // Read the response body
    char audio_buffer[1024]; // Adjust size as needed
    int total_read = 0;
    ESP_LOGI(TAG, "Content-Length: %d", ((int) esp_http_client_get_content_length(client)));
    while (1) {
        int read_len = esp_http_client_read(client, audio_buffer + total_read, sizeof(audio_buffer) - total_read);
        if (read_len == 0) {
            ESP_LOGI(TAG, "End of response body reached");
            break; // End of response body
        }
        if (read_len < 0) {
            ESP_LOGE(TAG, "Error reading HTTP response body: %s", esp_err_to_name(read_len));
            goto cleanup;
        }

        ESP_LOGI(TAG, "Read %d bytes from response body", read_len);
        total_read += read_len;

        if (total_read >= sizeof(audio_buffer)) {
            ESP_LOGW(TAG, "Buffer full, truncating response");
            break;
        }
    }

    if (total_read > 0) {
        ESP_LOGI(TAG, "Response body received, size: %d bytes", total_read);
        // Process the response body (e.g., play audio)
        if (play_audio((uint8_t *)audio_buffer, total_read) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to play audio");
        }
    } else {
        ESP_LOGE(TAG, "No response body received");
    }

    esp_http_client_cleanup(client);
    return 0;

cleanup:
    esp_http_client_cleanup(client);
    return -1;
}
