#include "common_data.h"
#include "error_code.h"
#include "d_w5500_socket.h"
#include "d_w5500_reg.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static inline bool sn_valid(uint8_t sn) { return sn < 8; }

// -------------------------
// thin wrappers over reg layer
// -------------------------
uint16_t w5500_sn_read8(uint8_t sn, uint16_t reg, uint8_t *out)
{
    if (!sn_valid(sn) || !out) return SPI_ERROR_ARGUMENT;
    return w5500_read8(W5500_BSB_SOCK_REG(sn), reg, out);
}

uint16_t w5500_sn_write8(uint8_t sn, uint16_t reg, uint8_t val)
{
    if (!sn_valid(sn)) return SPI_ERROR_ARGUMENT;
    return w5500_write8(W5500_BSB_SOCK_REG(sn), reg, val);
}

uint16_t w5500_sn_read16(uint8_t sn, uint16_t reg, uint16_t *out)
{
    if (!sn_valid(sn) || !out) return SPI_ERROR_ARGUMENT;
    return w5500_read16(W5500_BSB_SOCK_REG(sn), reg, out);
}

uint16_t w5500_sn_write16(uint8_t sn, uint16_t reg, uint16_t val)
{
    if (!sn_valid(sn)) return SPI_ERROR_ARGUMENT;
    return w5500_write16(W5500_BSB_SOCK_REG(sn), reg, val);
}

uint16_t w5500_sn_read_buf(uint8_t sn, uint16_t reg, uint8_t *buf, size_t len)
{
    if (!sn_valid(sn) || !buf || len == 0) return SPI_ERROR_ARGUMENT;
    return w5500_read_buf(W5500_BSB_SOCK_REG(sn), reg, buf, len);
}

uint16_t w5500_sn_write_buf(uint8_t sn, uint16_t reg, const uint8_t *buf, size_t len)
{
    if (!sn_valid(sn) || !buf || len == 0) return SPI_ERROR_ARGUMENT;
    return w5500_write_buf(W5500_BSB_SOCK_REG(sn), reg, buf, len);
}

