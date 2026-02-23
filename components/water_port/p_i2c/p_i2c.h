#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t i2c_init(void);
uint16_t i2c_deinit(void);

/**
 * @brief 7-bit address probe (ACK 확인)
 */
uint16_t i2c_probe(uint8_t addr_7bit);

/**
 * @brief Write only
 */
uint16_t i2c_write(uint8_t addr_7bit, const uint8_t *data, size_t len);

/**
 * @brief Read only
 */
uint16_t i2c_read(uint8_t addr_7bit, uint8_t *data, size_t len);

/**
 * @brief Write then Read (레지스터 읽기 같은 흔한 패턴)
 *        예: [reg] 쓰고, 이어서 N바이트 읽기
 */
uint16_t i2c_write_read(uint8_t addr_7bit,
                        const uint8_t *wdata, size_t wlen,
                        uint8_t *rdata, size_t rlen);

#ifdef __cplusplus
}
#endif
