#include "common_data.h"
#include "error_code.h"

#include "s_udp.h"
#include "d_w5500/d_w5500_socket.h"
#include "d_w5500/d_w5500_net.h"    // phy link check

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#define TAG "SERVICE/UDP"

static inline bool sn_valid(uint8_t sn) { return sn < 8; }

uint16_t s_udp_open(s_udp_handle_t *h, uint8_t sn, uint16_t local_port)
{
    if (!h || !sn_valid(sn) || local_port == 0) return SPI_ERROR_ARGUMENT;

    h->sn = sn;
    h->local_port = local_port;
    h->opened = false;

    // close -> UDP -> bind port -> open
    (void)w5500_sock_cmd(sn, W5500_CR_CLOSE, 100);
    vTaskDelay(pdMS_TO_TICKS(5));

    uint16_t r;
    r = w5500_sn_write8(sn, W5500_Sn_MR, W5500_MR_UDP);
    if (r != SPI_OK) return r;

    r = w5500_sn_write16(sn, W5500_Sn_PORT, local_port);
    if (r != SPI_OK) return r;

    r = w5500_sock_cmd(sn, W5500_CR_OPEN, 200);
    if (r != SPI_OK) return r;

    h->opened = true;
    ESP_LOGI(TAG, "open sn=%u port=%u", (unsigned)sn, (unsigned)local_port);
    return SPI_OK;
}

uint16_t s_udp_close(s_udp_handle_t *h)
{
    if (!h || !sn_valid(h->sn)) return SPI_ERROR_ARGUMENT;
    (void)w5500_sock_cmd(h->sn, W5500_CR_CLOSE, 100);
    h->opened = false;
    return SPI_OK;
}

int32_t s_udp_recvfrom(s_udp_handle_t *h, s_udp_peer_t *from, uint8_t *buf, size_t buf_size)
{
    if (!h || !h->opened || !sn_valid(h->sn) || !from || !buf || buf_size == 0) return -1;

    const uint8_t sn = h->sn;

    // 링크가 안 올라왔으면 그냥 "없음" 취급 (busy loop 방지)
    if (!w5500_phy_link_up()) return 0;

    uint16_t rsr = 0;
    if (w5500_sock_get_rx_size(sn, &rsr) != SPI_OK) return -1;
    if (rsr < 8) return 0; // 헤더도 없음

    // RX_RD 기준 peek 방식
    uint16_t rx_rd = 0;
    if (w5500_sn_read16(sn, W5500_Sn_RX_RD, &rx_rd) != SPI_OK) return -1;

    uint8_t hdr[8];
    if (w5500_sock_rx_peek(sn, rx_rd, hdr, sizeof(hdr)) != SPI_OK) return -1;

    from->ip[0] = hdr[0];
    from->ip[1] = hdr[1];
    from->ip[2] = hdr[2];
    from->ip[3] = hdr[3];
    from->port  = (uint16_t)((hdr[4] << 8) | hdr[5]);

    uint16_t data_len = (uint16_t)((hdr[6] << 8) | hdr[7]);

    // 패킷 전체가 들어왔는지 확인
    if (data_len == 0 || data_len > 2048) {
        // 최소 손상: 헤더만 소비해서 다음으로
        (void)w5500_sock_rx_consume(sn, (uint16_t)(rx_rd + 8), 100);
        return -1;
    }

    if (rsr < (uint16_t)(8 + data_len)) {
        // 아직 덜 들어옴
        return 0;
    }

    uint16_t payload_ptr = (uint16_t)(rx_rd + 8);

    uint16_t copy_len = data_len;
    if (copy_len > buf_size) copy_len = (uint16_t)buf_size;

    if (w5500_sock_rx_peek(sn, payload_ptr, buf, copy_len) != SPI_OK) return -1;

    // 프레임 전체 소비 (남은 payload도 포함)
    (void)w5500_sock_rx_consume(sn, (uint16_t)(rx_rd + 8 + data_len), 100);

    // payload가 buf_size보다 컸으면 남은 건 버린 것(프레임 소비로 처리)
    return (int32_t)copy_len;
}

uint16_t s_udp_sendto(s_udp_handle_t *h, const s_udp_peer_t *to, const uint8_t *buf, size_t len)
{
    if (!h || !h->opened || !sn_valid(h->sn) || !to || !buf || len == 0) return SPI_ERROR_ARGUMENT;

    const uint8_t sn = h->sn;

    if (!w5500_phy_link_up()) return SPI_ERROR;

    // 목적지 세팅
    uint16_t r;
    r = w5500_sn_write_buf(sn, W5500_Sn_DIPR, to->ip, 4);
    if (r != SPI_OK) return r;

    r = w5500_sn_write16(sn, W5500_Sn_DPORT, to->port);
    if (r != SPI_OK) return r;

    // TX 공간 확인
    uint16_t fsr = 0;
    r = w5500_sock_get_tx_free(sn, &fsr);
    if (r != SPI_OK) return r;
    if (fsr < (uint16_t)len) return SPI_ERROR;

    // TX write + SEND
    r = w5500_sock_tx_write(sn, buf, len, NULL);
    if (r != SPI_OK) return r;

    r = w5500_sock_cmd(sn, W5500_CR_SEND, 500);
    return r;
}
