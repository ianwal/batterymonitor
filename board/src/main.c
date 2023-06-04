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

void upload_battery() {
    set_ha_url(HA_URL);
    set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);
 
    HAEntity* entity = HAEntity_create();
    entity->state = malloc(8);
    snprintf(entity->state, 8, "%.2f", get_battery_voltage());
    strcpy(entity->entity_id, "sensor.car_battery");

    add_entity_attribute("friendly_name", "Car Battery Voltage", entity);
    add_entity_attribute("unit_of_measurement", "Volts", entity);

    ESP_LOGI(TAG, "Uploading battery to %s", HA_URL);
    post_entity(entity);

    HAEntity_print(entity);
    HAEntity_delete(entity); 
}

// Connects to Wi-Fi and uploads battery then enters deep sleep
static void deep_sleep_task(void *args)
{
    wifi_init_sta();
    if(is_wifi_connected()){
        upload_battery();
    }

    ESP_LOGI(TAG, "Entering deep sleep\n");

    // Task does not need to be deleted because deep sleep does not power RAM 
    esp_wifi_stop();
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