#include "common_data.h"
#include "p_i2c.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2c_master.h"

#define TAG "PORT/I2C"

static i2c_master_bus_handle_t s_bus = NULL;

static inline uint16_t err_to_u16(esp_err_t err)
{
    // 너희 프로젝트 에러코드 체계 있으면 여기서 매핑하면 됨.
    // 일단 0=성공, 그 외=ESP-IDF 에러를 압축해서 반환
    return (err == ESP_OK) ? 0 : (uint16_t)(0x8000 | (err & 0x7FFF));
}

uint16_t i2c_init(void)
{
    if (s_bus) {
        ESP_LOGW(TAG, "i2c already init");
        return 0;
    }

    ESP_LOGI(TAG, "i2c init (SDA=%d, SCL=%d, %d Hz)", PIN_I2C_SDA, PIN_I2C_SCL, I2C_SCL_SPEED_HZ);

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT_NUM,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
        .intr_priority = 0,
        .flags = {
            .enable_internal_pullup = 0, // 외부 풀업 있으면 0으로 바꿔도 됨
        },
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        s_bus = NULL;
        return err_to_u16(err);
    }

    return 0;
}

uint16_t i2c_deinit(void)
{
    if (!s_bus) return 0;

    esp_err_t err = i2c_del_master_bus(s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_del_master_bus failed: %s", esp_err_to_name(err));
        return err_to_u16(err);
    }

    s_bus = NULL;
    return 0;
}

static esp_err_t add_temp_device(uint8_t addr_7bit, i2c_master_dev_handle_t *out_dev)
{
    if (!s_bus) return ESP_ERR_INVALID_STATE;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr_7bit,
        .scl_speed_hz = I2C_SCL_SPEED_HZ,
    };

    return i2c_master_bus_add_device(s_bus, &dev_cfg, out_dev);
}

uint16_t i2c_probe(uint8_t addr_7bit)
{
    if (!s_bus) return err_to_u16(ESP_ERR_INVALID_STATE);

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = add_temp_device(addr_7bit, &dev);
    if (err != ESP_OK) return err_to_u16(err);

    // HT16K33 기준: oscillator on 커맨드 (ACK 확인용으로도 적절)
    uint8_t cmd = 0x21;

    err = i2c_master_transmit(dev, &cmd, 1, I2C_TIMEOUT_MS);

    esp_err_t err2 = i2c_master_bus_rm_device(dev);
    if (err2 != ESP_OK) {
        ESP_LOGW(TAG, "rm_device warn: %s", esp_err_to_name(err2));
    }

    return err_to_u16(err);
}

uint16_t i2c_write(uint8_t addr_7bit, const uint8_t *data, size_t len)
{
    if (!s_bus) return err_to_u16(ESP_ERR_INVALID_STATE);
    if (!data || len == 0) return err_to_u16(ESP_ERR_INVALID_ARG);

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = add_temp_device(addr_7bit, &dev);
    if (err != ESP_OK) return err_to_u16(err);

    err = i2c_master_transmit(dev, data, len, I2C_TIMEOUT_MS);

    esp_err_t err2 = i2c_master_bus_rm_device(dev);
    if (err2 != ESP_OK) {
        ESP_LOGW(TAG, "rm_device warn: %s", esp_err_to_name(err2));
    }

    return err_to_u16(err);
}

uint16_t i2c_read(uint8_t addr_7bit, uint8_t *data, size_t len)
{
    if (!s_bus) return err_to_u16(ESP_ERR_INVALID_STATE);
    if (!data || len == 0) return err_to_u16(ESP_ERR_INVALID_ARG);

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = add_temp_device(addr_7bit, &dev);
    if (err != ESP_OK) return err_to_u16(err);

    err = i2c_master_receive(dev, data, len, I2C_TIMEOUT_MS);

    esp_err_t err2 = i2c_master_bus_rm_device(dev);
    if (err2 != ESP_OK) {
        ESP_LOGW(TAG, "rm_device warn: %s", esp_err_to_name(err2));
    }

    return err_to_u16(err);
}

uint16_t i2c_write_read(uint8_t addr_7bit,
                        const uint8_t *wdata, size_t wlen,
                        uint8_t *rdata, size_t rlen)
{
    if (!s_bus) return err_to_u16(ESP_ERR_INVALID_STATE);
    if (!wdata || wlen == 0 || !rdata || rlen == 0) return err_to_u16(ESP_ERR_INVALID_ARG);

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = add_temp_device(addr_7bit, &dev);
    if (err != ESP_OK) return err_to_u16(err);

    err = i2c_master_transmit_receive(dev, wdata, wlen, rdata, rlen, I2C_TIMEOUT_MS);

    esp_err_t err2 = i2c_master_bus_rm_device(dev);
    if (err2 != ESP_OK) {
        ESP_LOGW(TAG, "rm_device warn: %s", esp_err_to_name(err2));
    }

    return err_to_u16(err);
}
