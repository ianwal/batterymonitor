#ifndef PROJECTIO_TESTING
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_http_client.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"

#include "secrets.h"
#include "battery.h"
#include "esp_ha_lib.h"
#include "wifi.h"

static const char *TAG = "Main";
static const uint64_t wakeup_time_sec = 1800; // 1800 sec = 30 minutes
static const uint32_t hard_sleep_timeout_sec = 10; // max time awake

// Read and upload battery.
static void upload_battery_task(void *args) {
    while(1) {
        HAEntity* entity = HAEntity_create();
        entity->state = malloc(8);
        snprintf(entity->state, 8, "%.2f", get_battery_voltage());
        strcpy(entity->entity_id, "sensor.car_battery");

        add_entity_attribute("friendly_name", "Car Battery Voltage", entity);
        add_entity_attribute("unit_of_measurement", "Volts", entity);

        HAEntity_print(entity);
        ESP_LOGI(TAG, "Waiting for Wi-Fi to upload battery...");
        if(wait_wifi(portMAX_DELAY)) {
            ESP_LOGI(TAG, "Uploading battery to %s", HA_URL);
            post_entity(entity);
            ESP_LOGI(TAG, "Battery upload attempted.");
        } else {
            ESP_LOGI(TAG, "upload_battery timed out. Retrying...");
        }
        HAEntity_delete(entity);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Turn off peripherals and enters deep sleep
static void start_deep_sleep()
{
    ESP_LOGI(TAG, "Shutting down peripherals.");
    esp_wifi_stop();
    
    ESP_LOGI(TAG, "Entering Deep Sleep");
    esp_deep_sleep_start();
}

// Connects to Wi-Fi and suspends
static void start_wifi_task(void *args)
{
    while(1) {
        wifi_init_sta();
        vTaskSuspend(NULL);
    }
}

void app_main(void)
{
    set_ha_url(HA_URL);
    set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);

    ESP_LOGI(TAG, "Enabling timer wakeup, %llds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

    // Connect to Wi-Fi
    pre_wifi_setup();
    TaskHandle_t wifi_task_handle = NULL;
    xTaskCreate(start_wifi_task, "start wifi task", 4096, NULL, 5, &wifi_task_handle);

    // Upload battery
    TaskHandle_t upload_battery_task_handle = NULL;
    xTaskCreate(upload_battery_task, "upload battery task", 8192, NULL, 1, &upload_battery_task_handle);

    // Calls deep sleep after a set amount of time 
    vTaskDelay(pdMS_TO_TICKS(hard_sleep_timeout_sec * 1000));
    ESP_LOGI(TAG, "Deep Sleep timeout reached");
 
    if(wifi_task_handle) {
        vTaskDelete(wifi_task_handle);
        wifi_task_handle = NULL;
    }
    if(upload_battery_task_handle) {
        vTaskDelete(upload_battery_task_handle);
        upload_battery_task_handle = NULL;
    }
    start_deep_sleep();
}
#endif