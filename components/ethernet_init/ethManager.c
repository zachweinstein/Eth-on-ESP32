#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_netif_ip_addr.h"  // Required for esp_ip4addr_aton()
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "esp_eth.h"
#include "ethernet_init.h"
#include "esp_http_server.h"

#include "freertos/event_groups.h"

#include "utils.h"

extern EventGroupHandle_t g_network_event_group;

static esp_eth_handle_t *eth_handles ;
static esp_netif_t *eth_netifs ;
static esp_eth_netif_glue_handle_t eth_netif_glues;
static int currState = INIT;

static char* TAG = "ethManager";
extern char event_status[64];
// This handler will be called when the Ethernet interface gets an IP address
void eth_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        ESP_LOGI(TAG, "Ethernet Got IP Address");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");

        // Optional: Check which netif got the IP, if you have multiple interfaces
        // const char *netif_key;
        // esp_netif_get_netif_impl_index(netif); // if needed
    }
}

/** Event handler for Ethernet events */
void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        // Link detected signal network manager
        xEventGroupSetBits(g_network_event_group, ETH_MODE);
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        // Link lost signal network manager
        xEventGroupSetBits(g_network_event_group, WIFI_MODE);
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;

    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");

        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

esp_err_t init_eth_state_handler(void){

    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netifs  = esp_netif_new(&cfg);
    eth_netif_glues = esp_eth_new_netif_glue(eth_handles[0]);

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netifs)); // Stop DHCP

    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = esp_ip4addr_aton("10.0.0.124");
    ip_info.gw.addr = esp_ip4addr_aton("10.0.0.1");
    ip_info.netmask.addr = esp_ip4addr_aton("255.255.255.0");
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netifs, &ip_info));

    return ESP_OK;
}

void ready_eth_state_handler(void){
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netifs, eth_netif_glues));
    esp_eth_start(eth_handles[0]);
}

esp_err_t run_state_handler(void){
    
    EventBits_t bits = xEventGroupWaitBits(g_network_event_group, 
        ETH_MODE | ETH_ENABLE | ETH_DISABLE ,
        pdTRUE, pdFALSE, portMAX_DELAY
    );

    if (bits & ETH_ENABLE){
        ESP_LOGI(TAG, "Connecting to Eth Network: ");
        //ESP_ERROR_CHECK(esp_netif_dhcpc_start(eth_netifs));
        strcpy(event_status, "Ethernet Connected");
        return ESP_OK;
    }

    
    else if (bits & ETH_DISABLE)
    {
        //ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netifs));
        return 5;
    }
    
    

    //ESP_LOGE(TAG, "Unexpected Wi-Fi error");
    return ESP_FAIL;
    
}

esp_err_t reset_state_handler(void){

    ESP_ERROR_CHECK(esp_eth_stop(eth_handles));
    ESP_ERROR_CHECK(esp_eth_del_netif_glue(eth_netif_glues));
    ESP_ERROR_CHECK(example_eth_deinit(eth_handles, 0));
    return ESP_OK;

}

void eth_task(void* pvArgs){

    int currState = INIT;
    
    while(1){

        switch (currState)
        {
        case INIT:
        
        init_eth_state_handler();
            ESP_LOGE(TAG,"Init state completed");
            currState = READY;
            break;
        
        case READY:
        ready_eth_state_handler();
            ESP_LOGE(TAG,"ready state completed");
            currState = RUN;
            break;

        case RUN:
            int ret = run_state_handler();
            if(ESP_OK == ret){
                ESP_LOGE(TAG,"run state passed");
                currState = RUN;
            }
            else if (5 == run_state_handler()){
                currState = RESET;
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