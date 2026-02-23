#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    spi_host_device_t host;
    spi_device_handle_t dev;
} p_spi_dev_t;

// SPI 버스 초기화(1회)
uint16_t spi_init(void);

// 버스 초기화 여부
bool spi_is_inited(void);

// 디바이스 등록 (CS는 common_data.h의 PIN_SPI_CS 사용)
uint16_t spi_dev_add(p_spi_dev_t *out, int clock_hz, int mode);

// TX/RX 트랜잭션 (tx==NULL -> 0x00 더미 송신)
uint16_t spi_txrx(p_spi_dev_t *dev, const uint8_t *tx, uint8_t *rx, size_t len);

static inline uint16_t spi_write(p_spi_dev_t *dev, const uint8_t *tx, size_t len) {
    return spi_txrx(dev, tx, NULL, len);
}
static inline uint16_t spi_read(p_spi_dev_t *dev, uint8_t *rx, size_t len) {
    return spi_txrx(dev, NULL, rx, len);
}

#ifdef __cplusplus
}
#endif
