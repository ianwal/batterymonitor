extern "C" {
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "wifi.h"
#include <assert.h>
#include <unity.h>
}

#include "battery.hpp"
#include "esp_ha_lib.hpp"
#include "nvs_control.hpp"
#include "secrets.h"

#include <cmath>

static constexpr const char *TAG{"TESTING"};

// HA only stores floats to 2 decimal places
static constexpr float epsilon{1e-2};
static constexpr int MAX_HTTP_OUTPUT_BUFFER{1024};

void test_sensor_upload(void)
{
        HAEntity *entity{create_battery_entity()};

        entity->entity_id = "sensor.test_battery_monitor";
        entity->state = std::to_string(esp_random() % 100);
        TEST_ASSERT_NOT_NULL(entity);
        entity->post();

        HAEntity *newEntity{HAEntity::get(entity->entity_id)};
        TEST_ASSERT_NOT_NULL(newEntity);

        float const state{battery_entity_value(*entity)};

        float const fstate{battery_entity_value(*newEntity)};
        ESP_LOGI(TAG, "Uploaded: %f, Received %f", state, fstate);

        TEST_ASSERT_FLOAT_WITHIN(epsilon, state, fstate);
        delete newEntity;
        delete entity;
}

int runUnityTests(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_sensor_upload);
        return UNITY_END();
}

extern "C" {
void app_main(void)
{
        set_ha_url(HA_URL);
        set_long_lived_access_token(LONG_LIVED_ACCESS_TOKEN);
        if (!init_nvs()) {
                // Don't continue if NVS init failed because Wi-Fi won't work either
                while (1) {
                }
        }
        wifi_init_sta();

        runUnityTests();
}
}
