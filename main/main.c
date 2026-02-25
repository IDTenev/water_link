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
#include "d_ht16k33.h"

#include "s_udp.h"
#include "s_dhcp.h"
#include "s_ping.h"

#define TAG "APP/MAIN"

static void ping_task(void *arg)
{
    const uint8_t target[4] = {192,168,10,1};   // 라우터/NAT 환경에서만 의미 있음
    // PC 직결이면 192.168.10.1 같은 걸로

    while (1) {
        s_ping_result_t r = { .id = 0x1234, .seq = 1 };

        bool ok = s_ping_once(2, target, 1000, &r);
        if (ok) {
            ESP_LOGI("APP/PING", "PING OK rtt=%ums", (unsigned)r.rtt_ms);
        } else {
            ESP_LOGW("APP/PING", "PING TIMEOUT");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
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
    uint16_t rc = can_init();
    if (rc) {
        ESP_LOGE(TAG, "can_init failed: 0x%04X", rc);
        return;
    }

    //driver init
    w5500_init();
    ws2812_init();
    //ht16k33_init(HT16K33_ADDRESS);
    
    xTaskCreate(ping_task, "ping", 4096, NULL, 3, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}