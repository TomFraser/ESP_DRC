/*
*       ESP_common.h
*/
#ifndef Common_h_
#define Common_h_

#include <stdio.h>
#include <stdint.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "esp_system.h"

// ========================================
//      Definitions
// ========================================

#define ESC_PIN         26
#define STEERING_PIN    25

#define UART_RX         16
#define UART_TX         17

#define LED0            5
#define LED1            18
#define PWR_KILL        19
#define PWR_INT         21

// ========================================
//      Structs
// ========================================

typedef struct control {

    int8_t steering;
    uint8_t speed;

}   control_t;

typedef struct robot {

    uint8_t stop;
    int8_t max_speed;
    int8_t steering_correction;

}   robot_t;

#endif