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
    //ht16k33_init(HT16K33_ADDRESS);
    
    while (1) {

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}