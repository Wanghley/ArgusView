#include "wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define MAX_RETRY 5

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD \
    (strlen(EXAMPLE_ESP_WIFI_PASS) == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK)

#define MAX_RETRIES 5

static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static volatile bool is_wifi_enabled = false;
static volatile bool intentional_disconnect = false; // New flag to track intentional disconnects
static wifi_config_t global_wifi_config; // Global variable to store Wi-Fi configuration

bool is_wifi_enabled_status() {
    return is_wifi_enabled;
}

void pause_wifi() {
    if (is_wifi_enabled) {
        ESP_LOGI(TAG, "Pausing Wi-Fi...");
        intentional_disconnect = true; // Mark disconnect as intentional
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        is_wifi_enabled = false;
    } else {
        ESP_LOGW(TAG, "Wi-Fi is already paused.");
    }
}

void resume_wifi() {
    if (!is_wifi_enabled) {
        ESP_LOGI(TAG, "Resuming Wi-Fi...");
        intentional_disconnect = false; // Reset the flag for normal operation
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

        // Reapply the stored Wi-Fi configuration
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &global_wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        // Give some 5 seconds for the Wi-Fi to reconnect
        // vTaskDelay(pdMS_TO_TICKS(5000));

        // Wait for Wi-Fi to reconnect
        is_wifi_enabled = true;
        // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // if (bits & WIFI_CONNECTED_BIT) {
        //     ESP_LOGI(TAG, "Wi-Fi reconnected successfully.");
        //     is_wifi_enabled = true;
        // } else if (bits & WIFI_FAIL_BIT) {
        //     ESP_LOGE(TAG, "Failed to reconnect Wi-Fi.");
        //     is_wifi_enabled = false;
        // } else {
        //     ESP_LOGE(TAG, "Unexpected event occurred during Wi-Fi resume.");
        //     is_wifi_enabled = false;
        // }
    } else {
        ESP_LOGW(TAG, "Wi-Fi is already running.");
    }
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initiate Wi-Fi connection: %s", esp_err_to_name(err));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (intentional_disconnect) {
            ESP_LOGI(TAG, "Wi-Fi disconnect was intentional, skipping reconnect.");
        } else if (s_retry_num < MAX_RETRIES) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP... Attempt %d/%d", s_retry_num, MAX_RETRIES);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(const char *ssid, const char *password) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = (strlen(password) == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK),
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    // Store the Wi-Fi configuration in the global variable
    memcpy(&global_wifi_config, &wifi_config, sizeof(wifi_config_t));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialization finished. Connecting to AP...");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP: %s", ssid);
        is_wifi_enabled = true; // Set to true when connected
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP: %s", ssid);
        is_wifi_enabled = false; // Set to false on failure
    } else {
        ESP_LOGE(TAG, "Unexpected event occurred");
        is_wifi_enabled = false; // Set to false on unexpected events
    }

    // Do not unregister event handlers if they are needed later
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
}
