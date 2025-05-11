#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "webserver";

// Simple HTML page
static const char *html_page = "<!DOCTYPE html><html><body><h1>Hello from ESP32!</h1></body></html>";

// HTTP GET handler
esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// URI handler structure
httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = root_get_handler,
    .user_ctx = NULL
};

// Start web server
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
    }
    return server;
}


