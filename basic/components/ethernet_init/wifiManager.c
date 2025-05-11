#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include <inttypes.h>
#include <string.h>

#include "freertos/event_groups.h"

#include "utils.h"
#include "esp_netif_ip_addr.h"  // Required for esp_ip4addr_aton()
extern EventGroupHandle_t g_network_event_group;

#define WIFI_AUTHMODE WIFI_AUTH_WPA2_PSK

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

extern char event_status[64];
static esp_netif_t *wifi_netif = NULL;
static esp_event_handler_instance_t ip_event_handler;
static esp_event_handler_instance_t wifi_event_handler;

static char* wifi_ssid = "HartwickHome";
static char* wifi_password = "sahat123";
static EventGroupHandle_t s_wifi_event_group = NULL;

static char* TAG = "wifiManager";

void ip_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Handling IP event, event code 0x%" PRIx32, event_id);
    switch (event_id)
    {
    case (IP_EVENT_STA_GOT_IP):
        ip_event_got_ip_t *event_ip = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event_ip->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case (IP_EVENT_STA_LOST_IP):
        ESP_LOGI(TAG, "Lost IP");
        break;
    case (IP_EVENT_GOT_IP6):
        ip_event_got_ip6_t *event_ip6 = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6: " IPV6STR, IPV62STR(event_ip6->ip6_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        ESP_LOGI(TAG, "IP event not handled");
        break;
    }
}

void wifi_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Handling Wi-Fi event, event code 0x%" PRIx32, event_id);

    switch (event_id)
    {
    case (WIFI_EVENT_STA_START):
        ESP_LOGI(TAG, "Wi-Fi started, connecting to AP...");
        esp_wifi_connect();
        break;
    case (WIFI_EVENT_STA_STOP):
        ESP_LOGI(TAG, "Wi-Fi stopped");
        break;
    case (WIFI_EVENT_STA_CONNECTED):
        ESP_LOGI(TAG, "Wi-Fi connected");
        break;
    case (WIFI_EVENT_STA_DISCONNECTED):
        ESP_LOGI(TAG, "Wi-Fi disconnected");
//        if (wifi_retry_count < WIFI_RETRY_ATTEMPT) {
//            ESP_LOGI(TAG, "Retrying to connect to Wi-Fi network...");
//            esp_wifi_connect();
//            wifi_retry_count++;
//        } else {
//            ESP_LOGI(TAG, "Failed to connect to Wi-Fi network");
//            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//        }
        break;
    default:
        ESP_LOGI(TAG, "Wi-Fi event not handled");
        break;
    }
}

static esp_err_t init_state_handler(void){

    s_wifi_event_group = xEventGroupCreate();

    // Initialize Non-Volatile Storage (NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(esp_netif_init());

    ret = esp_wifi_set_default_wifi_sta_handlers();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default handlers");
        return ret;
    }

    wifi_netif = esp_netif_create_default_wifi_sta();
    if (wifi_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(wifi_netif)); // Stop DHCP for Wi-Fi
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = esp_ip4addr_aton("10.0.0.124");
    ip_info.gw.addr = esp_ip4addr_aton("10.0.0.1");
    ip_info.netmask.addr = esp_ip4addr_aton("255.255.255.0");
    ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_netif, &ip_info));

    // Wi-Fi stack configuration parameters
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    return ESP_OK;
}

static void ready_state_handler(void){
    
    wifi_config_t wifi_config = {
        .sta = {
            // this sets the weakest authmode accepted in fast scan mode (default)
            .threshold.authmode = WIFI_AUTHMODE,
        },
    };

    strncpy((char*)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // default is WIFI_PS_MIN_MODEM
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); // default is WIFI_STORAGE_FLASH

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

}

static esp_err_t run_state_handler(void){

    EventBits_t bits = xEventGroupWaitBits(g_network_event_group, 
        WIFI_MODE | WIFI_ENABLE | WIFI_DISABLE ,
        pdTRUE, pdFALSE, portMAX_DELAY
    );

    if (bits & WIFI_ENABLE){
        ESP_LOGI(TAG, "Connecting to Wi-Fi network: ");
        ESP_ERROR_CHECK(esp_wifi_start());
        
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdTRUE, pdFALSE, portMAX_DELAY);
    
        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Connected to Wi-Fi network: ");
            strcpy(event_status, "Wifi Connected");
            return ESP_OK;
        } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGE(TAG, "Failed to connect to Wi-Fi network: ");
            return ESP_FAIL;
        }

        wifi_ap_record_t ap_info;
        int ret = esp_wifi_sta_get_ap_info(&ap_info);
        if (ret == ESP_ERR_WIFI_CONN) {
            ESP_LOGE(TAG, "Wi-Fi station interface not initialized");
        }
        else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
            ESP_LOGE(TAG, "Wi-Fi station is not connected");
        }else {
            ESP_LOGI(TAG, "--- Access Point Information ---");
            ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
            ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
            ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
            ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);
            //ping_host("8.8.8.8");
            ESP_LOGI(TAG, "Disconnecting in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

    }

    
    else if (bits & WIFI_DISABLE)
    {
        ESP_ERROR_CHECK(esp_wifi_stop());
        return ESP_OK;
    }
    
    

    //ESP_LOGE(TAG, "Unexpected Wi-Fi error");
    return ESP_FAIL;

}

static esp_err_t reset_state_handler(void){

    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
    return ESP_OK;

}

void wifi_task(void* pvArgs){

    int currState = INIT;
    
    while(1){

        switch (currState)
        {
        case INIT:
            init_state_handler();
            ESP_LOGE(TAG,"Init state completed");
            currState = READY;
            break;
        
        case READY:
            ready_state_handler();
            ESP_LOGE(TAG,"ready state completed");
            currState = RUN;
            break;

        case RUN:
            if(ESP_OK == run_state_handler()){
                ESP_LOGE(TAG,"run state passed");
                currState = RUN;
            }
            ESP_LOGE(TAG,"run state completed");
            
            break;
        
        case RESET:
            reset_state_handler();
            currState = INIT;
            break;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(100));

}