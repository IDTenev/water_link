#include "s_dhcp.h"
#include "s_udp.h"
#include "d_w5500.h"
#include "d_w5500_net.h"
#include "esp_log.h"
#include <string.h>

#define TAG "SERVICE/DHCP"

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

#define DHCP_DISCOVER 1
#define DHCP_REQUEST  3

#define DHCP_OPTION_MSGTYPE 53
#define DHCP_OPTION_REQIP   50
#define DHCP_OPTION_END     255

static uint32_t xid = 0x12345678;

static void dhcp_build_discover(uint8_t *buf, uint8_t *mac)
{
    memset(buf, 0, 548);

    buf[0] = 1;   // BOOTREQUEST
    buf[1] = 1;   // Ethernet
    buf[2] = 6;   // MAC length
    buf[3] = 0;   // hops

    buf[4] = (xid >> 24) & 0xFF;
    buf[5] = (xid >> 16) & 0xFF;
    buf[6] = (xid >> 8)  & 0xFF;
    buf[7] = xid & 0xFF;

    memcpy(&buf[28], mac, 6);

    // magic cookie
    buf[236] = 99;
    buf[237] = 130;
    buf[238] = 83;
    buf[239] = 99;

    // DHCP Message Type
    buf[240] = DHCP_OPTION_MSGTYPE;
    buf[241] = 1;
    buf[242] = DHCP_DISCOVER;

    buf[243] = DHCP_OPTION_END;
}

bool s_dhcp_start(s_dhcp_info_t *info)
{
    s_udp_handle_t udp;
    s_udp_open(&udp, 1, DHCP_CLIENT_PORT);

    uint8_t mac[6];
    w5500_get_mac(mac);

    uint8_t packet[548];
    dhcp_build_discover(packet, mac);

    s_udp_peer_t peer = {
        .ip   = {255,255,255,255},
        .port = DHCP_SERVER_PORT
    };

    ESP_LOGI(TAG, "Sending DHCP DISCOVER");

    s_udp_sendto(&udp, &peer, packet, 244);

    return true;
}
