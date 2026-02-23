#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  ip[4];
    uint16_t port;
} s_udp_peer_t;

typedef struct {
    uint8_t  sn;          // W5500 socket number 0..7
    uint16_t local_port;  // bound port
    bool     opened;
} s_udp_handle_t;

// UDP 소켓 열기 (W5500 socket 하나 점유)
uint16_t s_udp_open(s_udp_handle_t *h, uint8_t sn, uint16_t local_port);

// UDP 소켓 닫기
uint16_t s_udp_close(s_udp_handle_t *h);

// 수신 대기(논블로킹): 패킷이 없으면 0 리턴, 있으면 payload 길이 리턴
// buf_size보다 큰 payload면 buf_size만큼만 복사하고, 나머지는 버림(consume은 정상 수행)
int32_t  s_udp_recvfrom(s_udp_handle_t *h, s_udp_peer_t *from, uint8_t *buf, size_t buf_size);

// 송신
uint16_t s_udp_sendto(s_udp_handle_t *h, const s_udp_peer_t *to, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif
