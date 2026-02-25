#include "common_data.h"
#include "error_code.h"

#include "s_ping.h"
#include "d_w5500_socket.h"
#include "d_w5500_reg.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

#define TAG "SERVICE/PING"

// ICMP
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

static uint16_t icmp_checksum(const void *data, int len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t sum = 0;

    for (int i = 0; i < len; i += 2) {
        uint16_t w = ((uint16_t)p[i] << 8);
        if (i + 1 < len) w |= p[i + 1];
        sum += w;
    }
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static void wr16be(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)(v & 0xFF); }

// W5500 UDP/IPRAW RX 포맷(네가 UDP에서 이미 쓴 것):
// [SRC IP 4][SRC PORT 2][DATA LEN 2][DATA...]
// IPRAW에서도 동일하게 앞에 8바이트 헤더가 붙는다고 보는 게 제일 안전함.
static bool read_ipraw_packet(uint8_t sn, uint8_t *src_ip, uint16_t *data_len, uint8_t *data_buf, uint16_t data_buf_sz)
{
    uint16_t rsr = 0;
    if (w5500_sock_get_rx_size(sn, &rsr) != SPI_OK) return false;
    if (rsr < 8) return false;

    uint8_t hdr[8];
    uint16_t rd_after_hdr = 0;
    if (w5500_sock_rx_read(sn, hdr, 8, &rd_after_hdr) != SPI_OK) return false;

    src_ip[0] = hdr[0];
    src_ip[1] = hdr[1];
    src_ip[2] = hdr[2];
    src_ip[3] = hdr[3];

    // src_port는 ICMP에선 의미 없어서 무시
    uint16_t len = (uint16_t)((hdr[6] << 8) | hdr[7]);
    *data_len = len;

    if (len == 0) {
        w5500_sock_rx_consume(sn, rd_after_hdr, 100);
        return false;
    }

    uint16_t to_read = len;
    if (to_read > data_buf_sz) to_read = data_buf_sz;

    uint16_t rd_after_data = 0;
    if (w5500_sock_rx_read(sn, data_buf, to_read, &rd_after_data) != SPI_OK) {
        w5500_sock_rx_consume(sn, rd_after_hdr, 100);
        return false;
    }

    // 남는 데이터는 버림
    if (len > to_read) {
        uint16_t tmp = (uint16_t)(rd_after_data + (len - to_read));
        w5500_sock_rx_consume(sn, tmp, 100);
    } else {
        w5500_sock_rx_consume(sn, rd_after_data, 100);
    }

    // 만약 잘린 경우(버퍼 부족)엔 len을 실제 읽은 만큼으로
    *data_len = to_read;
    return true;
}

bool s_ping_once(uint8_t sn,
                 const uint8_t target_ip[4],
                 uint32_t timeout_ms,
                 s_ping_result_t *out)
{
    if (!target_ip) return false;

    // 결과 초기화
    s_ping_result_t dummy = { .id = 0xBEEF, .seq = 1, .rtt_ms = 0 };
    if (!out) out = &dummy;

    // 1) 소켓 정리
    w5500_sn_write8(sn, W5500_Sn_CR, W5500_CR_CLOSE);
    vTaskDelay(pdMS_TO_TICKS(5));

    // 2) IPRAW + ICMP(1)
    w5500_sn_write8(sn, W5500_Sn_MR, W5500_MR_IPRAW);
    w5500_sn_write8(sn, W5500_Sn_PROTO, 1); // ICMP

    // 3) 목적지 설정
    w5500_sn_write_buf(sn, W5500_Sn_DIPR, target_ip, 4);
    w5500_sn_write16(sn, W5500_Sn_DPORT, 0);

    // 4) OPEN
    if (w5500_sock_cmd(sn, W5500_CR_OPEN, 100) != SPI_OK) {
        ESP_LOGW(TAG, "OPEN failed");
        return false;
    }

    // 5) ICMP Echo Request 패킷 구성
    // ICMP 헤더: type(1) code(1) csum(2) id(2) seq(2) + payload
    uint8_t pkt[32];
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = ICMP_ECHO_REQUEST; // type
    pkt[1] = 0;                 // code
    wr16be(&pkt[4], out->id);
    wr16be(&pkt[6], out->seq);

    // payload는 아무거나(시간/패턴 넣어도 됨)
    for (int i = 8; i < (int)sizeof(pkt); i++) pkt[i] = (uint8_t)i;

    uint16_t csum = icmp_checksum(pkt, sizeof(pkt));
    wr16be(&pkt[2], csum);

    // 6) SEND
    int64_t t0 = esp_timer_get_time();
    w5500_sock_tx_write(sn, pkt, sizeof(pkt), NULL);
    if (w5500_sock_cmd(sn, W5500_CR_SEND, 500) != SPI_OK) {
        ESP_LOGW(TAG, "SEND cmd failed");
        w5500_sn_write8(sn, W5500_Sn_CR, W5500_CR_CLOSE);
        return false;
    }

    // 7) 응답 대기(폴링)
    const int64_t deadline = t0 + (int64_t)timeout_ms * 1000;

    while (esp_timer_get_time() < deadline) {
        uint8_t src_ip[4];
        uint16_t data_len = 0;
        uint8_t rx[256];

        if (read_ipraw_packet(sn, src_ip, &data_len, rx, sizeof(rx))) {
            ESP_LOGI(TAG, "RX RAW len=%u first bytes=%02X %02X %02X %02X", data_len, rx[0], rx[1], rx[2], rx[3]);

            // 대상 IP에서 온 것만 처리(브로드캐스트/잡패킷 방지)
            if (memcmp(src_ip, target_ip, 4) != 0) {
                continue;
            }

            // 일부 환경에서 IP 헤더가 붙어올 수도 있어서(구현 차이/레지스터 설정 꼬임 대비)
            // 1) ICMP 헤더가 바로 오는 경우
            // 2) IPv4 헤더(0x45...) 뒤에 ICMP가 오는 경우
            const uint8_t *icmp = rx;
            int icmp_len = (int)data_len;

            if (icmp_len >= 20 && (rx[0] >> 4) == 4) {
                // IPv4 header
                int ihl = (rx[0] & 0x0F) * 4;
                if (ihl >= 20 && icmp_len > ihl) {
                    icmp = rx + ihl;
                    icmp_len -= ihl;
                }
            }

            if (icmp_len >= 8) {
                uint8_t type = icmp[0];
                uint8_t code = icmp[1];
                uint16_t rid = be16(&icmp[4]);
                uint16_t rseq = be16(&icmp[6]);

                if (type == ICMP_ECHO_REPLY && code == 0 && rid == out->id && rseq == out->seq) {
                    int64_t t1 = esp_timer_get_time();
                    out->rtt_ms = (uint32_t)((t1 - t0) / 1000);
                    w5500_sn_write8(sn, W5500_Sn_CR, W5500_CR_CLOSE);
                    return true;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 타임아웃
    w5500_sn_write8(sn, W5500_Sn_CR, W5500_CR_CLOSE);
    return false;
}
