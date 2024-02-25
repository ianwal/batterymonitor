#pragma once

#include "freertos/FreeRTOS.h"
#include <cstddef>
namespace BatteryMonitor
{

namespace Wifi
{

bool wait_wifi(TickType_t timeout);
void wifi_init_station(void);
void stop_wifi(void);

} // namespace Wifi

} // namespace BatteryMonitor
