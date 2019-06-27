/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#define DEBUG false

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <nvs_flash.h>
#include "driver/uart.h"
#include <string.h>

#include "Serial.h"
#include "SerialMsg.h"
#include "Servo.h"
#include "ap_server.h"

#define ESP_PWR_CTRL 19
#define ESP_PWR_STACK 1024
#define ESP_PWR_PRIO 1

#define ESP_PWR_BTN 21

#define ESP_LED0 5
#define ESP_LED1 18

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<ESP_LED0) | (1ULL<<ESP_LED1) | (1ULL<<ESP_PWR_CTRL))
#define GPIO_INPUT_PIN_SEL ((1ULL<<ESP_PWR_BTN))

static void IRAM_ATTR powerBtnHandler(void* arg) {
    ESP_LOGE("PWR", "BTN PRESSED");
}

void app_main()
{
    nvs_flash_init();

    /* Remove those pesky GPIO log messages */
    esp_log_level_set("*", ESP_LOG_WARN);

    ESP_LOGW("INFO", "Main Started\n");
    
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;  
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    gpio_set_intr_type(ESP_PWR_BTN, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(ESP_PWR_BTN, powerBtnHandler, (void *)ESP_PWR_BTN);

    gpio_set_level(ESP_PWR_CTRL, 1);

    robot_t * robot = (robot_t *) malloc(sizeof(robot_t));
        robot->stop = 0;
        robot->max_speed = 10;

    start_http_server(robot);

    servo_init(robot);

    robot->stop = 0;

    /* Setup Task for Button */
    // TaskHandle_t handle = NULL;
    // static uint8_t pwr_par;
    // xTaskCreate(espPowerButton, "PWR_BTN", ESP_PWR_STACK, &pwr_par, ESP_PWR_PRIO, &handle);
    // configASSERT(handle);

    xTaskCreate((TaskFunction_t) uart_rx_task, "UART Task", 2048, NULL, configMAX_PRIORITIES, NULL);

    for(;;) {
        gpio_set_level(ESP_LED0, 0);
        gpio_set_level(ESP_LED1, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(ESP_LED0, 1);
        gpio_set_level(ESP_LED1, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    free(robot);
    // gpio_set_level(ESP_PWR_CTRL, 0);
}

