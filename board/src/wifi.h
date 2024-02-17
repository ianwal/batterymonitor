#pragma once

#include "freertos/FreeRTOS.h"
#include <cstddef>
namespace BatteryMonitor
{

namespace Wifi
{

bool wait_wifi_connected(TickType_t timeout);
bool wait_wifi(TickType_t timeout);
bool wifi_init_station(void);
void stop_wifi(void);

} // namespace Wifi

} // namespace BatteryMonitor
