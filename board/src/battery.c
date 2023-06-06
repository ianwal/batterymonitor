#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define BATT_VOLTAGE_ADC_CHANNEL ADC_CHANNEL_3
#define BATT_VOLTAGE_ADC_ATTEN ADC_ATTEN_DB_11
#define BATT_VOLTAGE_ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_UNIT ADC_UNIT_1
#define ADC_VREF 1100

// Calibrate the battery voltage measurement based on the resistor voltage divider
#define VDIV_RATIO 5.02

static const char *TAG = "Battery";

adc_oneshot_unit_handle_t batt_voltage_adc_handle;
adc_cali_handle_t batt_voltage_adc_cali_handle = NULL;

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_bitwidth_t bitwidth, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = bitwidth,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
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
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &batt_voltage_adc_handle));

    // ADC1 Config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = BATT_VOLTAGE_ADC_BITWIDTH,
        .atten = BATT_VOLTAGE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(batt_voltage_adc_handle, BATT_VOLTAGE_ADC_CHANNEL, &config));
}

// Deinitialize calibration of an ADC handle
static void adc_calibration_deinit(adc_cali_handle_t handle)
{
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
}

// Read ADC voltage from battery and return battery voltage calculated from the ADC and voltage divider
float get_battery_voltage()
{
    int adc_raw[1][10];
    int voltage[1][10];

    config_batt_adc();

    // ADC1 Read
    ESP_ERROR_CHECK(adc_oneshot_read(batt_voltage_adc_handle, BATT_VOLTAGE_ADC_CHANNEL, &adc_raw[0][0]));
    ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT + 1, BATT_VOLTAGE_ADC_CHANNEL, adc_raw[0][0]);
    
    // ADC1 Calibration
    bool do_calibration_batt_voltage = adc_calibration_init(ADC_UNIT, BATT_VOLTAGE_ADC_CHANNEL, BATT_VOLTAGE_ADC_ATTEN, BATT_VOLTAGE_ADC_BITWIDTH, &batt_voltage_adc_cali_handle);
    
    if (do_calibration_batt_voltage) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(batt_voltage_adc_cali_handle, adc_raw[0][0], &voltage[0][0]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT + 1, BATT_VOLTAGE_ADC_CHANNEL, voltage[0][0]);
    }

    // ADC1 Teardown
    ESP_ERROR_CHECK(adc_oneshot_del_unit(batt_voltage_adc_handle));
    if(do_calibration_batt_voltage)
        adc_calibration_deinit(batt_voltage_adc_cali_handle);

    return (voltage[0][0] / 1000.f) * VDIV_RATIO;
}
