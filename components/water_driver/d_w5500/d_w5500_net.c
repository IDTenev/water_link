#include "common_data.h"
#include "error_code.h"
#include "d_w5500_net.h"
#include "d_w5500_reg.h"
#include "d_w5500_socket.h"

#include <string.h>

#include "esp_log.h"

#define TAG_NET "DRIVER/W5500/NET"
// PHYCFGR bits (datasheet)
// bit0: LINK (1=up)
// bit1: SPD  (1=100M)
// bit2: DPX  (1=full)
// bit3: OPMDC
// bit4: OPMDC
// bit5: OPMDC
// bit6: RESERVED
// bit7: RST  (1=software reset PHY?) (chip dependent; we only read link here)

static uint16_t common_read_buf(uint16_t addr, uint8_t *buf, size_t len)
{
    return w5500_read_buf(W5500_BSB_COMMON, addr, buf, len);
}
static uint16_t common_write_buf(uint16_t addr, const uint8_t *buf, size_t len)
{
    return w5500_write_buf(W5500_BSB_COMMON, addr, buf, len);
}
static uint16_t common_read8(uint16_t addr, uint8_t *out)
{
    return w5500_read8(W5500_BSB_COMMON, addr, out);
}
static uint16_t common_write8(uint16_t addr, uint8_t v)
{
    return w5500_write8(W5500_BSB_COMMON, addr, v);
}
static uint16_t common_read16(uint16_t addr, uint16_t *out)
{
    return w5500_read16(W5500_BSB_COMMON, addr, out);
}
static uint16_t common_write16(uint16_t addr, uint16_t v)
{
    return w5500_write16(W5500_BSB_COMMON, addr, v);
}

uint16_t w5500_net_set(const w5500_netinfo_t *ni)
{
    if (!ni) return SPI_ERROR_ARGUMENT;

    uint16_t r;
    r = common_write_buf(W5500_SHAR, ni->mac, 6); if (r != SPI_OK) return r;
    r = common_write_buf(W5500_GAR,  ni->gw,  4); if (r != SPI_OK) return r;
    r = common_write_buf(W5500_SUBR, ni->sn,  4); if (r != SPI_OK) return r;
    r = common_write_buf(W5500_SIPR, ni->ip,  4); if (r != SPI_OK) return r;

    return SPI_OK;
}

uint16_t w5500_net_get(w5500_netinfo_t *ni)
{
    if (!ni) return SPI_ERROR_ARGUMENT;

    uint16_t r;
    r = common_read_buf(W5500_SHAR, ni->mac, 6); if (r != SPI_OK) return r;
    r = common_read_buf(W5500_GAR,  ni->gw,  4); if (r != SPI_OK) return r;
    r = common_read_buf(W5500_SUBR, ni->sn,  4); if (r != SPI_OK) return r;
    r = common_read_buf(W5500_SIPR, ni->ip,  4); if (r != SPI_OK) return r;

    // DNS는 W5500 공통 레지스터에 없음 (보통 DHCP/DNS는 상위 서비스가 관리)
    memset(ni->dns, 0, 4);
    return SPI_OK;
}

// rtr_ms: 밀리초 단위로 받지만, W5500 RTR 레지스터 단위는 100us.
// 즉 RTR = rtr_ms * 10 (최대값 체크는 사용자가 적당히)
uint16_t w5500_net_set_retry(uint16_t rtr_ms, uint8_t rcr)
{
    uint16_t rtr = (uint16_t)(rtr_ms * 10);
    uint16_t r;
    r = common_write16(W5500_RTR, rtr); if (r != SPI_OK) return r;
    r = common_write8(W5500_RCR, rcr); if (r != SPI_OK) return r;
    return SPI_OK;
}

