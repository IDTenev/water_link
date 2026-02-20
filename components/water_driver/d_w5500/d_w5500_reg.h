#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "p_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------- bind ----------
uint16_t w5500_reg_bind_spi(p_spi_dev_t *spi);

// ---------- register R/W (auto-increment, VDM) ----------
uint16_t w5500_read8 (uint8_t bsb, uint16_t addr, uint8_t *out);
uint16_t w5500_write8(uint8_t bsb, uint16_t addr, uint8_t val);

uint16_t w5500_read16 (uint8_t bsb, uint16_t addr, uint16_t *out);
uint16_t w5500_write16(uint8_t bsb, uint16_t addr, uint16_t val);

// bulk register read/write (sequential)
uint16_t w5500_read_buf (uint8_t bsb, uint16_t addr, uint8_t *buf, size_t len);
uint16_t w5500_write_buf(uint8_t bsb, uint16_t addr, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif
