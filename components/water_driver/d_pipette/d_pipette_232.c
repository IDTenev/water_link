#include <string.h>
#include <stdio.h>

#include "common_data.h"
#include "error_code.h"
#include "log_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "d_pipette_232.h"
#include "p_rs232.h"

#define TAG_232 "DRIVER/PIPETTE/232"

static uint16_t read_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static void write_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)((v >> 8) & 0xFF);
    p[1] = (uint8_t)(v & 0xFF);
}

void pipette232_init_parser(pipette232_parser_t *parser)
{
    if (!parser) return;
    memset(parser, 0, sizeof(*parser));
}

uint16_t pipette232_crc16_ccitt_false(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;

    if (!data && len > 0) return 0;

    for (size_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
            else              crc = (uint16_t)(crc << 1);
        }
    }

    return crc;
}

pipette232_result_t pipette232_build_frame(
    uint16_t cmd,
    const uint8_t *data,
    uint16_t data_len,
    uint8_t *out_frame,
    size_t out_frame_size,
    size_t *out_frame_len
)
{
    if (!out_frame || !out_frame_len) return PIPETTE232_ERR_NULL;
    if (data_len > PIPETTE232_MAX_DATA_LEN) return PIPETTE232_ERR_ARG;
    if (data_len > 0 && !data) return PIPETTE232_ERR_NULL;

    size_t total_len = 2 + 2 + 2 + data_len + 2 + 2;
    if (out_frame_size < total_len) return PIPETTE232_ERR_OVERFLOW;

    out_frame[0] = PIPETTE232_STX_H;
    out_frame[1] = PIPETTE232_STX_L;
    write_be16(&out_frame[2], cmd);
    write_be16(&out_frame[4], data_len);

    if (data_len > 0) {
        memcpy(&out_frame[6], data, data_len);
    }

    uint16_t crc = pipette232_crc16_ccitt_false(&out_frame[2], 2 + 2 + data_len);
    write_be16(&out_frame[6 + data_len], crc);

    out_frame[8 + data_len] = PIPETTE232_ETX_H;
    out_frame[9 + data_len] = PIPETTE232_ETX_L;

    *out_frame_len = total_len;
    return PIPETTE232_OK;
}

pipette232_result_t pipette232_send_packet(
    uint16_t cmd,
    const uint8_t *data,
    uint16_t data_len
)
{
    uint8_t frame[PIPETTE232_MAX_FRAME_LEN];
    size_t frame_len = 0;

    pipette232_result_t rc = pipette232_build_frame(
        cmd, data, data_len, frame, sizeof(frame), &frame_len
    );
    if (rc != PIPETTE232_OK) {
        return rc;
    }
    
    log_hex(TAG_232, "232 Send :", frame, frame_len);
    size_t sent = rs232_send(frame, frame_len);
    if (sent != frame_len) {
        return PIPETTE232_ERR_SEND;
    }

    return PIPETTE232_OK;
}

pipette232_result_t pipette232_send_result(
    uint16_t cmd,
    uint8_t result
)
{
    return pipette232_send_packet(cmd, &result, 1);
}

pipette232_result_t pipette232_parse_frame(
    const uint8_t *frame,
    size_t frame_len,
    pipette232_packet_t *out_packet
)
{
    if (!frame || !out_packet) return PIPETTE232_ERR_NULL;
    if (frame_len < 10) return PIPETTE232_ERR_FORMAT;

    if (frame[0] != PIPETTE232_STX_H || frame[1] != PIPETTE232_STX_L) {
        return PIPETTE232_ERR_STX;
    }

    uint16_t data_len = read_be16(&frame[4]);
    if (data_len > PIPETTE232_MAX_DATA_LEN) {
        return PIPETTE232_ERR_OVERFLOW;
    }

    size_t expected_len = 2 + 2 + 2 + data_len + 2 + 2;
    if (frame_len != expected_len) {
        return PIPETTE232_ERR_FORMAT;
    }

    if (frame[8 + data_len] != PIPETTE232_ETX_H || frame[9 + data_len] != PIPETTE232_ETX_L) {
        return PIPETTE232_ERR_ETX;
    }

    uint16_t rx_crc = read_be16(&frame[6 + data_len]);
    uint16_t calc_crc = pipette232_crc16_ccitt_false(&frame[2], 2 + 2 + data_len);

    if (rx_crc != calc_crc) {
        return PIPETTE232_ERR_CRC;
    }

    out_packet->cmd = read_be16(&frame[2]);
    out_packet->data_len = data_len;

    if (data_len > 0) {
        memcpy(out_packet->data, &frame[6], data_len);
    }

    return PIPETTE232_OK;
}

