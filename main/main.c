#include <stdio.h>

#include "common_data.h"
#include "error_code.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "p_gpio.h"
#include "p_rs232.h"
#include "p_can.h"

#include "d_switch.h"
#define TAG "APP/MAIN"

void app_main(void)
{
    printf("Boot Water v1.1\n");

    printf("Initialing System\n");
    //port init
    gpio_init();
    rs232_init();
    uint16_t rc = can_init();
    if (rc) {
        ESP_LOGE(TAG, "can_init failed: 0x%04X", rc);
        return;
    }

    printf("Start Water-Link v1.0\n");

    while (1) {
        if(switch_get_06_status() == SW_SHORT) {
            while(switch_get_06_status() == SW_SHORT);

            const uint8_t tx[8] = {0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06};
            printf("RS232 SEND [%d byte]: ", sizeof(tx));
            for (int i = 0; i < sizeof(tx); i++) {
                printf("%02X ", tx[i]);
            }
            printf("\n");
            rs232_send(tx, sizeof(tx));

            vTaskDelay(pdMS_TO_TICKS(50));
        }
        if(switch_get_14_status() == SW_SHORT) {
            while(switch_get_14_status() == SW_SHORT);

            const uint8_t tx[8] = {0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14};
            printf("RS232 SEND [%d byte]: ", sizeof(tx));
            for (int i = 0; i < sizeof(tx); i++) {
                printf("%02X ", tx[i]);
            }
            printf("\n");
            rs232_send(tx, sizeof(tx));

            vTaskDelay(pdMS_TO_TICKS(50));
        }
        if(switch_get_37_status() == SW_SHORT) {
            while(switch_get_37_status() == SW_SHORT);

            const uint8_t tx[8] = {0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37};
            printf("RS232 SEND [%d byte]: ", sizeof(tx));
            for (int i = 0; i < sizeof(tx); i++) {
                printf("%02X ", tx[i]);
            }
            printf("\n");
            rs232_send(tx, sizeof(tx));

            vTaskDelay(pdMS_TO_TICKS(50));
        }
        uint8_t can_tx_data[8] = {0,};
        uint8_t rx[256];
        size_t n = rs232_read(rx, sizeof(rx));

        if (n) {
            printf("RS232 READ [%d byte]: ", n);
            for (int i = 0; i < n; i++) {
                printf("%02X ", rx[i]);
            }
            printf("\n");

            //make can fream
            if(n < 8) {
                for (int i=0;i<n;i++) {
                    can_tx_data[i] = rx[i];
                }
                for (int i=n;i<8;i++){
                    can_tx_data[i] = 0;
                }
            }
            else{
                for (int i=0;i<8;i++){
                    can_tx_data[i] = rx[i];
                }
            }

            //send can fream
            uint32_t identifier = 0x12345678;
            bool ext = true;

            printf("CAN TX : [%lX] : ", identifier);
            for (int i = 0; i < n; i++) {
                printf("%02X ", can_tx_data[i]);
            }
            printf("\n");
            rc = can_send(identifier, can_tx_data, 8, ext, false, 10);
            if (rc) {
                ESP_LOGW(TAG, "TX failed: 0x%04X", rc);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}