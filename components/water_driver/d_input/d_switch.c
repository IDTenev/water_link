#include "common_data.h"
#include "error_code.h"
#include "d_switch.h"
#include "p_gpio.h"

#include "esp_log.h"

#define TAG "PORT/GPIO"

uint16_t switch_get_water_status(void) {
    uint16_t result = gpio_sw_water_get_level();
    if(result == SW_SHORT) {
        ESP_LOGI(TAG, "SW WATER SHORT!");
    }
    else if(result == SW_OPEN) {
        ESP_LOGI(TAG, "SW WATER OPEN!");
    }

    return result;
}

uint16_t switch_get_06_status(void) {
    uint16_t result = gpio_sw_06_get_level();
    if(result == SW_SHORT) {
        ESP_LOGI(TAG, "SW 06 SHORT!");
    }
    else if(result == SW_OPEN) {
        ESP_LOGI(TAG, "SW 06 OPEN!");
    }

    return result;
}

uint16_t switch_get_14_status(void) {
    uint16_t result = gpio_sw_14_get_level();
    if(result == SW_SHORT) {
        ESP_LOGI(TAG, "SW 14 SHORT!");
    }
    else if(result == SW_OPEN) {
        ESP_LOGI(TAG, "SW 14 OPEN!");
    }

    return result;
}

uint16_t switch_get_37_status(void) {
    uint16_t result = gpio_sw_37_get_level();
    if(result == SW_SHORT) {
        ESP_LOGI(TAG, "SW 37 SHORT!");
    }
    else if(result == SW_OPEN) {
        ESP_LOGI(TAG, "SW 37 OPEN!");
    }

    return result;
}