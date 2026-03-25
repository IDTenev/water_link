#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIPETTE232_STX_H            0x55
#define PIPETTE232_STX_L            0x55
#define PIPETTE232_ETX_H            0xAA
#define PIPETTE232_ETX_L            0xAA

#define PIPETTE232_MAX_DATA_LEN     256
#define PIPETTE232_MAX_FRAME_LEN    (2 + 2 + 2 + PIPETTE232_MAX_DATA_LEN + 2 + 2)

typedef enum {
    PIPETTE232_OK = 0,
    PIPETTE232_ERR_NULL,
    PIPETTE232_ERR_ARG,
    PIPETTE232_ERR_OVERFLOW,
    PIPETTE232_ERR_STX,
    PIPETTE232_ERR_ETX,
    PIPETTE232_ERR_CRC,
    PIPETTE232_ERR_FORMAT,
    PIPETTE232_ERR_SEND,
} pipette232_result_t;

typedef enum {
    PIPETTE232_CMD_EXAMPLE = 0x0001,
} pipette232_cmd_t;

typedef enum {
    PIPETTE232_SUBCMD_TEST_1 = 0x01,
    PIPETTE232_SUBCMD_TEST_2 = 0x02,
    PIPETTE232_SUBCMD_TEST_3 = 0x03,
    PIPETTE232_SUBCMD_TEST_4 = 0x04,
} pipette232_subcmd_t;

typedef struct {
    uint16_t cmd;
    uint16_t data_len;
    uint8_t  data[PIPETTE232_MAX_DATA_LEN];
} pipette232_packet_t;

typedef struct {
    uint8_t buf[PIPETTE232_MAX_FRAME_LEN];
    size_t idx;
    size_t expected_len;
} pipette232_parser_t;

void pipette232_init_parser(pipette232_parser_t *parser);

uint16_t pipette232_crc16_ccitt_false(const uint8_t *data, size_t len);

pipette232_result_t pipette232_build_frame(
    uint16_t cmd,
    const uint8_t *data,
    uint16_t data_len,
    uint8_t *out_frame,
    size_t out_frame_size,
    size_t *out_frame_len
);

pipette232_result_t pipette232_send_packet(
    uint16_t cmd,
    const uint8_t *data,
    uint16_t data_len
);

pipette232_result_t pipette232_send_result(
    uint16_t cmd,
    uint8_t result
);

pipette232_result_t pipette232_parse_frame(
    const uint8_t *frame,
    size_t frame_len,
    pipette232_packet_t *out_packet
);

pipette232_result_t pipette232_feed_parser(
    pipette232_parser_t *parser,
    uint8_t byte,
    pipette232_packet_t *out_packet,
    bool *out_ready
);

pipette232_result_t pipette232_poll_packet(
    pipette232_parser_t *parser,
    pipette232_packet_t *out_packet,
    bool *out_ready
);

pipette232_result_t pipette232_send_subcmd_result(
    uint8_t sub_cmd,
    uint8_t result
);

#ifdef __cplusplus
}
#endif