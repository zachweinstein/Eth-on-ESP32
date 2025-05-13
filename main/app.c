#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_http_server.h"

extern void network_task(void* pvParam);
extern void wifi_task(void* pvArgs);
extern void eth_task(void* pvArgs);
extern void manual_switcher(void* pv);
extern httpd_handle_t start_webserver(void);


void app_main(void)
{

    xTaskCreate(&network_task, "network_task", 4096, NULL, 5, NULL);
    xTaskCreate(&eth_task, "network_task", 4096, NULL, 5, NULL);
    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 5, NULL);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 1 second


    xTaskCreate(&manual_switcher, "switcher_task", 2048, NULL, 3, NULL);

    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    
    start_webserver();

}