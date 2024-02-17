#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <chrono>
#include <concepts>

namespace BatteryMonitor
{

namespace Utils
{

template <typename T>
concept Duration = requires { typename std::chrono::duration_values<T>; };

// Convert chrono duration to FreeRTOS ticks.
template <Duration DurationType> constexpr auto to_ticks(DurationType duration) -> TickType_t
{
        auto const milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        auto const raw_milliseconds = milliseconds.count();
        return pdMS_TO_TICKS(raw_milliseconds);
}

// Check if all bits, and only those bits, are in the bitmask are set in bits.
bool are_bits_set(EventBits_t bits, EventBits_t bitmask);

} // namespace Utils

} // namespace BatteryMonitor
