extern "C" {
#include "esp_log.h"
#include "nvs_flash.h"
}
#include "nvs_control.hpp"

namespace BatteryMonitor
{

namespace Nvs
{

namespace
{
constexpr const char *TAG{"NVS Control"};
}

bool init_nvs()
{
        auto success = false;
        auto const ret_flash_init = nvs_flash_init();
        if (ret_flash_init == ESP_ERR_NVS_NO_FREE_PAGES || ret_flash_init == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                auto const ret_flash_erase = nvs_flash_erase();
                ESP_ERROR_CHECK_WITHOUT_ABORT(ret_flash_erase);
                if (ret_flash_erase == ESP_OK) {
                        auto const post_erase_ret_flash_init = nvs_flash_init();
                        ESP_ERROR_CHECK_WITHOUT_ABORT(post_erase_ret_flash_init);
                        success = (post_erase_ret_flash_init == ESP_OK);
                } else {
                        success = false;
                }
        } else {
                success = true;
        }

        auto const msg = success ? "NVS init successful." : "NVS init failure.";
        ESP_LOGI(TAG, "%s", msg);
        return success;
}

} // namespace Nvs
} // namespace BatteryMonitor
