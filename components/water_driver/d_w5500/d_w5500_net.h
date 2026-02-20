#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t sn[4];   // subnet mask
    uint8_t gw[4];   // gateway
    uint8_t dns[4];  // optional (W5500엔 DNS 레지스터는 없고 보통 서비스 레이어가 씀)
} w5500_netinfo_t;

// 공통 네트워크 정보 설정/조회
uint16_t w5500_net_set(const w5500_netinfo_t *ni);
uint16_t w5500_net_get(w5500_netinfo_t *ni);

// 재전송/타임아웃 튜닝 (Phase2에서 체감 큼)
uint16_t w5500_net_set_retry(uint16_t rtr_ms, uint8_t rcr);
uint16_t w5500_net_get_retry(uint16_t *rtr_ms, uint8_t *rcr);

// PHY 링크 상태/설정
bool     w5500_phy_link_up(void);

// 소켓 TX/RX 버퍼 할당(각 소켓 KB 단위: 0/1/2/4/8/16)
// 예: tx_kb[8]={2,2,2,2,0,0,0,0} 이런 식
uint16_t w5500_buf_alloc(const uint8_t tx_kb[8], const uint8_t rx_kb[8]);

void w5500_net_dump(void);

#ifdef __cplusplus
}
#endif
