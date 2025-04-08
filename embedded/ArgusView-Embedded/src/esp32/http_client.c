#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"

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
    esp_http_client_config_t config = {
        .url = url,
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
    if (http_client_write_all(client, body_start, strlen(body_start)) != ESP_OK) goto cleanup;
    if (http_client_write_all(client, image_data, image_size) != ESP_OK) goto cleanup;
    if (http_client_write_all(client, body_end, strlen(body_end)) != ESP_OK) goto cleanup;

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    esp_http_client_cleanup(client);
    return 0;

cleanup:
    esp_http_client_cleanup(client);
    return -1;
}
