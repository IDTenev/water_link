#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t id;
    uint16_t seq;
    uint32_t rtt_ms;   // 성공 시 왕복시간(ms)
} s_ping_result_t;

// sn: 사용할 소켓 번호 (0~7 중 하나, 다른 서비스와 겹치지 않게)
// target_ip: 대상 IP
// timeout_ms: 응답 대기 시간
bool s_ping_once(uint8_t sn,
                 const uint8_t target_ip[4],
                 uint32_t timeout_ms,
                 s_ping_result_t *out);
