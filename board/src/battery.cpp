extern "C" {
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
}

#include "battery.hpp"
#include "esp_ha_lib.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace BatteryMonitor
{

#define BATT_VOLTAGE_ADC_CHANNEL ADC_CHANNEL_3
#define BATT_VOLTAGE_ADC_ATTEN ADC_ATTEN_DB_11
#define BATT_VOLTAGE_ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_UNIT ADC_UNIT_1
constexpr int const ADC_VREF{1100};

// Calibrate the battery voltage measurement
// based on the resistor voltage divider
constexpr float const VDIV_RATIO{5.02f};

static constexpr const char *TAG{"Battery"};

adc_oneshot_unit_handle_t batt_voltage_adc_handle;
adc_cali_handle_t batt_voltage_adc_cali_handle{nullptr};

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_bitwidth_t bitwidth,
                                 adc_cali_handle_t *out_handle)
{
        adc_cali_handle_t handle{nullptr};
        auto ret = static_cast<esp_err_t>(ESP_FAIL);
        bool calibrated{false};

        if (!calibrated) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
                adc_cali_curve_fitting_config_t cali_config;
                cali_config.unit_id = unit;
                cali_config.chan = channel;
                cali_config.atten = atten;
                cali_config.bitwidth = bitwidth;
                ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
                adc_cali_line_fitting_config_t cali_config;
                cali_config.unit_id = unit;
                cali_config.atten = atten;
                cali_config.bitwidth = bitwidth;
                ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
#else
#error "No ADC calibration scheme defined"
#endif
                calibrated = (ret == ESP_OK);
        }

        *out_handle = handle;
        if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Calibration Success");
        } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
                ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
        } else {
                ESP_LOGE(TAG, "Invalid arg or no memory");
        }

        return calibrated;
}

// Configures the battery ADC
static void config_batt_adc()
{
        ESP_LOGI(TAG, "Configuring ADC characteristics");

        // ADC1 Init
        adc_oneshot_unit_init_cfg_t init_config1;
        init_config1.unit_id = ADC_UNIT;
        init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
        init_config1.clk_src = static_cast<adc_oneshot_clk_src_t>(ADC_DIGI_CLK_SRC_DEFAULT);
        ESP_ERROR_CHECK_WITHOUT_ABORT(adc_oneshot_new_unit(&init_config1, &batt_voltage_adc_handle));

        // ADC1 Config
        adc_oneshot_chan_cfg_t const config{BATT_VOLTAGE_ADC_ATTEN, BATT_VOLTAGE_ADC_BITWIDTH};
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            adc_oneshot_config_channel(batt_voltage_adc_handle, BATT_VOLTAGE_ADC_CHANNEL, &config));
}

// Deinitialize calibration of an ADC handle
static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        ESP_ERROR_CHECK_WITHOUT_ABORT(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        ESP_ERROR_CHECK_WITHOUT_ABORT(adc_cali_delete_scheme_line_fitting(handle));
#else
#error "No ADC calibration scheme defined"
#endif
}

// Read ADC voltage from battery and return battery
// voltage calculated from the ADC and voltage divider
float get_battery_voltage()
{
        int raw_adc_read{};
        int voltage{};

        config_batt_adc();

        // ADC1 Read
        ESP_ERROR_CHECK(adc_oneshot_read(batt_voltage_adc_handle, BATT_VOLTAGE_ADC_CHANNEL, &raw_adc_read));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT + 1, BATT_VOLTAGE_ADC_CHANNEL, raw_adc_read);

        // ADC1 Calibration
        bool const do_calibration_batt_voltage{adc_calibration_init(ADC_UNIT, BATT_VOLTAGE_ADC_CHANNEL,
                                                                    BATT_VOLTAGE_ADC_ATTEN, BATT_VOLTAGE_ADC_BITWIDTH,
                                                                    &batt_voltage_adc_cali_handle)};

        if (do_calibration_batt_voltage) {
                ESP_ERROR_CHECK(adc_cali_raw_to_voltage(batt_voltage_adc_cali_handle, raw_adc_read, &voltage));
                ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT + 1, BATT_VOLTAGE_ADC_CHANNEL, voltage);
        }

        // ADC1 Teardown
        ESP_ERROR_CHECK(adc_oneshot_del_unit(batt_voltage_adc_handle));
        if (do_calibration_batt_voltage) {
                adc_calibration_deinit(batt_voltage_adc_cali_handle);
        }

        return (voltage / 1000.f) * VDIV_RATIO;
}

std::unique_ptr<HAEntity> create_battery_entity()
{
        auto entity = std::make_unique<HAEntity>();
        entity->state = std::to_string(get_battery_voltage());
        entity->entity_id = "sensor.car_battery";
        entity->add_attribute("friendly_name", "Car Battery Voltage");
        entity->add_attribute("unit_of_measurement", "Volts");
        return entity;
}

} // namespace BatteryMonitor
