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

#include "protocol_examples_common.h"
#include "sdkconfig.h"
#include "esp_eth.h"

#define ssid "WACT^2 Router"
#define pass "YeetYeet"

#define GOT_IPV4_BIT BIT(0)
#define GOT_IPV6_BIT BIT(1)

#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
#define CONNECTED_BITS (GOT_IPV4_BIT | GOT_IPV6_BIT)
#else
#define CONNECTED_BITS (GOT_IPV4_BIT)
#endif

static EventGroupHandle_t s_connect_event_group;
static ip4_addr_t s_ip_addr;
static const char *s_connection_name;

#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
static ip6_addr_t s_ipv6_addr;
#endif

static const char *TAG = "http_server";

static EventGroupHandle_t wifi_event_group;

static robot_t * robot;

/* set up connection, Wi-Fi or Ethernet */
static void start();

/* tear down connection, release resources */
static void stop();

static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    xEventGroupSetBits(s_connect_event_group, GOT_IPV4_BIT);
}

#ifdef CONFIG_EXAMPLE_CONNECT_IPV6

static void on_got_ipv6(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    memcpy(&s_ipv6_addr, &event->ip6_info.ip, sizeof(s_ipv6_addr));
    xEventGroupSetBits(s_connect_event_group, GOT_IPV6_BIT);
}

#endif // CONFIG_EXAMPLE_CONNECT_IPV6

esp_err_t example_connect()
{
    if (s_connect_event_group != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_connect_event_group = xEventGroupCreate();
    start();
    xEventGroupWaitBits(s_connect_event_group, CONNECTED_BITS, true, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to %s", s_connection_name);
    ESP_LOGI(TAG, "IPv4 address: " IPSTR, IP2STR(&s_ip_addr));
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_LOGI(TAG, "IPv6 address: " IPV6STR, IPV62STR(s_ipv6_addr));
#endif
    return ESP_OK;
}

esp_err_t example_disconnect()
{
    if (s_connect_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    vEventGroupDelete(s_connect_event_group);
    s_connect_event_group = NULL;
    stop();
    ESP_LOGI(TAG, "Disconnected from %s", s_connection_name);
    s_connection_name = NULL;
    return ESP_OK;
}

#ifdef CONFIG_EXAMPLE_CONNECT_WIFI

    static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
    {   
        ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
        ESP_ERROR_CHECK(esp_wifi_connect());
    }   

#ifdef CONFIG_EXAMPLE_CONNECT_IPV6

    static void on_wifi_connect(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
    {
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
    }

#endif // CONFIG_EXAMPLE_CONNECT_IPV6

static void start()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6, NULL));
#endif

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ssid,
            .password = pass,
        },
    };
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    s_connection_name = CONFIG_EXAMPLE_WIFI_SSID;
}

static void stop()
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect));
#endif
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET

#ifdef CONFIG_EXAMPLE_CONNECT_IPV6

/** Event handler for Ethernet events */
static void on_eth_event(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data)
{
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_ETH);
        break;
    default:
        break;
    }
}

#endif // CONFIG_EXAMPLE_CONNECT_IPV6

static esp_eth_handle_t s_eth_handle = NULL;
static esp_eth_mac_t *s_mac = NULL;
static esp_eth_phy_t *s_phy = NULL;

static void start()
{
    ESP_ERROR_CHECK(tcpip_adapter_set_default_eth_handlers());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_got_ip, NULL));
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED, &on_eth_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6, NULL));
#endif
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    s_mac = esp_eth_mac_new_esp32(&mac_config);
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
#if CONFIG_EXAMPLE_ETH_PHY_IP101
    s_phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_RTL8201
    s_phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_LAN8720
    s_phy = esp_eth_phy_new_lan8720(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_DP83848
    s_phy = esp_eth_phy_new_dp83848(&phy_config);
#endif
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(s_mac, s_phy);

    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &s_eth_handle));
    s_connection_name = "Ethernet";
}

static void stop()
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_got_ip));
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6));
    ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ETHERNET_EVENT_CONNECTED, &on_eth_event));
#endif
    ESP_ERROR_CHECK(esp_eth_driver_uninstall(s_eth_handle));
    ESP_ERROR_CHECK(s_phy->del(s_phy));
    ESP_ERROR_CHECK(s_mac->del(s_mac));
}

#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

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
            changed_value = atoi(value);
            if (changed_value == 3) {
                servo_init(robot);
            }
            else {
                robot->stop = changed_value;
            }
            
            robot->stop = changed_value;
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

// ==================================================================================================
//          Start http server for wifi config setting
// ==================================================================================================
void start_http_server(robot_t * robot_)
{
    robot = robot_;

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
    #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_WIFI
    #ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    server = start_webserver();
}
