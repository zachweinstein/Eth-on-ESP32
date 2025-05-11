#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "webserver";

#if 0
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
#endif

// Global event status string
char event_status[64] = "Waiting for event...";

// HTTP handler for /status
esp_err_t status_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, event_status, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Serve a simple HTML page
esp_err_t html_handler(httpd_req_t *req)
{
    const char* html = "<!DOCTYPE html><html><head><title>Status</title></head>"
                       "<body><h1>ESP32 Ethernet Status</h1><p id='status'>...</p>"
                       "<script>setInterval(()=>fetch('/status').then(r=>r.text()).then(t=>{document.getElementById('status').innerText=t}),1000);</script>"
                       "</body></html>";
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Start HTTP server
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t status_uri = {
            .uri = "/status",
            .method = HTTP_GET,
            .handler = status_get_handler
        };
        httpd_register_uri_handler(server, &status_uri);

        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = html_handler
        };
        httpd_register_uri_handler(server, &root_uri);
    }
    return server;
}


