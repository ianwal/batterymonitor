#pragma once

#include "esp_ha_lib.hpp"
#include <memory>

namespace BatteryMonitor
{

using esphalib::state::HAEntity;

float get_battery_voltage();
std::unique_ptr<HAEntity> create_battery_entity();

}
