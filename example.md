# CAN Example
## 외부-> Water-link 로 데이터를 주면 데이터에 +1 하여 리턴
```
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

#define TAG "APP/MAIN"

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

    printf("Start Water-Link v1.0\n");

    while (1) {
        twai_message_t rx = {0};

        // 1000ms 동안 수신 대기
        rc = can_read(&rx, 1000);
        if (rc) {
            ESP_LOGW(TAG, "RX Time out");
            continue;
        }

        // RTR(원격요청)는 데이터 없으니 무시
        if (rx.rtr) {
            ESP_LOGI(TAG, "RX RTR id=0x%X", (unsigned)rx.identifier);
            continue;
        }

        // DLC 8바이트만 처리(요구사항)
        if (rx.data_length_code != 8) {
            ESP_LOGW(TAG, "RX dlc=%d (skip) id=0x%X",
                     rx.data_length_code, (unsigned)rx.identifier);
            continue;
        }

        uint8_t tx_data[8];
        for (int i = 0; i < 8; i++) {
            tx_data[i] = (uint8_t)(rx.data[i] + 1); // 0xFF면 0x00으로 래핑됨(정상)
        }

        // 표준/확장 플래그 유지해서 그대로 리턴
        bool ext = (rx.extd != 0);

        rc = can_send(rx.identifier, tx_data, 8, ext, false, 10);
        if (rc) {
            ESP_LOGW(TAG, "TX failed: 0x%04X", rc);
        } else {
            ESP_LOGI(TAG, "ECHO id=0x%X ext=%d : +1 returned", (unsigned)rx.identifier, (int)ext);
        }
    }
}
```