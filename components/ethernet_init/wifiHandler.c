// wifi_tutorial.c
#include <stdio.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "freertos/task.h"

// Enter the Wi-Fi credentials here
#define WIFI_SSID "HartwickHome"
#define WIFI_PASSWORD "sahat123"

static char* TAG = "wifiHandler";

extern esp_err_t tutorial_init(void);

extern esp_err_t tutorial_connect(char* wifi_ssid, char* wifi_password);

extern esp_err_t tutorial_disconnect(void);

extern esp_err_t tutorial_deinit(void);

static void ping_success_callback(esp_ping_handle_t hdl, void *args) {
    uint32_t elapsed_time =0;
    //esp_ping_get_profile(hdl, ESP_PING_PROF_RESPONSE_TIME, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI("PING", "Ping success");
}

static void ping_end_callback(esp_ping_handle_t hdl, void *args) {
    uint32_t transmitted, received;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    ESP_LOGI("PING", "Ping finished");
    esp_ping_stop(hdl);
    //esp_ping_delete(hdl);
}

void ping_host(const char *target_ip) {
    ip_addr_t target_addr;
    inet_pton(AF_INET, target_ip, &target_addr.u_addr.ip4);
    target_addr.type = IPADDR_TYPE_V4;

    ESP_LOGI(TAG, "Pinging %s", target_ip);

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target_addr;
    config.count = 4;

    esp_ping_callbacks_t cbs = {
        .on_ping_success = ping_success_callback,
        .on_ping_end = ping_end_callback
    };

    esp_ping_handle_t ping;
    esp_ping_new_session(&config, &cbs, &ping);
    esp_ping_start(ping);
}

void wifi_main(void)
{
    ESP_LOGI(TAG, "Starting tutorial...");
    ESP_ERROR_CHECK(tutorial_init());

    esp_err_t ret = tutorial_connect(WIFI_SSID, WIFI_PASSWORD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi network");
    }

    wifi_ap_record_t ap_info;
    ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_ERR_WIFI_CONN) {
        ESP_LOGE(TAG, "Wi-Fi station interface not initialized");
    }
    else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGE(TAG, "Wi-Fi station is not connected");
    } else {
        ESP_LOGI(TAG, "--- Access Point Information ---");
        ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
        ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
        ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
        ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);
        ping_host("8.8.8.8");
        ESP_LOGI(TAG, "Disconnecting in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    //ESP_ERROR_CHECK(tutorial_disconnect());

    //ESP_ERROR_CHECK(tutorial_deinit());

    ESP_LOGI(TAG, "End of tutorial...");
}
