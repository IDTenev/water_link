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
    
    // =========================
    // SPI TEST
    // =========================
    printf("SPI Test Start\n");

    p_spi_dev_t spi_dev;

    // W5500은 SPI mode 0, 8~30MHz 가능.
    // 일단 안정적으로 8MHz로 테스트.
    if (spi_dev_add(&spi_dev, 8 * 1000 * 1000, 0) != 0) {
        printf("SPI device add failed\n");
        return;
    }

    uint8_t tx_buf[4] = {0xAA, 0x55, 0x00, 0xFF};
    uint8_t rx_buf[4] = {0};

    if (spi_txrx(&spi_dev, tx_buf, rx_buf, sizeof(tx_buf)) != 0) {
        printf("SPI txrx failed\n");
    } else {
        printf("SPI TXRX OK\n");
        printf("RX: ");
        for (int i = 0; i < 4; i++) {
            printf("%02X ", rx_buf[i]);
        }
        printf("\n");
    }

    printf("SPI Test End\n");
    while (1) {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}