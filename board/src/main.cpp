#ifndef PROJECTIO_TESTING

#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "battery.hpp"
#include "secrets.h"
#include "utils.h"
#include "wifi.h"
#include <chrono>
#include <cstring>

namespace BatteryMonitor
{

namespace
{

constexpr auto TAG{"Main"};

using namespace std::literals::chrono_literals;

constexpr auto TIME_IN_DEEP_SLEEP{30min};
constexpr auto TIME_UNTIL_DEEP_SLEEP{10s};

// Read and upload battery data to Home Assistant.
void upload_battery_task(void *args)
{
        while (1) {
                auto const battery_entity = create_battery_entity();

                ESP_LOGI(TAG, "Waiting for Wi-Fi to upload battery...");
                if (Wifi::wait_wifi(Utils::to_ticks(5s))) {
                        ESP_LOGI(TAG, "Uploading battery to %s", Secrets::HA_URL.data());
                        battery_entity->post();
                        ESP_LOGI(TAG, "Battery upload attempted.");
                } else {
                        ESP_LOGI(TAG, "upload_battery timed out. Retrying...");
                }
                battery_entity->print();
                vTaskDelay(Utils::to_ticks(1s));
        }
}

// Start Wi-Fi state machine.
void start_wifi_task(void *args)
{
        while (1) {
                Wifi::wifi_init_station();
                vTaskSuspend(NULL);
        }
}

// Turn off peripherals and enters deep sleep
[[noreturn]] void start_deep_sleep()
{
        ESP_LOGI(TAG, "Shutting down peripherals.");
        ESP_LOGI(TAG, "Stopping Wi-Fi.");
        Wifi::stop_wifi();

        ESP_LOGI(TAG, "Entering Deep Sleep");
        esp_deep_sleep_start();
}

} // namespace

extern "C" {
void app_main(void)
{
        // Setup Home Assistant info.
        {
                static_assert(!Secrets::LONG_LIVED_ACCESS_TOKEN.empty());
                static_assert(!Secrets::HA_URL.empty());
                static_assert(Secrets::HA_URL.back() != '/', "HA URL must not have a leading slash.");
                esphalib::api::set_ha_url(Secrets::HA_URL.data());
                esphalib::api::set_long_lived_access_token(Secrets::LONG_LIVED_ACCESS_TOKEN.data());
        }

        // Main.
        {
                ESP_LOGI(TAG, "Enabling sleep timer wakeup: %d minutes until wakeup. \n",
                         static_cast<int>(TIME_IN_DEEP_SLEEP.count()));
                ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_timer_wakeup(
                    std::chrono::duration_cast<std::chrono::microseconds>(TIME_IN_DEEP_SLEEP).count()));

                // Connect to Wi-Fi.
                TaskHandle_t wifi_task_handle = nullptr;
                xTaskCreate(start_wifi_task, "start wifi task", 4096, nullptr, 5, &wifi_task_handle);

                // Upload sensor data.
                TaskHandle_t upload_battery_task_handle = nullptr;
                xTaskCreate(upload_battery_task, "upload battery task", 4096, nullptr, 1, &upload_battery_task_handle);

                // Go to deep sleep after a set amount of time.
                vTaskDelay(Utils::to_ticks(TIME_UNTIL_DEEP_SLEEP));
                ESP_LOGI(TAG, "Deep Sleep timeout reached");

                // NOTE: Tasks don't need to be cleaned up because deep sleep will reset the device and clear RAM.
                start_deep_sleep();
        }
}
}

} // namespace BatteryMonitor

#endif
