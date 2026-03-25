#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "driver/twai.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIPETTE_CAN_OK = 0,
    PIPETTE_CAN_ERR_NULL,
    PIPETTE_CAN_ERR_SEND,
    PIPETTE_CAN_ERR_RECV,
    PIPETTE_CAN_ERR_TIMEOUT,
    PIPETTE_CAN_ERR_FORMAT,
    PIPETTE_CAN_ERR_NOT_STREAMING,
} pipette_can_result_t;

pipette_can_result_t pipette_can_example1(uint8_t *out_data,
                                          uint16_t *out_data_len,
                                          uint16_t out_buf_size);

// example2: 시작 CAN 송신 + streaming on
pipette_can_result_t pipette_can_example2_start(void);

// example3: streaming off
pipette_can_result_t pipette_can_example2_stop(void);

// streaming 상태일 때 CAN RX 하나 polling
pipette_can_result_t pipette_can_poll_stream_rx(twai_message_t *out_msg, bool *out_ready);

bool pipette_can_is_streaming(void);

#ifdef __cplusplus
}
#endif