#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------
// API: Socket register helpers
// -------------------------
uint16_t w5500_sn_read8 (uint8_t sn, uint16_t reg, uint8_t *out);
uint16_t w5500_sn_write8(uint8_t sn, uint16_t reg, uint8_t val);

uint16_t w5500_sn_read16 (uint8_t sn, uint16_t reg, uint16_t *out);
uint16_t w5500_sn_write16(uint8_t sn, uint16_t reg, uint16_t val);

uint16_t w5500_sn_read_buf (uint8_t sn, uint16_t reg, uint8_t *buf, size_t len);
uint16_t w5500_sn_write_buf(uint8_t sn, uint16_t reg, const uint8_t *buf, size_t len);

// -------------------------
// API: Socket util (Phase 1.5 핵심)
// -------------------------

// Sn_CR에 커맨드 쓰고, Sn_CR이 0으로 돌아올 때까지 대기
uint16_t w5500_sock_cmd(uint8_t sn, uint8_t cmd, uint32_t timeout_ms);

// TX free size / RX received size (datasheet 권장: 안정값 나올 때까지 2번 읽기)
uint16_t w5500_sock_get_tx_free(uint8_t sn, uint16_t *out_fsr);
uint16_t w5500_sock_get_rx_size(uint8_t sn, uint16_t *out_rsr);

// Socket buffer size (KB 단위 레지스터를 Byte로 변환)
uint16_t w5500_sock_get_tx_bufsize(uint8_t sn, uint16_t *out_bytes);
uint16_t w5500_sock_get_rx_bufsize(uint8_t sn, uint16_t *out_bytes);

// TX 링버퍼에 “쓰기” (Sn_TX_WR 갱신 + SEND는 별도)
uint16_t w5500_sock_tx_write(uint8_t sn, const uint8_t *data, size_t len, uint16_t *new_tx_wr);

// RX 링버퍼에서 “읽기” (Sn_RX_RD 갱신 + RECV는 별도)
uint16_t w5500_sock_rx_read(uint8_t sn, uint8_t *data, size_t len, uint16_t *new_rx_rd);
uint16_t w5500_sock_rx_peek(uint8_t sn, uint16_t rx_rd, uint8_t *data, size_t len);

// RX 데이터 소비 처리 (Sn_RX_RD 반영 + RECV 커맨드)
uint16_t w5500_sock_rx_consume(uint8_t sn, uint16_t new_rx_rd, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
