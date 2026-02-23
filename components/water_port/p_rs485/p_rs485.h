#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t rs485_init(void);

/**
 * @brief RS485 송신 (블로킹)
 * @return 송신한 바이트 수 (0이면 실패)
 */
size_t rs485_send(const uint8_t *data, size_t len, uint32_t timeout_ms);

/**
 * @brief RS485 수신 (최대 len 바이트, timeout_ms 동안 대기)
 * @return 수신한 바이트 수
 */
size_t rs485_read(uint8_t *out, size_t len, uint32_t timeout_ms);

/** RX 버퍼 비우기 */
void rs485_flush_rx(void);

/** TX 완료까지 대기 */
void rs485_wait_tx_done(uint32_t timeout_ms);

/** 보드레이트 변경 */
uint16_t rs485_set_baud(int baudrate);

#ifdef __cplusplus
}
#endif
