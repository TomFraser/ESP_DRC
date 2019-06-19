#ifndef SerialMsg_h_
#define SerialMsg_h_

#include <stdio.h>
#include <stdint.h>

typedef enum MsgType {
    Message = 0,
    PowerReq,
    PowerConf,
    PowerDeny,
    ForceReset,
    ForceStop
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