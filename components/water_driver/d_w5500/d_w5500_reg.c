#include "common_data.h"
#include "error_code.h"
#include "d_w5500_reg.h"

#include <string.h>

#include "esp_log.h"

#define TAG "DRIVER/W5500/REG"

// W5500 control fields
// RWB: 0=READ, 1=WRITE
// OM : 00=VDM (Variable Data Length Mode)
static inline uint8_t w5500_ctrl(uint8_t bsb, uint8_t rwb, uint8_t om)
{
    return (uint8_t)(((bsb & 0x1F) << 3) | ((rwb & 0x01) << 2) | (om & 0x03));
}

// SPI handle (bound from d_w5500.c)
static p_spi_dev_t *s_spi = NULL;

// 전송 청크 크기: (header 3 + payload <= 256) 정도로 안전
#define W5500_XFER_CHUNK 256

uint16_t w5500_reg_bind_spi(p_spi_dev_t *spi)
{
    if (!spi || !spi->dev) return SPI_ERROR_ARGUMENT;
    s_spi = spi;
    ESP_LOGI(TAG, "bind spi ok");
    return SPI_OK;
}

// 내부 공통: header+payload를 한 번에 처리하는 helper
static uint16_t w5500_xfer(uint8_t bsb, uint16_t addr, uint8_t rwb,
                          const uint8_t *tx_payload, uint8_t *rx_payload, size_t len)
{
    if (!s_spi || !s_spi->dev) return SPI_ERROR_DRIVER;

    // len=0이면 header만 보내는 경우도 있을 수 있는데, 여기선 금지(명확하게)
    if (len == 0) return SPI_ERROR_ARGUMENT;

    // header 3바이트 + payload(len)
    // read: payload 만큼 dummy를 보내며 rx를 받음
    // write: payload를 그대로 전송
    uint8_t tx[3 + W5500_XFER_CHUNK];
    uint8_t rx[3 + W5500_XFER_CHUNK];

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > W5500_XFER_CHUNK) chunk = W5500_XFER_CHUNK;

        // header
        tx[0] = (uint8_t)(addr >> 8);
        tx[1] = (uint8_t)(addr & 0xFF);
        tx[2] = w5500_ctrl(bsb, rwb, 0 /*VDM*/);

        if (rwb == 1) {
            // WRITE: header + data
            memcpy(&tx[3], &tx_payload[offset], chunk);

            uint16_t r = spi_txrx(s_spi, tx, NULL, 3 + chunk);
            if (r != SPI_OK) return r;
        } else {
            // READ: header + dummy, then take rx[3..]
            memset(&tx[3], 0x00, chunk);

            uint16_t r = spi_txrx(s_spi, tx, rx, 3 + chunk);
            if (r != SPI_OK) return r;

            memcpy(&rx_payload[offset], &rx[3], chunk);
        }

        addr   += (uint16_t)chunk; // auto-increment 주소와 맞춰서 진행
        offset += chunk;
    }

    return SPI_OK;
}

uint16_t w5500_read8(uint8_t bsb, uint16_t addr, uint8_t *out)
{
    if (!out) return SPI_ERROR_ARGUMENT;
    uint8_t v = 0;
    uint16_t r = w5500_xfer(bsb, addr, 0 /*READ*/, NULL, &v, 1);
    if (r != SPI_OK) return r;
    *out = v;
    return SPI_OK;
}

uint16_t w5500_write8(uint8_t bsb, uint16_t addr, uint8_t val)
{
    return w5500_xfer(bsb, addr, 1 /*WRITE*/, &val, NULL, 1);
}

uint16_t w5500_read16(uint8_t bsb, uint16_t addr, uint16_t *out)
{
    if (!out) return SPI_ERROR_ARGUMENT;
    uint8_t b[2] = {0};
    uint16_t r = w5500_xfer(bsb, addr, 0 /*READ*/, NULL, b, 2);
    if (r != SPI_OK) return r;
    *out = (uint16_t)((b[0] << 8) | b[1]); // W5500 레지스터는 big-endian 표기
    return SPI_OK;
}

uint16_t w5500_write16(uint8_t bsb, uint16_t addr, uint16_t val)
{
    uint8_t b[2] = {(uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    return w5500_xfer(bsb, addr, 1 /*WRITE*/, b, NULL, 2);
}

uint16_t w5500_read_buf(uint8_t bsb, uint16_t addr, uint8_t *buf, size_t len)
{
    if (!buf || len == 0) return SPI_ERROR_ARGUMENT;
    return w5500_xfer(bsb, addr, 0 /*READ*/, NULL, buf, len);
}

uint16_t w5500_write_buf(uint8_t bsb, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (!buf || len == 0) return SPI_ERROR_ARGUMENT;
    return w5500_xfer(bsb, addr, 1 /*WRITE*/, buf, NULL, len);
}
