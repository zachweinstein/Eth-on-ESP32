#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include <inttypes.h>
#include <string.h>

#include "freertos/event_groups.h"
#include "utils.h"
#include "ethernet_init.h"
EventGroupHandle_t g_network_event_group = NULL;
static int  currentMode = NONE;
static int  requestedMode = WIFI;

static esp_event_handler_instance_t ip_event_handler;
static esp_event_handler_instance_t eth_event_handler_instance;
static esp_event_handler_instance_t wifi_event_handler;

extern void ip_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
extern void wifi_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
extern void eth_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data);

extern esp_err_t init_eth_state_handler(void);
extern esp_err_t ready_eth_state_handler(void);


void manual_switcher(void* pv){
    int count = 0;
    while(count < 1){
        vTaskDelay(pdMS_TO_TICKS(100));
        xEventGroupSetBits(g_network_event_group,WIFI_MODE);
        //vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 10 second
        //xEventGroupSetBits(g_network_event_group, ETH_MODE);
        vTaskDelay(pdMS_TO_TICKS(10000000));
        count++;
    }
    
}


void network_task(){

    g_network_event_group = xEventGroupCreate();
    
    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // start the eth driver
    //init_eth_state_handler();
    //ready_eth_state_handler();
    


    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_cb,
        NULL,
        &wifi_event_handler));
  
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, 
        ESP_EVENT_ANY_ID, 
        &eth_event_handler, 
        NULL,
        &eth_event_handler_instance));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        ESP_EVENT_ANY_ID,
        &ip_event_cb,
        NULL,
        &ip_event_handler));

    while(1){
    
    EventBits_t bits = xEventGroupWaitBits(g_network_event_group, 
                                           WIFI_MODE | ETH_MODE ,
                                           pdTRUE, pdFALSE, portMAX_DELAY
                                          );

    if (bits & ETH_MODE)
    {
        ESP_LOGE("n/w","Eth mode Activated");
        requestedMode = ETH;
    }
    else{
        ESP_LOGE("n/w","WifI mode Activated");
        requestedMode = WIFI;
    }

    if ( currentMode != requestedMode){
        ESP_LOGE("n/w","Switching from %d",currentMode);
        switch (currentMode)
        {
        case WIFI:
            ESP_LOGE("n/w","Setting wifi dis from %d",currentMode);
            xEventGroupSetBits(g_network_event_group, WIFI_DISABLE | ETH_ENABLE);
            break;
        case ETH:
        default:
            ESP_LOGE("n/w","Setting wifi en from %d",currentMode);
            xEventGroupSetBits(g_network_event_group, WIFI_ENABLE | ETH_DISABLE);
            break;
        }
        currentMode = requestedMode;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }    
}