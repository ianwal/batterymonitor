#pragma once

#include "esp_ha_lib.h"

float get_battery_voltage();
float battery_entity_value(HAEntity *entity);
HAEntity *get_battery_entity();
