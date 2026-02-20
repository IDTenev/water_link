#include "common_data.h"
#include "error_code.h"
#include "p_spi.h"

#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

#define TAG "PORT/SPI"

#ifndef WATER_SPI_HOST
#define WATER_SPI_HOST SPI2_HOST
#endif

#ifndef WATER_SPI_DMA_CHAN
#define WATER_SPI_DMA_CHAN SPI_DMA_CH_AUTO
#endif

static bool s_spi_inited = false;

bool spi_is_inited(void)
{
    return s_spi_inited;
}

uint16_t spi_init(void)
{
    if (s_spi_inited) {
        ESP_LOGI(TAG, "spi already inited");
        return SPI_OK;
    }

    ESP_LOGI(TAG, "spi init host=%d MOSI=%d MISO=%d SCK=%d CS=%d",
             (int)WATER_SPI_HOST, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK, PIN_SPI_CS);

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_SPI_MOSI,
        .miso_io_num = PIN_SPI_MISO,
        .sclk_io_num = PIN_SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t err = spi_bus_initialize(WATER_SPI_HOST, &buscfg, WATER_SPI_DMA_CHAN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return SPI_ERROR_DRIVER;
    }

    s_spi_inited = true;
    ESP_LOGI(TAG, "spi init done");
    return SPI_OK;
}

uint16_t spi_dev_add(p_spi_dev_t *out, int clock_hz, int mode)
{
    if (!out) return SPI_ERROR_ARGUMENT;
    if (!s_spi_inited) {
        uint16_t r = spi_init();
        if (r != SPI_OK) return r;
    }

    // 이미 등록된 핸들이면 재등록 금지 (안 꼬이게!)
    if (out->dev != NULL) {
        ESP_LOGW(TAG, "spi_dev_add ignored (already has device handle)");
        return SPI_OK;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = clock_hz,
        .mode = mode,
        .spics_io_num = PIN_SPI_CS,
        .queue_size = 4,
        .flags = 0, // FULL DUPLEX로 두고, W5500 드라이버에서 프레임을 쪼개든 말든 선택
    };

    spi_device_handle_t handle = NULL;
    esp_err_t err = spi_bus_add_device(WATER_SPI_HOST, &devcfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        return SPI_ERROR_DRIVER;
    }

    out->host = WATER_SPI_HOST;
    out->dev  = handle;

    ESP_LOGI(TAG, "spi dev added clk=%dHz mode=%d cs=%d", clock_hz, mode, PIN_SPI_CS);
    return SPI_OK;
}

uint16_t spi_txrx(p_spi_dev_t *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (!dev || !dev->dev || len == 0) return SPI_ERROR_ARGUMENT;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = len * 8;
    if (rx) t.rxlength = len * 8;

    t.tx_buffer = tx;
    t.rx_buffer = rx;

    esp_err_t err = spi_device_transmit(dev->dev, &t);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_transmit failed: %s", esp_err_to_name(err));
        return SPI_ERROR_DRIVER;
    }

    return SPI_OK;
}
