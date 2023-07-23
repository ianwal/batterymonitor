#include "wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "secrets.h"
#include <stdbool.h>
#include <stdint.h>

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

/*
 * The event group allows multiple bits for each event, but we only care about two events:
 * - Connected to the AP with an IP
 * - Failed to connect after the maximum amount of retries
 */

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define WIFI_STOPPED_BIT BIT2

static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 0;
static const int MAXIMUM_RETRY = 10;

static const char *TAG = "WiFi";

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
                esp_wifi_connect();
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
                if (s_retry_num < MAXIMUM_RETRY) {
                        esp_wifi_connect();
                        s_retry_num++;
                        ESP_LOGI(TAG, "Retrying connection to AP");
                }
                ESP_LOGI(TAG, "Connection to AP failed.");
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "IP Obtained - " IPSTR, IP2STR(&event->ip_info.ip));
                s_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_STOPPED_BIT);
        }
}

bool wait_wifi_connected(TickType_t timeout)
{
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, timeout);

        if (bits & WIFI_CONNECTED_BIT)
                return true;

        return false;
}

bool wait_wifi(TickType_t timeout)
{
        // Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
        // number of re-tries (WIFI_DISCONNECTED_BIT). The bits are set by event_handler() (see above)
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT, pdFALSE,
                                               pdFALSE, timeout);

        // xEventGroupWaitBits() returns the bits before the call
        // returned, hence we can test which event actually happened
        if (bits & WIFI_CONNECTED_BIT)
                return true;

        return false;
}

void stop_wifi()
{
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
        esp_wifi_stop();
        EventBits_t bits =
            xEventGroupWaitBits(s_wifi_event_group, WIFI_STOPPED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(1000));
        if (bits & WIFI_STOPPED_BIT) {
                ESP_LOGI(TAG, "WiFi stopped.");
        } else {
                ESP_LOGE(TAG, "WiFi did not stop within the timeout period.");
                return;
        }
        vEventGroupDelete(s_wifi_event_group);
}

void wifi_init_sta(void)
{
        ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

        s_wifi_event_group = xEventGroupCreate();
        xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(
            esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
        ESP_ERROR_CHECK(
            esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

        wifi_config_t wifi_config = {
            .sta = {.ssid = NETWORK_SSID, .password = NETWORK_PASSWORD, .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK},
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_sta finished.");

        // Print if WiFi is connected currently
        bool connected = wait_wifi(portMAX_DELAY);
        if (connected) {
                ESP_LOGI(TAG, "Connected to AP SSID: '%s'", NETWORK_SSID);
        } else {
                ESP_LOGI(TAG, "Failed to connect to SSID: '%s'", NETWORK_SSID);
        }
}
