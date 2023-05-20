#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#define CHANNEL ADC1_CHANNEL_4
#define ATTEN ADC_ATTEN_DB_11
#define WIDTH ADC_WIDTH_BIT_12
#define UNIT ADC_UNIT_1
#define VREF 1100

// Calibrate the battery voltage measurement based on the resistor voltage divider
#define VDIV_RATIO 5.02

esp_adc_cal_characteristics_t* batt_adc_characteristics;

// Configures the battery ADC 
static void configure_batt_adc(){
    ESP_LOGI("ADC", "Configuring battery ADC characteristics");

    adc1_config_width(WIDTH);
    adc1_config_channel_atten(CHANNEL, ATTEN); //2600mV MAX
    batt_adc_characteristics = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(UNIT, ATTEN, WIDTH, VREF, batt_adc_characteristics);
}

// Read ADC voltage from battery and return battery voltage calculated from adc and voltage divider
float get_battery_voltage() {
    if(!batt_adc_characteristics) {
        configure_batt_adc();
    }

    uint32_t reading = adc1_get_raw(CHANNEL);
    float voltage = esp_adc_cal_raw_to_voltage(reading, batt_adc_characteristics) / 1000.0;

    ESP_LOGI("ADC", "Battery Read RAW = %.2f V, CONVERTED = %.2f V", voltage, voltage*VDIV_RATIO);
    
    return voltage*VDIV_RATIO;
}
