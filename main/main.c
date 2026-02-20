#include <stdio.h>

#include "common_data.h"
#include "error_code.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "p_gpio.h"
#include "p_rs232.h"
#include "p_rs485.h"
#include "p_i2c.h"
#include "p_spi.h"
#include "p_can.h"

#include "d_w5500.h"
#include "d_switch.h"
#include "d_ws2812.h"

#include "d_w5500/d_w5500_socket.h"
#include "d_w5500/d_w5500_net.h"

static void udp_echo_task(void *arg)
{
    const uint8_t sn = 0;
    const uint16_t local_port = 5000;

    // 1) 소켓 close (안전)
    w5500_sock_cmd(sn, W5500_CR_CLOSE, 100);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2) UDP 모드 + 포트 설정
    w5500_sn_write8(sn, W5500_Sn_MR, W5500_MR_UDP);
    w5500_sn_write16(sn, W5500_Sn_PORT, local_port);

    // 3) OPEN
    w5500_sock_cmd(sn, W5500_CR_OPEN, 100);

    ESP_LOGI("APP/UDP", "UDP echo started: port=%u", (unsigned)local_port);

    while (1) {
        if (!w5500_phy_link_up()) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        uint16_t rsr = 0;
        if (w5500_sock_get_rx_size(sn, &rsr) != SPI_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (rsr < 8) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // 현재 RX_RD 읽기 (이 포인터를 기준으로 peek)
        uint16_t rx_rd = 0;
        if (w5500_sn_read16(sn, W5500_Sn_RX_RD, &rx_rd) != SPI_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // UDP 헤더: [SRC IP 4][SRC PORT 2][DATA LEN 2]
        uint8_t hdr[8];
        if (w5500_sock_rx_peek(sn, rx_rd, hdr, sizeof(hdr)) != SPI_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        uint8_t  src_ip[4] = {hdr[0], hdr[1], hdr[2], hdr[3]};
        uint16_t src_port  = (uint16_t)((hdr[4] << 8) | hdr[5]);
        uint16_t data_len  = (uint16_t)((hdr[6] << 8) | hdr[7]);

        // 방어: data_len sanity + 프레임 전체가 RX_RSR 안에 들어왔는지 확인
        if (data_len == 0 || data_len > 2048 || rsr < (uint16_t)(8 + data_len)) {
            // 꼬였으면 일단 헤더만 소비해서 다음으로 넘기기(최소 손상)
            uint16_t new_rx_rd = (uint16_t)(rx_rd + 8);
            w5500_sock_rx_consume(sn, new_rx_rd, 100);
            ESP_LOGW("APP/UDP", "invalid frame rsr=%u data_len=%u", (unsigned)rsr, (unsigned)data_len);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // payload 읽기 (peek): 시작은 rx_rd + 8
        uint16_t payload_ptr = (uint16_t)(rx_rd + 8);

        uint8_t buf[512];
        uint16_t to_read = data_len;
        if (to_read > sizeof(buf)) to_read = sizeof(buf); // 테스트 제한

        if (w5500_sock_rx_peek(sn, payload_ptr, buf, to_read) != SPI_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // 이번 UDP 프레임 전체 소비: 헤더(8) + 실제 data_len(전체) 만큼
        uint16_t new_rx_rd = (uint16_t)(rx_rd + 8 + data_len);
        w5500_sock_rx_consume(sn, new_rx_rd, 100);

        ESP_LOGI("APP/UDP", "RX from %u.%u.%u.%u:%u len=%u",
                 src_ip[0], src_ip[1], src_ip[2], src_ip[3],
                 (unsigned)src_port, (unsigned)to_read);

        // 목적지 설정 + SEND
        w5500_sn_write_buf(sn, W5500_Sn_DIPR, src_ip, 4);
        w5500_sn_write16(sn, W5500_Sn_DPORT, src_port);

        uint16_t fsr = 0;
        if (w5500_sock_get_tx_free(sn, &fsr) == SPI_OK && fsr >= to_read) {
            w5500_sock_tx_write(sn, buf, to_read, NULL);
            w5500_sock_cmd(sn, W5500_CR_SEND, 500);
            ESP_LOGI("APP/UDP", "TX echo len=%u", (unsigned)to_read);
        } else {
            ESP_LOGW("APP/UDP", "TX no space (fsr=%u)", (unsigned)fsr);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


void app_main(void)
{
    printf("Boot Water v1.1\n");

    printf("Initialing System\n");
    //port init
    gpio_init();
    rs232_init();
    rs485_init();
    i2c_init();
    spi_init();
    can_init();

    //driver init
    w5500_init();
    ws2812_init();

    printf("Start Water-Link v1.0\n");
    xTaskCreate(udp_echo_task, "udp_echo", 4096, NULL, 5, NULL);

    while (1) {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}