#ifndef PROJECTIO_TESTING
#include <stdbool.h>
#include <string.h>

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"

#include "battery.h"
#include "esp_ha_lib.h"
#include "secrets.h"
#include "wifi.h"

static const char *TAG = "Main";
static const uint64_t WAKEUP_TIME_SEC = 1800;
static const uint32_t HARD_SLEEP_TIMEOUT_SEC = 10;

HAEntity *battery_entity = NULL;

void init_nvs()
{
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
}

// Read and upload battery.
static void upload_battery_task(void *args)
{
        while (1) {
                battery_entity = get_battery_entity();
                if (!battery_entity) {
                        ESP_LOGE(TAG, "Failed to get battery entity.");
                        vTaskDelay(pdMS_TO_TICKS(5000));
                        continue;
                }

                HAEntity_print(battery_entity);
                ESP_LOGI(TAG, "Waiting for Wi-Fi to upload battery...");
                if (wait_wifi_connected(pdMS_TO_TICKS(2000))) {
                        ESP_LOGI(TAG, "Uploading battery to %s", HA_URL);
                        post_entity(battery_entity);
                        ESP_LOGI(TAG, "Battery upload attempted.");
                } else {
                        ESP_LOGI(TAG, "upload_battery timed out. Retrying...");
                }
                HAEntity_delete(battery_entity);
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

void app_main(void)
{
        init_nvs();

        set_ha_url(HA_URL);
        set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);

        ESP_LOGI(TAG, "Enabling timer wakeup, %llds\n", WAKEUP_TIME_SEC);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(WAKEUP_TIME_SEC * 1000000));

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
#endif