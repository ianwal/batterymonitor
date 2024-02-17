#include <string_view>

namespace BatteryMonitor
{

namespace Secrets
{

inline constexpr std::string_view LONG_LIVED_ACCESS_TOKEN{
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJpc3MiOiIxNDI4MDFmZWJmY2U0MTI0YTkzMTFlMmVjMjZkN2ZhYSIsImlhdCI6MTcwODA2NjY3MiwiZXhwIjoyMDIzNDI2NjcyfQ.0-"
    "whVd4GJpr28A06FmxQ25VIpO34ydyf0atmEy33FEw"};
// must be of the form "http://<ha.local>" with no leading slash
inline constexpr std::string_view HA_URL{"http://hassio.local:8123"};
inline constexpr std::string_view NETWORK_SSID{"changeme"};
inline constexpr std::string_view NETWORK_PASSWORD{"password"};

} // namespace Secrets
} // namespace BatteryMonitor
