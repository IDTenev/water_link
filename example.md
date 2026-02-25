# Water-Link Example

## On-board SW Example
### 500ms에 한번씩 스위치 값 출력
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
        switch_get_water_status();
        switch_get_06_status();
        switch_get_14_status();
        switch_get_37_status();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

## On-board WS2812 Example
### 500ms주기로 색 변경 (R -> G -> B -> W -> R Fade In -> Off)
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

        ws2812_set_pixel(0,0,0,0);
        ws2812_refresh();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

## RS232 Example
### 1s 주기로 hello 전송 후 수신 확인 (TX,RX 연결하여 Loopback)
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
```

## RS485 Example
### 외부에서 들어온 데이터를 Echo + 첫글자 하는 예제 (RX : Hello -> TX : HelloH)
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

    uint8_t buf[256];
    while (1) {
        size_t n = rs485_read(buf, sizeof(buf), 100); // 100ms 대기
        buf[n] = buf[0];
        if (n > 0) {
            rs485_send(buf, n+1, 100);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        //vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

## CAN Example
### 외부-> Water-link 로 데이터를 주면 데이터에 +1 하여 리턴
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

## i2c Example
### 모듈 Address 찾기
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

    for (int addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_probe((uint8_t)addr) == 0) {
            printf("I2C device found: 0x%02X\n", addr);
        }
    }
    while (1) {

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

### ht16k33 (FND 출력)
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
    ht16k33_init(HT16K33_ADDRESS);
    
    uint16_t cnt = 0;
    while (1) {
        ht16k33_print_u16(HT16K33_ADDRESS, cnt, 0);
        cnt = (cnt + 1) % 10000;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

## W5500 Example
### UDP Echo
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
#include "d_ht16k33.h"

#include "s_udp.h"

#define TAG "APP/MAIN"

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
    uint16_t rc = can_init();
    if (rc) {
        ESP_LOGE(TAG, "can_init failed: 0x%04X", rc);
        return;
    }

    //driver init
    w5500_init();
    ws2812_init();
    //ht16k33_init(HT16K33_ADDRESS);
    xTaskCreate(udp_echo_task, "udp_echo", 4096, NULL, 3, NULL);
    
    while (1) {

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

### Ping Test
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
```