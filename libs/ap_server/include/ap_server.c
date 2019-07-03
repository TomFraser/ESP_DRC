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

static robot_t * robot;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGW(TAG, "---station joined---");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGW(TAG, "---station leave---");
            break;
        default:
            break;
    }
    return ESP_OK;
}

char terminal[NUMBER_OF_STRING][MAX_STRING_SIZE] = {"WACT² DRC"};
/* 
 * handlers for the web server.
 */
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
            char value[4];
            if (httpd_query_key_value(buf, "speed", (char *) &value, 4) == ESP_OK) {
                robot->max_speed = atoi(value);
                ESP_LOGE(TAG, "Received speed command: %d", robot->max_speed);

                nvs_open("storage", NVS_READWRITE, &nvs_data_handle);
                nvs_set_i8(nvs_data_handle, "speed_max", robot->max_speed);
                nvs_commit(nvs_data_handle);
                nvs_close(nvs_data_handle);
            } else if (httpd_query_key_value(buf, "steering", (char *) &value, 4) == ESP_OK) {
                robot->steering_correction = atoi(value);
                ESP_LOGE(TAG, "Received steering command: %d", robot->steering_correction);

                nvs_open("storage", NVS_READWRITE, &nvs_data_handle);
                nvs_set_i8(nvs_data_handle, "steering_correction", robot->steering_correction);
                nvs_commit(nvs_data_handle);
                nvs_close(nvs_data_handle);
            }
        }
        free(buf);
    }

    char resp_str[375];
    sprintf(resp_str, "<!DOCTYPE html><html><head><title>WACT² DRC</title></head><body><h1>Settings</h1><form action='/settings'>Speed:<br><input type='text' name='speed' value='%i'><br><br>Steering Offset:<br><input type='text' name='steering' value='%d'><br><input type='submit' value='Submit'></form><form action='/'><br><br><input type='submit' value='<- BACK'></form></body></html>", robot->max_speed, robot->steering_correction); 

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

    const char* resp_str = "<!DOCTYPE html><html><head><title>WACT² DRC</title></head><body><h1 style='font-size: 50px; text-align: center;'>Home</h1><form style='font-align:center;' action='/'><input type='hidden' name='start' value='1'><input style='background: #48A9A6; color: white; border-style: solid; border-color: #48A9A6; height: 20%; width: 100%; font: bold 50px arial, sans-serif; text-shadow:none;' type='submit' value='START'></form><br><br><form action='/'><input type='hidden' name='start' value='2'><input style='background: #D62839; color: white; border-style: solid; border-color: #D62839; height: 20%; width: 100%; font: bold 50px arial, sans-serif; text-shadow:none;' type='submit' value='PAUSE'><br><br><form action='/'><input type='hidden' name='start' value='0'><input style='background: #D62839; color: white; border-style: solid; border-color: #D62839; height: 20%; width: 100%; font: bold 50px arial, sans-serif; text-shadow:none;' type='submit' value='STOP'></form><br><br><form action='/settings'><input style='background: #175676; color: white; border-style: solid; border-color: #175676; height: 20%; width: 100%; font: bold 50px arial, sans-serif; text-shadow:none;' type='submit' value='SETTINGS'></form><br><br><form action='/'><input type='hidden' name='start' value='3'><input style='background: #D62839; color: white; border-style: solid; border-color: #D62839; height: 20%; width: 100%; font: bold 50px arial, sans-serif; text-shadow:none;' type='submit' value='CALIB ESC'><br><br><form action='/term'><input style='background: #885053; color: white; border-style: solid; border-color: #885053; height: 20%; width: 100%; font: bold 50px arial, sans-serif; text-shadow:none;' type='submit' value='TERMINAL'></form></body></html>";
    
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
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &settings);
        httpd_register_uri_handler(server, &terminal);
        httpd_register_uri_handler(server, &uri_home);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
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

void wifi_init_softap()
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

// ==================================================================================================
//          Start http server for wifi config setting
// ==================================================================================================
void start_http_server(robot_t * robot_)
{
    robot = robot_;

    wifi_init_softap();
    start_webserver();
}