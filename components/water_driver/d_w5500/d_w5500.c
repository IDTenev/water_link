#include "common_data.h"
#include "error_code.h"
#include "d_w5500.h"

#include "p_spi.h"
#include "d_w5500/d_w5500_reg.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "DRIVER/W5500"

static p_spi_dev_t s_w5500_spi = {0};
static bool   s_ready = false;
static uint8_t s_pmode = 0xFF;
static uint8_t s_ver = 0x00;

static void w5500_reset_pin_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << PIN_W5500_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
}

static void w5500_reset_sequence(void)
{
    w5500_reset_pin_init();
    gpio_set_level(PIN_W5500_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PIN_W5500_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

static uint8_t w5500_read_pmode_strap(void)
{
    gpio_set_direction(PIN_W5500_PMODE0, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_W5500_PMODE1, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_W5500_PMODE2, GPIO_MODE_INPUT);

    int p0 = gpio_get_level(PIN_W5500_PMODE0);
    int p1 = gpio_get_level(PIN_W5500_PMODE1);
    int p2 = gpio_get_level(PIN_W5500_PMODE2);

    return (uint8_t)((p2 << 2) | (p1 << 1) | (p0 << 0));
}

uint16_t w5500_init(void)
{
    s_ready = false;
    s_pmode = 0xFF;
    s_ver   = 0x00;

    ESP_LOGI(TAG, "W5500 init");

    // 1) Strap 확인
    s_pmode = w5500_read_pmode_strap();
    ESP_LOGI(TAG, "PMODE strap = %u (bin %d%d%d)",
             s_pmode,
             (s_pmode >> 2) & 1, (s_pmode >> 1) & 1, (s_pmode >> 0) & 1);

    // 2) Reset
    w5500_reset_sequence();

    // 3) SPI device add (딱 1번만)
    uint16_t r = spi_dev_add(&s_w5500_spi, 8 * 1000 * 1000, 0 /*mode0*/);
    if (r != SPI_OK) {
        ESP_LOGE(TAG, "spi_dev_add failed (%u)", (unsigned)r);
        return r;
    }

    // 4) REG 모듈에 SPI 바인딩
    r = w5500_reg_bind_spi(&s_w5500_spi);
    if (r != SPI_OK) {
        ESP_LOGE(TAG, "w5500_reg_bind_spi failed (%u)", (unsigned)r);
        return r;
    }

    // 5) VERSIONR 읽기 (페이즈1 reg API로)
    r = w5500_read8(W5500_BSB_COMMON, W5500_VERSIONR, &s_ver);
    if (r != SPI_OK) {
        ESP_LOGE(TAG, "VERSIONR read failed (%u)", (unsigned)r);
        return r;
    }

    ESP_LOGI(TAG, "VERSIONR(0x%04X) = 0x%02X", W5500_VERSIONR, s_ver);

    if (s_ver == 0x04) {
        s_ready = true;
        ESP_LOGI(TAG, "W5500 ready");
        return SPI_OK;
    }

    ESP_LOGW(TAG, "W5500 not ready (unexpected version). Check wiring/CS/RST.");
    return SPI_ERROR;
}