// -------------------------
// command helper
// -------------------------
uint16_t w5500_sock_cmd(uint8_t sn, uint8_t cmd, uint32_t timeout_ms)
{
    if (!sn_valid(sn)) return SPI_ERROR_ARGUMENT;

    uint16_t r = w5500_sn_write8(sn, W5500_Sn_CR, cmd);
    if (r != SPI_OK) return r;

    // Sn_CR은 커맨드 수행 후 0으로 자동 클리어됨
    const TickType_t start = xTaskGetTickCount();
    const TickType_t limit = pdMS_TO_TICKS(timeout_ms);

    while (1) {
        uint8_t cr = 0xFF;
        r = w5500_sn_read8(sn, W5500_Sn_CR, &cr);
        if (r != SPI_OK) return r;

        if (cr == 0x00) return SPI_OK;

        if ((xTaskGetTickCount() - start) > limit) {
            return SPI_ERROR; // timeout
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// -------------------------
// stable read helpers (TX_FSR / RX_RSR)
// -------------------------
static uint16_t stable_read16(uint8_t sn, uint16_t reg, uint16_t *out)
{
    uint16_t a = 0, b = 0;
    uint16_t r;

    // 무한루프 금지: 최대 50번만 시도
    for (int i = 0; i < 50; i++) {
        r = w5500_sn_read16(sn, reg, &a);
        if (r != SPI_OK) return r;

        r = w5500_sn_read16(sn, reg, &b);
        if (r != SPI_OK) return r;

        if (a == b) {
            *out = a;
            return SPI_OK;
        }

        // yield/delay 없으면 IDLE이 못 돌아서 WDT 터짐
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return SPI_ERROR; // 안정값 못 얻음
}


uint16_t w5500_sock_get_tx_free(uint8_t sn, uint16_t *out_fsr)
{
    if (!sn_valid(sn) || !out_fsr) return SPI_ERROR_ARGUMENT;
    return stable_read16(sn, W5500_Sn_TX_FSR, out_fsr);
}

uint16_t w5500_sock_get_rx_size(uint8_t sn, uint16_t *out_rsr)
{
    if (!sn_valid(sn) || !out_rsr) return SPI_ERROR_ARGUMENT;
    return stable_read16(sn, W5500_Sn_RX_RSR, out_rsr);
}

// -------------------------
// buffer size (KB -> bytes)
// Sn_TXBUF_SIZE / Sn_RXBUF_SIZE: 0,1,2,4,8,16 (KB) in typical usage
// -------------------------
uint16_t w5500_sock_get_tx_bufsize(uint8_t sn, uint16_t *out_bytes)
{
    if (!sn_valid(sn) || !out_bytes) return SPI_ERROR_ARGUMENT;
    uint8_t kb = 0;
    uint16_t r = w5500_sn_read8(sn, W5500_Sn_TXBUF_SIZE, &kb);
    if (r != SPI_OK) return r;
    *out_bytes = (uint16_t)kb * 1024;
    return SPI_OK;
}

uint16_t w5500_sock_get_rx_bufsize(uint8_t sn, uint16_t *out_bytes)
{
    if (!sn_valid(sn) || !out_bytes) return SPI_ERROR_ARGUMENT;
    uint8_t kb = 0;
    uint16_t r = w5500_sn_read8(sn, W5500_Sn_RXBUF_SIZE, &kb);
    if (r != SPI_OK) return r;
    *out_bytes = (uint16_t)kb * 1024;
    return SPI_OK;
}

// -------------------------
// ring buffer copy helpers (TX/RX memory blocks)
// Address in TX/RX memory block is 16-bit pointer (offset) that wraps by bufsize.
// -------------------------
static uint16_t mem_write_ring(uint8_t bsb_mem, uint16_t ptr, const uint8_t *data, size_t len, uint16_t bufsize)
{
    if (!data || len == 0 || bufsize == 0) return SPI_ERROR_ARGUMENT;

    // ring wrap: two segments max
    uint16_t off = (uint16_t)(ptr % bufsize);
    uint16_t first = bufsize - off;
    if (first > len) first = (uint16_t)len;

    uint16_t r = w5500_write_buf(bsb_mem, off, data, first);
    if (r != SPI_OK) return r;

    size_t remain = len - first;
    if (remain > 0) {
        r = w5500_write_buf(bsb_mem, 0, data + first, remain);
        if (r != SPI_OK) return r;
    }

    return SPI_OK;
}

static uint16_t mem_read_ring(uint8_t bsb_mem, uint16_t ptr, uint8_t *data, size_t len, uint16_t bufsize)
{
    if (!data || len == 0 || bufsize == 0) return SPI_ERROR_ARGUMENT;

    uint16_t off = (uint16_t)(ptr % bufsize);
    uint16_t first = bufsize - off;
    if (first > len) first = (uint16_t)len;

    uint16_t r = w5500_read_buf(bsb_mem, off, data, first);
    if (r != SPI_OK) return r;

    size_t remain = len - first;
    if (remain > 0) {
        r = w5500_read_buf(bsb_mem, 0, data + first, remain);
        if (r != SPI_OK) return r;
    }

    return SPI_OK;
}

// -------------------------
// TX write: write data into TX buffer, update Sn_TX_WR, but do not SEND here.
// -------------------------
uint16_t w5500_sock_tx_write(uint8_t sn, const uint8_t *data, size_t len, uint16_t *new_tx_wr)
{
    if (!sn_valid(sn) || !data || len == 0) return SPI_ERROR_ARGUMENT;

    uint16_t txbuf = 0;
    uint16_t r = w5500_sock_get_tx_bufsize(sn, &txbuf);
    if (r != SPI_OK) return r;
    if (txbuf == 0) return SPI_ERROR; // buffer not configured

    uint16_t tx_wr = 0;
    r = w5500_sn_read16(sn, W5500_Sn_TX_WR, &tx_wr);
    if (r != SPI_OK) return r;

    // write into TX memory
    r = mem_write_ring(W5500_BSB_SOCK_TX(sn), tx_wr, data, len, txbuf);
    if (r != SPI_OK) return r;

    tx_wr = (uint16_t)(tx_wr + (uint16_t)len);
    r = w5500_sn_write16(sn, W5500_Sn_TX_WR, tx_wr);
    if (r != SPI_OK) return r;

    if (new_tx_wr) *new_tx_wr = tx_wr;
    return SPI_OK;
}

// -------------------------
// RX read: read data from RX buffer, update Sn_RX_RD (not RECV yet).
// caller decides how much to read (usually <= RX_RSR).
// -------------------------
uint16_t w5500_sock_rx_read(uint8_t sn, uint8_t *data, size_t len, uint16_t *new_rx_rd)
{
    if (!sn_valid(sn) || !data || len == 0) return SPI_ERROR_ARGUMENT;

    uint16_t rxbuf = 0;
    uint16_t r = w5500_sock_get_rx_bufsize(sn, &rxbuf);
    if (r != SPI_OK) return r;
    if (rxbuf == 0) return SPI_ERROR;

    uint16_t rx_rd = 0;
    r = w5500_sn_read16(sn, W5500_Sn_RX_RD, &rx_rd);
    if (r != SPI_OK) return r;

    r = mem_read_ring(W5500_BSB_SOCK_RX(sn), rx_rd, data, len, rxbuf);
    if (r != SPI_OK) return r;

    rx_rd = (uint16_t)(rx_rd + (uint16_t)len);
    r = w5500_sn_write16(sn, W5500_Sn_RX_RD, rx_rd);
    if (r != SPI_OK) return r;

    if (new_rx_rd) *new_rx_rd = rx_rd;
    return SPI_OK;
}

uint16_t w5500_sock_rx_peek(uint8_t sn, uint16_t rx_rd, uint8_t *data, size_t len)
{
    if (!sn_valid(sn) || !data || len == 0) return SPI_ERROR_ARGUMENT;

    uint16_t rxbuf = 0;
    uint16_t r = w5500_sock_get_rx_bufsize(sn, &rxbuf);
    if (r != SPI_OK) return r;
    if (rxbuf == 0) return SPI_ERROR;

    return mem_read_ring(W5500_BSB_SOCK_RX(sn), rx_rd, data, len, rxbuf);
}

// -------------------------
// RX consume: after updating RX_RD, issue RECV command to free buffer.
// -------------------------
uint16_t w5500_sock_rx_consume(uint8_t sn, uint16_t new_rx_rd, uint32_t timeout_ms)
{
    if (!sn_valid(sn)) return SPI_ERROR_ARGUMENT;

    uint16_t r = w5500_sn_write16(sn, W5500_Sn_RX_RD, new_rx_rd);
    if (r != SPI_OK) return r;

    return w5500_sock_cmd(sn, W5500_CR_RECV, timeout_ms);
}
