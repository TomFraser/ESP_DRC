/*
*       Serial.h
*/

#ifndef Serial_h_
#define Serial_h_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdint.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "soc/uart_struct.h"
#include "SerialMsg.h"
#include "freertos/queue.h"

// ========================================
//      Definitions
// ========================================

#define UART_TAG "UART"

#define UART_RP UART_NUM_1
#define UART_TX 17
#define UART_RX 16

;
extern QueueHandle_t uart_rx_queue;
extern QueueHandle_t uart_tx_queue;



// ========================================
//      Public Function Definitions
// ========================================

// ***** IMPORTANT NOTE: These two tasks are the only two functions to touch UART buffers directly, everything else through Rx / Tx Queues

/*
        @Task:      UART Rx Handler Task
*/
TaskFunction_t uart_rx_task(void);

/*
        @Task:      UART Tx Handler Task
*/
TaskFunction_t uart_tx_task(void);


#endif