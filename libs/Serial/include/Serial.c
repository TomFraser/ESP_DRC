/*
*       Serial.c
*/

#include "Serial.h"

// ========================================
//      Definitions
// ========================================

QueueHandle_t uart_queue;



// ========================================
//      Private Function Definitions
// ========================================

/*
        @Task:      Uart Initialisation Function
*/
static void uart_init(void);



// ========================================
//      Public Functions
// ========================================
TaskFunction_t uart_rx_task() {
    ESP_LOGI(UART_TAG, "UART Task Created");

    uart_init();

    uart_event_t event;
    size_t* bufSize = (size_t*) malloc(2);
    uint8_t* syncs = (uint8_t*) malloc(2);
    SerialHeader_t header;
    header.Sync1 = 0xB5;
    header.Sync2 = 0x62;

    for(;;) {
        if(xQueueReceive(uart_queue, (void *) &event, (TickType_t) 1)) {
            switch(event.type) {
                case UART_DATA:
                    uart_get_buffered_data_len(UART_RP, bufSize);

                    while(*bufSize > 0) {
                        uart_read_bytes(UART_RP, syncs, 1, portMAX_DELAY);
                        ESP_LOGE("GOT BYTE", "%d", (uint8_t) *syncs);
                        
                        // if (*syncs == header.Sync1) {   // First sync char
                        //     uart_read_bytes(UART_RP, syncs+1, 1, portMAX_DELAY);

                        //     if (*(syncs+1) == header.Sync2) {
                        //         // Read both sync char, at start of UART packet
                        //         uart_read_bytes(UART_RP, ((uint8_t*) (&header)) + 2, 2, portMAX_DELAY);

                        //         QueueMsg_t queueMsg;
                        //         queueMsg.Type = header.Type;
                        //         queueMsg.Size = header.Size;
                        //         queueMsg.Data = (uint8_t*) malloc(header.Size); // To be freed after useage

                        //         uart_read_bytes(UART_RP, queueMsg.Data, queueMsg.Size, 100);

                        //         #if DEBUG
                        //             ESP_LOGI(UART_TAG, "TYPE: %d", queueMsg.Type);
                        //             ESP_LOGI(UART_TAG, "SIZE: %d", queueMsg.Size);
                        //             ESP_LOGI(UART_TAG, "DATA:");
                        //             esp_log_buffer_hex(UART_TAG, queueMsg.Data, queueMsg.Size);
                        //         #endif

                        //         if (queueMsg.Type == DATA) {
                        //             servo_update((control_t *) queueMsg.Data);
                        //             free((control_t *) queueMsg.Data);
                        //         } else if (queueMsg.Type == LOG_MESSAGE) {
                        //             // Put on Debug Queue
                        //         }
                        //     }
                        // }
                        uart_get_buffered_data_len(UART_RP, bufSize);
                    }
                    break;

                default:
                    ESP_LOGI(UART_TAG, "Unhandled UART Event, Type: %d", event.type);
            }
        }
    }

    free(syncs);
    free(bufSize);
    syncs = NULL;
    bufSize = NULL;
    vTaskDelete(NULL);
}



// ========================================
//      Private Functions
// ========================================
static void uart_init(void) {
    const uart_config_t UartConfiguration = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_RP, &UartConfiguration);
    uart_set_pin(UART_RP, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_RP, 1024 * 2, 1024 * 2, 20, &uart_queue, 0);
}