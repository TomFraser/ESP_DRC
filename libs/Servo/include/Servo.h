/*
*       Servo.h
*/

#ifndef Servo_H_
#define Servo_h_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_system.h"
#include "driver/mcpwm.h"

#include "../../ESP_common.h"

// ========================================
//      Definitions
// ========================================

#define SERVO_MIN_PULSEWIDTH 1000
#define SERVO_MAX_PULSEWIDTH 2000



// ========================================
//      Public Function Definitions
// ========================================

/*
        @Task:      Initialises Timer and LEDC lib for Servos
*/
void servo_init();

/*
        @Task:      Sets ESC / Steering servos to those set in a control struct
*/
void servo_update(control_t * control);

#endif