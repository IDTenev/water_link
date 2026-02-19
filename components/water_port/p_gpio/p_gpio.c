#include "common_data.h"
#include "error_code.h"
#include "p_gpio.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "PORT/GPIO"

static uint16_t sw_water_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_WATER_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&io)==ESP_OK?NO_ERROR:GPIO_ERROR;
}

static uint16_t sw_06_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_IO06),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&io)==ESP_OK?NO_ERROR:GPIO_ERROR;
}

static uint16_t sw_14_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_IO14),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&io)==ESP_OK?NO_ERROR:GPIO_ERROR;
}

static uint16_t sw_37_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_IO37),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&io)==ESP_OK?NO_ERROR:GPIO_ERROR;
}

uint16_t gpio_init(void) {
    uint16_t result = 0;
    ESP_LOGI(TAG, "GPIO INIT START");

    result = sw_water_init();
    if(result != NO_ERROR) ESP_LOGI(TAG, "SW_WATER INIT ERROR");
    result = sw_06_init();
    if(result != NO_ERROR) ESP_LOGI(TAG, "SW_06 INIT ERROR");
    result = sw_14_init();
    if(result != NO_ERROR) ESP_LOGI(TAG, "SW_14 INIT ERROR");
    result = sw_37_init();
    if(result != NO_ERROR) ESP_LOGI(TAG, "SW_37 INIT ERROR");

    ESP_LOGI(TAG, "GPIO INIT DONE");
    return result==NO_ERROR?NO_ERROR:GPIO_ERROR;
}

uint16_t gpio_sw_water_get_level(void) {
    return gpio_get_level(PIN_WATER_SW);
}

uint16_t gpio_sw_06_get_level(void) {
    return gpio_get_level(PIN_IO06);
}

uint16_t gpio_sw_14_get_level(void) {
    return gpio_get_level(PIN_IO14);
}

uint16_t gpio_sw_37_get_level(void) {
    return gpio_get_level(PIN_IO37);
}