#ifndef S_DHCP_H
#define S_DHCP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t ip[4];
    uint8_t sn[4];
    uint8_t gw[4];
    uint8_t dns[4];
} s_dhcp_info_t;

bool s_dhcp_start(s_dhcp_info_t *info);

#endif
