#include "esp_ha_lib.hpp"
#include "secrets.h"
#include "wifi.h"
#include <gtest/gtest.h>

namespace BatteryMonitor
{

namespace Testing
{

namespace
{

constexpr const char *TAG{"TESTING"};

} // namespace

TEST(BatteryMonitorTest, DefaultTest) { EXPECT_TRUE(true); }

void init_gtest()
{
        ::testing::InitGoogleTest();
        if (RUN_ALL_TESTS()) {
        };
}

extern "C" {
int app_main(int argc, char **argv)
{

        // Setup Home Assistant info.
        {
                static_assert(!Secrets::LONG_LIVED_ACCESS_TOKEN.empty());
                static_assert(!Secrets::HA_URL.empty());
                static_assert(Secrets::HA_URL.back() != '/', "HA URL must not have a leading slash.");
                esphalib::api::set_ha_url(Secrets::HA_URL.data());
                esphalib::api::set_long_lived_access_token(Secrets::LONG_LIVED_ACCESS_TOKEN.data());
        }

        // Setup WiFi
        {
                Wifi::wifi_init_station();
        }

        init_gtest();
        // Always return zero-code and allow PlatformIO to parse results.
        return 0;
}
}

} // namespace Testing
} // namespace BatteryMonitor
