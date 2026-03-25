#include <stdio.h>

#include "common_data.h"
#include "error_code.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "log_utils.h"

#include "p_gpio.h"
#include "p_rs232.h"
#include "p_can.h"

#include "d_pipette_232.h"
#include "d_pipette_can.h"

#define TAG "APP/MAIN"

static void send_example_ack(uint8_t sub_cmd, uint8_t result, uint8_t error_code, bool has_error)
{
    uint8_t payload[4];
    uint16_t len = 0;

    payload[len++] = sub_cmd;
    payload[len++] = 0x01;   // type = ack
    payload[len++] = result; // 1=success, 0=fail

    if (has_error) {
        payload[len++] = error_code;
    }

    pipette232_send_packet(PIPETTE232_CMD_EXAMPLE, payload, len);
}

static void handle_pipette_packet(const pipette232_packet_t *pkt)
{
    if (!pkt) return;

    if (pkt->cmd != PIPETTE232_CMD_EXAMPLE) {
        return;
    }

    if (pkt->data_len < 1) {
        send_example_ack(0x00, 0, 0xF0, true);
        return;
    }

    uint8_t sub_cmd = pkt->data[0];

    switch (sub_cmd) {

    case PIPETTE232_SUBCMD_TEST_1:
    {
        uint8_t tx_data[16];
        uint16_t tx_len = 0;

        pipette_can_result_t rc = pipette_can_example1(tx_data, &tx_len, sizeof(tx_data));

        uint8_t payload[1 + 16];
        payload[0] = sub_cmd;

        if (tx_len > sizeof(payload) - 1) {
            send_example_ack(sub_cmd, 0, 0xF1, true);
            break;
        }

        memcpy(&payload[1], tx_data, tx_len);
        pipette232_send_packet(PIPETTE232_CMD_EXAMPLE, payload, (uint16_t)(1 + tx_len));
        break;
    }

    case PIPETTE232_SUBCMD_TEST_2:
    {
        pipette_can_result_t rc = pipette_can_example2_start();
        if (rc == PIPETTE_CAN_OK) {
            send_example_ack(sub_cmd, 1, 0, false);
        } else {
            send_example_ack(sub_cmd, 0, 0xE1, true);
        }
        break;
    }

    case PIPETTE232_SUBCMD_TEST_3:
    {
        pipette_can_result_t rc = pipette_can_example2_stop();
        if (rc == PIPETTE_CAN_OK) {
            send_example_ack(sub_cmd, 1, 0, false);
        } else {
            send_example_ack(sub_cmd, 0, 0xE2, true);
        }
        break;
    }

    case PIPETTE232_SUBCMD_TEST_4:
    {
        send_example_ack(sub_cmd, 1, 0, false);
        break;
    }

    default:
    {
        send_example_ack(sub_cmd, 0, 0xFF, true);
        break;
    }
    }
}

void app_main(void)
{
    printf("Boot Water v1.1\n");
    printf("Initialing System\n");

    gpio_init();
    rs232_init();

    uint16_t rc = can_init();
    if (rc) {
        ESP_LOGE(TAG, "can_init failed: 0x%04X", rc);
        return;
    }

    rs232_flush();

    printf("Start Water-Link v1.0\n");

    pipette232_parser_t parser;
    pipette232_init_parser(&parser);

    while (1) {
        pipette232_packet_t pkt;
        bool ready = false;

        pipette232_result_t prc = pipette232_poll_packet(&parser, &pkt, &ready);
        if (prc != PIPETTE232_OK) {
            ESP_LOGW(TAG, "pipette232 poll error: %d", prc);
        }

        if (ready) {
            handle_pipette_packet(&pkt);
        }

        if (pipette_can_is_streaming()) {
            twai_message_t msg;
            bool rx_ready = false;

            pipette_can_result_t crc = pipette_can_poll_stream_rx(&msg, &rx_ready);
            if (crc == PIPETTE_CAN_OK && rx_ready) {
                log_can(TAG, "CAN Read :", msg.identifier, msg.data, msg.data_length_code);
                uint8_t payload[1 + 1 + 2 + 1 + 8];
                uint16_t len = 0;

                payload[len++] = PIPETTE232_SUBCMD_TEST_2;
                payload[len++] = 0x02;  // type = stream
                payload[len++] = (uint8_t)((msg.identifier >> 8) & 0xFF);
                payload[len++] = (uint8_t)(msg.identifier & 0xFF);
                payload[len++] = msg.data_length_code;

                if (msg.data_length_code <= 8) {
                    memcpy(&payload[len], msg.data, msg.data_length_code);
                    len += msg.data_length_code;

                    pipette232_send_packet(PIPETTE232_CMD_EXAMPLE, payload, len);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}