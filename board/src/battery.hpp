#pragma once

#include "esp_ha_lib.hpp"

float get_battery_voltage();
float battery_entity_value(HAEntity const &entity);
HAEntity *create_battery_entity();
