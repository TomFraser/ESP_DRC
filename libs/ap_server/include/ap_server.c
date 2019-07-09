#include <string.h>
#include "esp_wifi.h"
#include "ap_server.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include <esp_http_server.h>
#include "esp_event.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "../../ESP_common.h"
#include "Servo.h"

static const char *TAG = "http_server";

#define ESP_WIFI_SSID      "WACT² Robot"
#define ESP_WIFI_PASS      "YeetYeet"
#define MAX_STA_CONN       5

#define NUMBER_OF_STRING 20
#define MAX_STRING_SIZE 50

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static robot_t * robot;


/* 
    Wifi Event Handler
*/

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "---station joined---");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "---station leave---");
            break;
        default:
            break;
    }
    return ESP_OK;
}





/* 
 * handlers for the web server.
 */

char terminal[NUMBER_OF_STRING][MAX_STRING_SIZE] = {"WACT² DRC"};

static esp_err_t settings_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {            
            char value[5];
            if (httpd_query_key_value(buf, "speed", (char *) &value, 5) == ESP_OK) {
                robot->max_speed = atoi(value);
                ESP_LOGE(TAG, "Received speed command: %d", robot->max_speed);

                nvs_open("storage", NVS_READWRITE, &nvs_data_handle);
                nvs_set_i8(nvs_data_handle, "speed_max", robot->max_speed);
                nvs_commit(nvs_data_handle);
                nvs_close(nvs_data_handle);
            }
            if (httpd_query_key_value(buf, "steering", (char *) &value, 5) == ESP_OK) {
                robot->boost = atoi(value) * 1000;
                ESP_LOGE(TAG, "Received boost command: %d", robot->boost);

                nvs_open("storage", NVS_READWRITE, &nvs_data_handle);
                nvs_set_i32(nvs_data_handle, "steering_correction", robot->boost);
                nvs_commit(nvs_data_handle);
                nvs_close(nvs_data_handle);
            }
        }
        free(buf);
    }

    char resp_str[375];
    sprintf(resp_str, "<!DOCTYPE html><html><head><title>WACT² DRC</title></head><body><h1>Settings</h1><form action='/settings'>Speed:<br><input type='text' name='speed' value='%i'><br><br>Steering Offset:<br><input type='text' name='steering' value='%d'><br><input type='submit' value='Submit'></form><form action='/'><br><br><input type='submit' value='<- BACK'></form></body></html>", robot->max_speed, robot->boost / 1000); 

    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

static esp_err_t terminal_handler(httpd_req_t *req)
{
    char* resp_str = (char*)malloc(1067 * sizeof(char));
    sprintf(resp_str, "<!DOCTYPE html><html><head><title>WACT² DRC</title><meta http-equiv='refresh' content='2'></head><body style='font-size=50px;'>%s<br><form action='/'><br><br><input type='submit' value='<- BACK'></form></body></html>", terminal[0]);
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    return ESP_OK;
}

static esp_err_t home_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    const char* resp_str = "<!DOCTYPE html><h1 style='font-size:50px;text-align:center;'>Home</h1><form action='/'><input type='hidden'name='start'value='1'><input style='background:#48A9A6;color:white;height:20%;width:100%;font:bold 50px arial;'type='submit'value='START'></form><br><br><form action='/'><input type='hidden'name='start'value='2'><input style='background:#D62839;color:white;height:20%;width:100%;font:bold 50px arial;'type='submit'value='PAUSE'></form><br><br><form action='/'><input type='hidden'name='start'value='0'><input style='background:#D62839;color:white;height:20%;width:100%;font:bold 50px arial;'type='submit'value='STOP'></form><br><br><form action='/settings'><input style='background:#175676;color:white;height:20%;width:100%;font:bold 50px arial;'type='submit'value='SETTINGS'></form><br><br><form action='/'><input type='hidden'name='start'value='3'><input style='background:#D62839;color:white;height:20%;width:100%;font:bold 50px arial;'type='submit'value='CALIB ESC'></form><br><br><form action='/term'><input style='background:#885053;color:white;height:20%;width:100%;font:bold 50px arial;'type='submit'value='TERMINAL'></form>";

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;

    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char value[2];
            httpd_query_key_value(buf, "start", (char *) &value, 2);
            uint8_t recv_value = atoi(value);
            if (recv_value == 3) {
                mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 2000);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 1000);
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                robot->stop = 0;
            }
            else if (recv_value == 2) {
                robot->boost_time = esp_timer_get_time();
                robot->stop = 1;
            }
            else {
                robot->stop = recv_value;
            }
            ESP_LOGE(TAG, "Received stop command: %d", robot->stop);
        }
        free(buf);
    }
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    static const httpd_uri_t settings = {
        .uri       = "/settings",
        .method    = HTTP_GET,
        .handler   = settings_handler,
        .user_ctx  = NULL
    };

    static const httpd_uri_t terminal = {
        .uri       = "/term",
        .method    = HTTP_GET,
        .handler   = terminal_handler,
        .user_ctx  = NULL
    };

    static const httpd_uri_t uri_home = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = home_handler,
        .user_ctx  = NULL
    };

    // Start the httpd server
    ESP_LOGW(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGW(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &settings);
        httpd_register_uri_handler(server, &terminal);
        httpd_register_uri_handler(server, &uri_home);
        return server;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return NULL;
}





/*
    AP Mode
*/

static void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized) {
        return;
    }
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    initialized = true;
}

void wifi_init()
{
    initialise_wifi();
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    //wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s\n", ESP_WIFI_SSID, ESP_WIFI_PASS);
}


/*
    Client Mode
*/

bool wifi_join(const char *ssid, const char *pass, int timeout_ms)
{
    wifi_config_t wifi_config = { 0 };
    strncpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strncpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_connect() );
    
    ESP_LOGI(TAG, "try to connect to ap SSID:%s password:%s", ssid, pass);
    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                   pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
    return (bits & CONNECTED_BIT) != 0;
}

bool wifi_connect(robot_t * robot_)
{
    robot = robot_;
  
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    initialise_wifi();
    // check wifi connected already
    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                   pdFALSE, pdTRUE, 300 / portTICK_PERIOD_MS);
    if((bits & CONNECTED_BIT) != 0)
        return true;

    if(wifi_join("WACT^2 Router", "YeetYeet", 10000)) {
        start_webserver();
        return true;
    }

    return false;
}

// ==================================================================================================
//          Start http server
// ==================================================================================================
void start_http_server(robot_t * robot_)
{
    robot = robot_;

    wifi_init();
    start_webserver();
}