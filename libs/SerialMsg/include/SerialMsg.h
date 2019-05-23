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

typedef struct SerialMsg {
    uint8_t Sync1;
    uint8_t Sync2;
    uint8_t Type;
    uint8_t Size;
    void * Data;
} SerialMsg_t;

#endif