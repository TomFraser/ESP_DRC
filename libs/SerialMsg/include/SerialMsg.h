#ifndef SerialMsg_h_
#define SerialMsg_h_

#include <stdio.h>
#include <stdint.h>

typedef enum MsgType {
    DATA = 0,
    POWER_REQUEST,
    POWER_CONFIRM,
    POWER_DENY,
    FORCE_RESET,
    FORCE_STOP,
    LOG_MESSAGE
} MsgType_t;

typedef struct SerialHeader {
    uint8_t Sync1;
    uint8_t Sync2;
    uint8_t Type;
    uint8_t Size;
} SerialHeader_t;

typedef struct QueueMsg {
    uint8_t Type;
    uint8_t Size;
    uint8_t* Data;
} QueueMsg_t;

#endif