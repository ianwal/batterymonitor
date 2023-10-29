#pragma once

#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stdint.h>

bool wait_wifi_connected(TickType_t timeout);
bool wait_wifi(TickType_t timeout);
bool wifi_init_sta(void);
void stop_wifi();