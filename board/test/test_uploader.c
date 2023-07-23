#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <assert.h>
#include <math.h>

#include "driver/gpio.h"

#include "battery.h"
#include "esp_ha_lib.h"
#include "secrets.h"
#include "wifi.h"

#include <unity.h>

static const char *TAG = "TESTING";

// HA only stores floats to 2 decimal places
static float epsilon = 1e-2;

#define MAX_HTTP_OUTPUT_BUFFER 1024

void test_sensor_upload(void)
{
        HAEntity *entity = get_battery_entity();
        TEST_ASSERT_NOT_NULL(entity);
        post_entity(entity);

        HAEntity *newEntity = get_entity(entity->entity_id);
        TEST_ASSERT_NOT_NULL(newEntity);
        float state = battery_entity_value(entity);
        float fstate = battery_entity_value(newEntity);

        ESP_LOGI(TAG, "Uploaded: %f, Received %f", state, fstate);

        HAEntity_delete(newEntity);
        HAEntity_delete(entity);
        TEST_ASSERT_FLOAT_WITHIN(epsilon, state, fstate);
}

void init_nvs()
{
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
}

int runUnityTests(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_sensor_upload);
        return UNITY_END();
}

void app_main(void)
{
        set_ha_url(HA_URL);
        set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);
        init_nvs();
        wifi_init_sta();
        runUnityTests();
}