uint16_t w5500_net_get_retry(uint16_t *rtr_ms, uint8_t *rcr)
{
    if (!rtr_ms || !rcr) return SPI_ERROR_ARGUMENT;

    uint16_t rtr = 0;
    uint8_t  rr  = 0;
    uint16_t r;

    r = common_read16(W5500_RTR, &rtr); if (r != SPI_OK) return r;
    r = common_read8(W5500_RCR, &rr);   if (r != SPI_OK) return r;

    *rtr_ms = (uint16_t)(rtr / 10);
    *rcr = rr;
    return SPI_OK;
}

bool w5500_phy_link_up(void)
{
    uint8_t phy = 0;
    if (common_read8(W5500_PHYCFGR, &phy) != SPI_OK) return false;
    return (phy & 0x01) ? true : false;
}

// 소켓 버퍼 크기 설정: Sn_TXBUF_SIZE / Sn_RXBUF_SIZE (KB)
// 주의: 총합은 16KB를 넘기면 안 됨 (W5500 내부 메모리 한계)
static bool valid_kb(uint8_t kb)
{
    return (kb == 0 || kb == 1 || kb == 2 || kb == 4 || kb == 8 || kb == 16);
}

uint16_t w5500_buf_alloc(const uint8_t tx_kb[8], const uint8_t rx_kb[8])
{
    if (!tx_kb || !rx_kb) return SPI_ERROR_ARGUMENT;

    uint16_t sum_tx = 0, sum_rx = 0;

    for (int i = 0; i < 8; i++) {
        if (!valid_kb(tx_kb[i]) || !valid_kb(rx_kb[i])) return SPI_ERROR_ARGUMENT;
        sum_tx += tx_kb[i];
        sum_rx += rx_kb[i];
    }

    // W5500 total TX=16KB, RX=16KB (각각 별도 풀)
    if (sum_tx > 16 || sum_rx > 16) return SPI_ERROR_ARGUMENT;

    // 각 소켓에 기록
    for (int sn = 0; sn < 8; sn++) {
        uint16_t r = w5500_sn_write8((uint8_t)sn, W5500_Sn_TXBUF_SIZE, tx_kb[sn]);
        if (r != SPI_OK) return r;
        r = w5500_sn_write8((uint8_t)sn, W5500_Sn_RXBUF_SIZE, rx_kb[sn]);
        if (r != SPI_OK) return r;
    }

    return SPI_OK;
}

static void print_ip(const char* label, const uint8_t ip[4])
{
    ESP_LOGI(TAG_NET, "  %s : %u.%u.%u.%u",
             label, ip[0], ip[1], ip[2], ip[3]);
}

static void print_mac(const uint8_t mac[6])
{
    ESP_LOGI(TAG_NET, "  MAC : %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void w5500_net_dump(void)
{
    w5500_netinfo_t ni;
    uint16_t rtr = 0;
    uint8_t  rcr = 0;

    if (w5500_net_get(&ni) != SPI_OK) {
        ESP_LOGW(TAG_NET, "net_get failed");
        return;
    }

    w5500_net_get_retry(&rtr, &rcr);

    ESP_LOGI(TAG_NET, "W5500 NET INFO ----------------");

    print_mac(ni.mac);
    print_ip("IP ", ni.ip);
    print_ip("SN ", ni.sn);
    print_ip("GW ", ni.gw);

    bool link = w5500_phy_link_up();

    // PHY 상세 읽기
    uint8_t phy = 0;
    if (w5500_read8(W5500_BSB_COMMON, 0x002E, &phy) == SPI_OK) {
        const char* spd = (phy & 0x02) ? "100M" : "10M";
        const char* dpx = (phy & 0x04) ? "Full" : "Half";

        ESP_LOGI(TAG_NET, "  PHY : %s (%s %s)",
                 link ? "UP" : "DOWN", spd, dpx);
    } else {
        ESP_LOGI(TAG_NET, "  PHY : %s", link ? "UP" : "DOWN");
    }

    ESP_LOGI(TAG_NET, "  RTR : %u ms", rtr);
    ESP_LOGI(TAG_NET, "  RCR : %u", rcr);
    ESP_LOGI(TAG_NET, "--------------------------------");
}