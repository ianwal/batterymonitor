#ifndef PROJECTIO_TESTING

extern "C" {
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "secrets.h"
#include "wifi.h"
}

#include "battery.hpp"
#include "esp_ha_lib.hpp"
#include "nvs_control.hpp"
#include <cstring>

static constexpr const char *TAG{"Main"};
static constexpr const uint64_t WAKEUP_TIME_SEC{60 * 30};   // 60 sec * 30 min
static constexpr const uint32_t HARD_SLEEP_TIMEOUT_SEC{10}; // time until deep sleep

// Read and upload battery data to HA
static void upload_battery_task(void *args)
{
        HAEntity *battery_entity;
        while (1) {
                battery_entity = create_battery_entity();
                if (!battery_entity) {
                        vTaskDelay(pdMS_TO_TICKS(5000));
                        continue;
                }

                battery_entity->print();
                ESP_LOGI(TAG, "Waiting for Wi-Fi to upload battery...");
                if (wait_wifi_connected(pdMS_TO_TICKS(5000))) {
                        ESP_LOGI(TAG, "Uploading battery to %s", HA_URL);
                        battery_entity->post();
                        ESP_LOGI(TAG, "Battery upload attempted.");
                } else {
                        ESP_LOGI(TAG, "upload_battery timed out. Retrying...");
                }
                delete battery_entity;
                battery_entity = nullptr;
                vTaskDelay(pdMS_TO_TICKS(1000));
        }
}

// Turn off peripherals and enters deep sleep
static void start_deep_sleep()
{
        ESP_LOGI(TAG, "Shutting down peripherals.");
        ESP_LOGI(TAG, "Stopping Wi-Fi.");
        stop_wifi();

        ESP_LOGI(TAG, "Entering Deep Sleep");
        esp_deep_sleep_start();
}

// Connects to Wi-Fi and suspends
static void start_wifi_task(void *args)
{
        while (1) {
                wifi_init_sta();
                vTaskSuspend(NULL);
        }
}

extern "C" {
void app_main(void)
{
        bool const ret_nvs_init{init_nvs()};
        if (!ret_nvs_init) {
                ESP_LOGI(TAG, "Entering Deep Sleep due to NVS init failure. Monitor has failed.");
                esp_deep_sleep_start();
        }

        set_ha_url(HA_URL);
        set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);

        ESP_LOGI(TAG, "Enabling timer wakeup, %llds\n", WAKEUP_TIME_SEC);
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_timer_wakeup(WAKEUP_TIME_SEC * 1000000));

        // Connect to Wi-Fi
        TaskHandle_t wifi_task_handle = NULL;
        xTaskCreate(start_wifi_task, "start wifi task", 4096, NULL, 5, &wifi_task_handle);

        // Upload battery
        TaskHandle_t upload_battery_task_handle = NULL;
        xTaskCreate(upload_battery_task, "upload battery task", 8192, NULL, 1, &upload_battery_task_handle);

        // Calls deep sleep after a set amount of time
        vTaskDelay(pdMS_TO_TICKS(HARD_SLEEP_TIMEOUT_SEC * 1000));
        ESP_LOGI(TAG, "Deep Sleep timeout reached");

        // Tasks don't need to be cleaned up because
        // deep sleep will reset the device and clear RAM
        start_deep_sleep();
}
}
#endif