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
#include "s_udp.h"

static void udp_echo_task(void *arg)
{
    s_udp_handle_t udp;
    s_udp_open(&udp, 0, 5000);

    ESP_LOGI("APP/UDP", "UDP echo started via service");

    while (1) {
        s_udp_peer_t peer;
        uint8_t buf[512];

        int32_t n = s_udp_recvfrom(&udp, &peer, buf, sizeof(buf));
        if (n > 0) {
            ESP_LOGI("APP/UDP", "RX from %u.%u.%u.%u:%u len=%d",
                     peer.ip[0], peer.ip[1], peer.ip[2], peer.ip[3],
                     (unsigned)peer.port, (int)n);

            s_udp_sendto(&udp, &peer, buf, (size_t)n);
            ESP_LOGI("APP/UDP", "TX echo len=%d", (int)n);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
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
    xTaskCreate(udp_echo_task, "udp_echo", 4096, NULL, 3, NULL);


    while (1) {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}