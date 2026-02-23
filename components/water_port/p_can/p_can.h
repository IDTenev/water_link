#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "driver/twai.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TWAI(CAN2.0) init + start
 * @return 0 if success, else non-zero
 */
uint16_t can_init(void);

/**
 * @brief Send one CAN frame
 *
 * @param id        11-bit or 29-bit identifier
 * @param data      payload pointer (can be NULL when dlc=0 or rtr=true)
 * @param dlc       0..8
 * @param extended  false: standard(11-bit), true: extended(29-bit)
 * @param rtr       Remote Transmission Request
 * @param timeout_ms tx queue wait timeout in ms
 *
 * @return 0 if success, else non-zero
 */
uint16_t can_send(uint32_t id,
                  const uint8_t *data,
                  uint8_t dlc,
                  bool extended,
                  bool rtr,
                  uint32_t timeout_ms);

/**
 * @brief Read one CAN frame
 *
 * @param out_msg   output message
 * @param timeout_ms rx wait timeout in ms (0 = non-blocking)
 *
 * @return 0 if success, else non-zero (timeout 포함)
 */
uint16_t can_read(twai_message_t *out_msg, uint32_t timeout_ms);

/**
 * @brief Get TWAI status for debugging/monitoring
 * @return 0 if success, else non-zero
 */
uint16_t can_get_status(twai_status_info_t *out_status);

/**
 * @brief Recover from BUS-OFF / stuck state by restarting driver
 * @return 0 if success, else non-zero
 */
uint16_t can_recover(void);

#ifdef __cplusplus
}
#endif
