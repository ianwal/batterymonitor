#include <assert.h>
#include <math.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"

#include "secrets.h"
#include "battery.h"
#include "wifi.h"
#include "esp_ha_lib.h"

#include <unity.h>

static const char *TAG = "TESTING";

#define MAX_HTTP_OUTPUT_BUFFER 1024

void test_sensor_upload(void){
    // entity name cannot contain special characters other than _ or . it appears
    char* entity_id = "sensor.esphalibtest";
    char* friendly_entity_name = "esp ha lib test";
    char* unit_of_measurement = "Test Units";
    float state = get_battery_voltage();
    
    HAEntity* entity = HAEntity_create();
    entity->state = malloc(8);
    snprintf(entity->state, 8, "%.2f", state);
    strcpy(entity->entity_id, entity_id);
    add_entity_attribute("friendly_name", friendly_entity_name, entity);
    add_entity_attribute("unit_of_measurement", unit_of_measurement, entity);

    post_entity(entity);

    HAEntity* newEntity = get_entity(entity_id);
    float fstate = strtof(newEntity->state, NULL);   
    
    ESP_LOGI(TAG, "Uploaded: %f, Received %f", state, fstate); 
    
    // HA only stores floats to 2 decimal places it seems
    float epsilon = 1e-2;

    HAEntity_Delete(entity);
    HAEntity_Delete(newEntity);
    TEST_ASSERT_FLOAT_WITHIN(epsilon, state, fstate); 
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_upload);
    return UNITY_END();
}

void app_main(void) {
    wifi_init_sta();
    runUnityTests();
}