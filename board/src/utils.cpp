#include "utils.h"

namespace BatteryMonitor
{

namespace Utils
{

bool are_bits_set(EventBits_t const bits, EventBits_t const bitmask) { return (bits & bitmask) == bitmask; }

} // namespace Utils

} // namespace BatteryMonitor
