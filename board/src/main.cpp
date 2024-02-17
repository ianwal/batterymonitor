#ifndef PROJECTIO_TESTING

extern "C" {
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
}

#include <chrono>
#include "battery.hpp"
#include "esp_ha_lib.hpp"
#include "nvs_control.hpp"
#include "secrets.h"
#include "utils.h"
#include "wifi.h"
#include <cstring>

namespace BatteryMonitor
{

namespace
{

constexpr auto TAG{"Main"};
constexpr std::chrono::minutes TIME_IN_DEEP_SLEEP{1};
constexpr std::chrono::seconds TIME_UNTIL_DEEP_SLEEP{10};

// Read and upload battery data to Home Assistant.
void upload_battery_task(void *args)
{
        while (1) {
                auto const battery_entity = create_battery_entity();

                ESP_LOGI(TAG, "Waiting for Wi-Fi to upload battery...");
                if (Wifi::wait_wifi_connected(Utils::to_ticks(std::chrono::seconds{5}))) {
                        ESP_LOGI(TAG, "Uploading battery to %s", Secrets::HA_URL.data());
                        battery_entity->post();
                        ESP_LOGI(TAG, "Battery upload attempted.");
                } else {
                        ESP_LOGI(TAG, "upload_battery timed out. Retrying...");
                }
                battery_entity->print();
                vTaskDelay(Utils::to_ticks(std::chrono::seconds{1}));
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
void start_deep_sleep()
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
        // Init non-volatile storage (for Wi-Fi).
        {
            auto const is_nvs_init_success = Nvs::init_nvs();
            if (!is_nvs_init_success) {
                    ESP_LOGI(TAG, "Entering Deep Sleep due to NVS init failure. Monitor has failed.");
                    esp_deep_sleep_start();
            }
        }

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
            ESP_LOGI(TAG, "Enabling sleep timer wakeup: %lld minutes until wakeup. \n", TIME_IN_DEEP_SLEEP.count());
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_timer_wakeup(std::chrono::duration_cast<std::chrono::microseconds>(TIME_IN_DEEP_SLEEP).count()));

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