pipette232_result_t pipette232_feed_parser(
    pipette232_parser_t *parser,
    uint8_t byte,
    pipette232_packet_t *out_packet,
    bool *out_ready
)
{
    if (!parser || !out_packet || !out_ready) return PIPETTE232_ERR_NULL;

    *out_ready = false;

    if (parser->idx == 0) {
        if (byte == PIPETTE232_STX_H) {
            parser->buf[parser->idx++] = byte;
        }
        return PIPETTE232_OK;
    }

    if (parser->idx == 1) {
        if (byte == PIPETTE232_STX_L) {
            parser->buf[parser->idx++] = byte;
        } else if (byte == PIPETTE232_STX_H) {
            parser->buf[0] = byte;
            parser->idx = 1;
        } else {
            parser->idx = 0;
            parser->expected_len = 0;
        }
        return PIPETTE232_OK;
    }

    if (parser->idx >= PIPETTE232_MAX_FRAME_LEN) {
        pipette232_init_parser(parser);
        return PIPETTE232_ERR_OVERFLOW;
    }

    parser->buf[parser->idx++] = byte;

    if (parser->idx == 6) {
        uint16_t data_len = read_be16(&parser->buf[4]);
        if (data_len > PIPETTE232_MAX_DATA_LEN) {
            pipette232_init_parser(parser);
            return PIPETTE232_ERR_OVERFLOW;
        }
        parser->expected_len = 2 + 2 + 2 + data_len + 2 + 2;
    }

    if (parser->expected_len > 0 && parser->idx == parser->expected_len) {
        pipette232_result_t rc = pipette232_parse_frame(parser->buf, parser->expected_len, out_packet);
        pipette232_init_parser(parser);

        if (rc == PIPETTE232_OK) {
            *out_ready = true;
        }
        return rc;
    }

    return PIPETTE232_OK;
}

pipette232_result_t pipette232_poll_packet(
    pipette232_parser_t *parser,
    pipette232_packet_t *out_packet,
    bool *out_ready
)
{
    if (!parser || !out_packet || !out_ready) return PIPETTE232_ERR_NULL;

    *out_ready = false;

    size_t avail = rs232_available();
    if (avail == 0) {
        return PIPETTE232_OK;
    }

    uint8_t rx_buf[128];
    if (avail > sizeof(rx_buf)) {
        avail = sizeof(rx_buf);
    }

    size_t rx_len = rs232_read(rx_buf, avail);
    if (rx_len > 0) {
        log_hex(TAG_232, "232 Read :", rx_buf, rx_len);
    }
    for (size_t i = 0; i < rx_len; i++) {
        pipette232_result_t rc = pipette232_feed_parser(parser, rx_buf[i], out_packet, out_ready);

        if (rc != PIPETTE232_OK) {
            *out_ready = false;
            return rc;
        }

        if (*out_ready) {
            return PIPETTE232_OK;
        }
    }

    return PIPETTE232_OK;
}

pipette232_result_t pipette232_send_subcmd_result(
    uint8_t sub_cmd,
    uint8_t result
)
{
    uint8_t data[2];
    data[0] = sub_cmd;
    data[1] = result;

    return pipette232_send_packet(
        PIPETTE232_CMD_EXAMPLE,
        data,
        2
    );
}