idf_component_register(SRCS "wifiManager.c" "ethManager.c" "ethernet_init.c" "wifiHandler.c" "web_server.c" "network_manager.c"
                       PRIV_REQUIRES esp_driver_gpio esp_eth spi_flash nvs_flash esp_event esp_wifi esp_http_server
                       INCLUDE_DIRS ".")
