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

#if SOC_RTC_FAST_MEM_SUPPORTED
static RTC_DATA_ATTR struct timeval sleep_enter_time;
#else
static struct timeval sleep_enter_time;
#endif

static const char *TAG = "Main";

void vReadAndUploadVoltage() {
    set_ha_url(HA_URL);
    set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);
    
    HAEntity* entity = HAEntity_create();
    entity->state = malloc(8);
    snprintf(entity->state, 8, "%.2f", get_battery_voltage());
    strcpy(entity->entity_id, "sensor.car_battery");

    add_entity_attribute("friendly_name", "Car Battery Voltage", entity);
    add_entity_attribute("unit_of_measurement", "Volts", entity);
    post_entity(entity);
    //HAEntity_print(entity);
    HAEntity_delete(entity); 
}

static void deep_sleep_task(void *args)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

    ESP_LOGI(TAG, "Wake up from timer. Time spent in deep sleep: %dms\n", sleep_time_ms);

    wifi_init_sta();
    vReadAndUploadVoltage();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Entering deep sleep\n");

    // get deep sleep enter time
    gettimeofday(&sleep_enter_time, NULL);
    esp_deep_sleep_start();
}

void app_main(void)
{
    const uint64_t wakeup_time_sec = 60;
    ESP_LOGI(TAG, "Enabling timer wakeup, %llds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

    xTaskCreate(deep_sleep_task, "deep_sleep_task", 4096, NULL, 6, NULL);
}
#endif