#include "connectivity.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "ping/ping_sock.h"
#include "lwip/netdb.h"

static const char *TAG = "Connectivity";

static void ping_on_success(esp_ping_handle_t hdl, void *args) {
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    uint16_t seqno;
    uint8_t ttl;

    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));

    // Set reachable status based on received packets
    *(bool *)args = (recv_len > 0);
    
    ESP_LOGI(TAG, "%d bytes from %s icmp_seq=%d ttl=%d time=%d ms",
             (int)recv_len, inet_ntoa(target_addr.u_addr.ip4), (int)seqno, (int)ttl, (int)elapsed_time);
}

static void ping_on_timeout(esp_ping_handle_t hdl, void *args) {
    uint16_t seqno;
    ip_addr_t target_addr;

    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));

    ESP_LOGI(TAG, "From %s icmp_seq=%d timeout", inet_ntoa(target_addr.u_addr.ip4), (int)seqno);
}

static void ping_on_end(esp_ping_handle_t hdl, void *args) {
    uint32_t transmitted, received, total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));

    ESP_LOGI(TAG, "%d packets transmitted, %d received, time %dms", (int)transmitted, (int)received, (int)total_time_ms);
    // Set reachable status based on received packets
    *(bool *)args = (received > 0);
    esp_ping_delete_session(hdl);
}

bool check_connectivity(const char *target_fqdn) {
    ip_addr_t target_ip;
    struct addrinfo hint, *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_ip, 0, sizeof(target_ip));

    if (getaddrinfo(target_fqdn, NULL, &hint, &res) != 0) {
        ESP_LOGE(TAG, "Failed to resolve FQDN: %s", target_fqdn);
        return false;
    }

    struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_ip), &addr4);
    freeaddrinfo(res);

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_ip;

    esp_ping_callbacks_t cbs = {
        .on_ping_success = ping_on_success,
        .on_ping_timeout = ping_on_timeout,
        .on_ping_end = ping_on_end,
    };

    bool reachable = false;
    cbs.cb_args = &reachable;

    esp_ping_handle_t ping;
    if (esp_ping_new_session(&ping_config, &cbs, &ping) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session");
        return false;
    }

    if (esp_ping_start(ping) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ping");
        esp_ping_delete_session(ping);
        return false;
    }

    // Wait for ping to complete
    ESP_LOGI(TAG, "Pinging %s...", target_fqdn);
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "Ping completed");

    return reachable;
}
