#include <stdio.h>

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

    while (1) {

        const uint8_t tx[] = "hello\r\n";
        rs232_send(tx, sizeof(tx) - 1);

        vTaskDelay(pdMS_TO_TICKS(50));

        uint8_t rx[256];
        size_t n = rs232_read(rx, sizeof(rx));

        if (n) {
            printf("RX %d bytes: ", (int)n);

            printf("%.*s", (int)n, rx);

            printf("HEX: ");
            for (int i = 0; i < n; i++) {
                printf("%02X ", rx[i]);
            }
            printf("\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}