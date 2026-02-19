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

    while(1) {
        printf("WATER_LINK IDEL\n");
        ws2812_set_pixel(255,0,0,50);
        ws2812_refresh();
        vTaskDelay(pdMS_TO_TICKS(500));

        ws2812_set_pixel(0,255,0,50);
        ws2812_refresh();
        vTaskDelay(pdMS_TO_TICKS(500));

        ws2812_set_pixel(0,0,255,50);
        ws2812_refresh();
        vTaskDelay(pdMS_TO_TICKS(500));

        ws2812_set_pixel(255,255,255,50);
        ws2812_refresh();
        vTaskDelay(pdMS_TO_TICKS(500));

        for(int i=0;i<255;i++) {
            ws2812_set_pixel(255,0,0,i);
            ws2812_refresh();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    printf("Start Water-Link v1.0\n");
}
