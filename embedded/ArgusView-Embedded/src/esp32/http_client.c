#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "HttpClient";

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

    esp_http_client_write(client, body_start, strlen(body_start));
    esp_http_client_write(client, (const char *)image_data, image_size);
    esp_http_client_write(client, body_end, strlen(body_end));

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return -1;
    }

    esp_http_client_cleanup(client);
    return 0;
}
