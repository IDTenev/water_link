#include <string.h>
#include <string.h>
#include <stdio.h>

#include "common_data.h"
#include "error_code.h"
#include "log_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "p_can.h"
#include "d_pipette_can.h"

#define TAG_CAN "DRIVER/PIPETTE/CAN"

#define PIPETTE_CAN_EX1_TX_ID              0x401
#define PIPETTE_CAN_EX1_TX_DLC             0
#define PIPETTE_CAN_EX1_RX_TIMEOUT_MS      1000

#define PIPETTE_CAN_EX2_TX_ID              0x001
#define PIPETTE_CAN_EX2_TX_DLC             8

static const uint8_t s_ex2_start_data[8] = {
    0x52, 0x46, 0x69, 0x64, 0x30, 0x30, 0x30, 0xA1
};

static bool s_streaming = false;

bool pipette_can_is_streaming(void)
{
    return s_streaming;
}

pipette_can_result_t pipette_can_example1(uint8_t *out_data,
                                          uint16_t *out_data_len,
                                          uint16_t out_buf_size)
{
    if (!out_data || !out_data_len) {
        return PIPETTE_CAN_ERR_NULL;
    }

    *out_data_len = 0;

    if (out_buf_size < (1 + 2 + 1 + 8)) {
        return PIPETTE_CAN_ERR_FORMAT;
    }

    ESP_LOGI(TAG_CAN, "CAN Send : [%03lX] (no data)", PIPETTE_CAN_EX1_TX_ID);
    uint16_t rc = can_send(
        PIPETTE_CAN_EX1_TX_ID,
        NULL,
        PIPETTE_CAN_EX1_TX_DLC,
        false,
        false,
        100
    );

    if (rc != 0) {
        out_data[0] = 0;
        out_data[1] = 0xE1;
        *out_data_len = 2;
        return PIPETTE_CAN_ERR_SEND;
    }

    twai_message_t msg;
    memset(&msg, 0, sizeof(msg));

    rc = can_read(&msg, PIPETTE_CAN_EX1_RX_TIMEOUT_MS);
    if (rc != 0) {
        log_can(TAG_CAN, "CAN Read :", msg.identifier, msg.data, msg.data_length_code);
        out_data[0] = 0;
        out_data[1] = 0xE2;
        *out_data_len = 2;
        return PIPETTE_CAN_ERR_TIMEOUT;
    }

    out_data[0] = 1;
    out_data[1] = (uint8_t)((msg.identifier >> 8) & 0xFF);
    out_data[2] = (uint8_t)(msg.identifier & 0xFF);
    out_data[3] = msg.data_length_code;

    if (msg.data_length_code > 8) {
        out_data[0] = 0;
        out_data[1] = 0xE3;
        *out_data_len = 2;
        return PIPETTE_CAN_ERR_FORMAT;
    }

    memcpy(&out_data[4], msg.data, msg.data_length_code);
    *out_data_len = (uint16_t)(4 + msg.data_length_code);

    return PIPETTE_CAN_OK;
}

pipette_can_result_t pipette_can_example2_start(void)
{
    uint16_t rc = can_send(
        PIPETTE_CAN_EX2_TX_ID,
        s_ex2_start_data,
        PIPETTE_CAN_EX2_TX_DLC,
        false,
        false,
        100
    );

    if (rc != 0) {
        return PIPETTE_CAN_ERR_SEND;
    }

    s_streaming = true;
    return PIPETTE_CAN_OK;
}

pipette_can_result_t pipette_can_example2_stop(void)
{
    s_streaming = false;
    return PIPETTE_CAN_OK;
}

pipette_can_result_t pipette_can_poll_stream_rx(twai_message_t *out_msg, bool *out_ready)
{
    if (!out_msg || !out_ready) {
        return PIPETTE_CAN_ERR_NULL;
    }

    *out_ready = false;

    if (!s_streaming) {
        return PIPETTE_CAN_ERR_NOT_STREAMING;
    }

    uint16_t rc = can_read(out_msg, 0);   // non-blocking
    if (rc != 0) {
        return PIPETTE_CAN_OK;
    }

    *out_ready = true;
    return PIPETTE_CAN_OK;
}