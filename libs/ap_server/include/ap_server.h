/*
 *      ap_server.h
 */

#ifndef AP_SERVER_H_
#define AP_SERVER_H_

#include "../../ESP_common.h"

extern nvs_handle nvs_data_handle;

void start_http_server(robot_t * robot_);

#endif