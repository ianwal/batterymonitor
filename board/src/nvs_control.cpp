extern "C" {
#include "esp_log.h"
#include "nvs_flash.h"
}

static constexpr const char *TAG{"NVS Control"};

bool init_nvs()
{
        bool success{false};
        esp_err_t ret_flash_init{nvs_flash_init()};
        if (ret_flash_init == ESP_ERR_NVS_NO_FREE_PAGES || ret_flash_init == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                esp_err_t const ret_flash_erase{nvs_flash_erase()};
                ESP_ERROR_CHECK_WITHOUT_ABORT(ret_flash_erase);
                if (ret_flash_erase == ESP_OK) {
                        ret_flash_init = nvs_flash_init();
                        ESP_ERROR_CHECK_WITHOUT_ABORT(ret_flash_init);
                        success = (ret_flash_init == ESP_OK);
                } else {
                        success = false;
                }
        } else {
                success = true;
        }

        if (success) {
                ESP_LOGI(TAG, "NVS init successful.");
        } else {
                ESP_LOGI(TAG, "NVS init FAILURE.");
        }
        return success;
}