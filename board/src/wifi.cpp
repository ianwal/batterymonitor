#include "wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "secrets.h"
#include "utils.h"
#include <cstddef>
#include <cstring>
#include <string>

namespace BatteryMonitor
{

namespace Wifi
{

namespace
{
constexpr auto TAG{"Wi-Fi"};

constexpr auto ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD = WIFI_AUTH_WPA_WPA2_PSK;

// The event group allows multiple bits for each event, but we only care about two events:
// - Connected to the AP with an IP
// - Failed to connect after the maximum amount of retries
constexpr EventBits_t WIFI_CONNECTED_BIT{BIT0};
constexpr EventBits_t WIFI_DISCONNECTED_BIT{BIT1};
constexpr EventBits_t WIFI_STOPPED_BIT{BIT2};

EventGroupHandle_t s_wifi_event_group;

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
        static int32_t s_retry_num{0};
        if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START)) {
                esp_wifi_connect();
        } else if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED)) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
                constexpr int32_t max_retries{10};
                if (s_retry_num < max_retries) {
                        esp_wifi_connect();
                        ++s_retry_num;
                        ESP_LOGI(TAG, "Retrying connection to AP");
                }
                ESP_LOGE(TAG, "Connection to AP failed. SSID: %s", Secrets::NETWORK_SSID.data());
        } else if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                auto const event_ptr = static_cast<ip_event_got_ip_t *>(event_data);
                ESP_LOGI(TAG, "IP Obtained - " IPSTR, IP2STR(&event_ptr->ip_info.ip));
                s_retry_num = 0;
        } else if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_STOP)) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_STOPPED_BIT);
        } else {
                ESP_LOGI(TAG, "Event not handled in event_handler.");
        }
}

} // namespace

bool wait_wifi(TickType_t timeout)
{
        // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
        // number of re-tries (WIFI_DISCONNECTED_BIT). The bits are set by event_handler() (see above)
        auto const bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT, pdFALSE,
                                              pdFALSE, timeout);
        // xEventGroupWaitBits() returns the bits before the call
        // returned, hence we can test which event actually happened
        return Utils::are_bits_set(bits, WIFI_CONNECTED_BIT);
}

void stop_wifi()
{
        // ESP_ERROR_CHECK_WITHOUT_ABORT(
        //     esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
        //     static_cast<esp_event_handler_instance_t>(&event_handler)));
        // ESP_ERROR_CHECK_WITHOUT_ABORT(
        //     esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
        esp_wifi_stop();
        auto const bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_STOPPED_BIT, pdFALSE, pdTRUE,
                                              Utils::to_ticks(std::chrono::seconds{1}));
        auto const is_wifi_stopped = Utils::are_bits_set(bits, WIFI_STOPPED_BIT);
        if (is_wifi_stopped) {
                vEventGroupDelete(s_wifi_event_group);
                ESP_LOGI(TAG, "WiFi stopped.");
        } else {
                ESP_LOGE(TAG, "WiFi did not stop within the timeout period.");
        }
}

// NOTE: NVS must be init before this can be run
void wifi_init_station()
{
        s_wifi_event_group = xEventGroupCreate();
        xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(wifi_storage_t::WIFI_STORAGE_RAM));

        esp_event_handler_instance_t instance_any_id{nullptr};
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr,
                                                            &instance_any_id));
        esp_event_handler_instance_t instance_got_ip{nullptr};
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ip_event_t::IP_EVENT_STA_GOT_IP, &event_handler,
                                                            nullptr, &instance_got_ip));

        wifi_sta_config_t wifi_config{};
        // Ensure these are filled in. No point in running the program if they aren't.
        static_assert(!Secrets::NETWORK_SSID.empty());
        static_assert(!Secrets::NETWORK_PASSWORD.empty());
        // Don't overrun the SSID and password buffers.
        static_assert(Secrets::NETWORK_SSID.size() <= sizeof(wifi_sta_config_t::ssid));
        static_assert(Secrets::NETWORK_PASSWORD.size() <= sizeof(wifi_sta_config_t::password));

        std::memcpy(wifi_config.ssid, Secrets::NETWORK_SSID.data(), Secrets::NETWORK_SSID.size());
        std::memcpy(wifi_config.password, Secrets::NETWORK_PASSWORD.data(), Secrets::NETWORK_PASSWORD.size());
        wifi_config.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config_t u_wifi_config;
        u_wifi_config.sta = wifi_config;
        ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode_t::WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(wifi_interface_t::WIFI_IF_STA, &u_wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
}

} // namespace Wifi

} // namespace BatteryMonitor
