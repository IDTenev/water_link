#include "common_data.h"
#include "p_rs485.h"

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define TAG "PORT/RS485"

// ---- UART 설정 ----

// 동시에 send/read 호출 방지
static SemaphoreHandle_t s_rs485_mutex = NULL;
static bool s_inited = false;

static inline TickType_t to_ticks(uint32_t ms)
{
    return (ms == 0) ? 0 : pdMS_TO_TICKS(ms);
}

uint16_t rs485_init(void)
{
    if (s_inited) {
        ESP_LOGW(TAG, "rs485 already init");
        return 0;
    }

    // mutex
    s_rs485_mutex = xSemaphoreCreateMutex();
    if (!s_rs485_mutex) {
        ESP_LOGE(TAG, "mutex create fail");
        return 1;
    }

    // UART config
    uart_config_t cfg = {
        .baud_rate = RS485_BAUD_DEFAULT,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;

    err = uart_driver_install(RS485_UART_NUM,
                             RS485_RX_BUF_SIZE,
                             RS485_TX_BUF_SIZE,
                             RS485_EVENT_QUEUE_SIZE,
                             NULL,
                             0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install fail (%s)", esp_err_to_name(err));
        return 2;
    }

    err = uart_param_config(RS485_UART_NUM, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config fail (%s)", esp_err_to_name(err));
        return 3;
    }

    err = uart_set_pin(RS485_UART_NUM,
                       PIN_RS485_TX,
                       PIN_RS485_RX,
                       PIN_RS485_DE,          // RTS -> DE
                       UART_PIN_NO_CHANGE);   // CTS
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin fail (%s)", esp_err_to_name(err));
        return 4;
    }

    // RS485 Half Duplex 모드
    err = uart_set_mode(RS485_UART_NUM, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_mode(RS485) fail (%s)", esp_err_to_name(err));
        return 5;
    }

    // 옵션: 충돌 감지 관련
    // err = uart_set_rx_timeout(RS485_UART_NUM, 2); // 심볼 타임아웃(바이트 기반), 필요할 때만

    // 혹시 모르니 버퍼 정리
    uart_flush_input(RS485_UART_NUM);

    s_inited = true;
    ESP_LOGI(TAG, "rs485 init ok (UART%d, TX=%d RX=%d DE/RTS=%d, baud=%d)",
             RS485_UART_NUM, PIN_RS485_TX, PIN_RS485_RX, PIN_RS485_DE, RS485_BAUD_DEFAULT);

    return 0;
}

size_t rs485_send(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (!s_inited || !data || len == 0) return 0;

    if (xSemaphoreTake(s_rs485_mutex, to_ticks(timeout_ms)) != pdTRUE) {
        ESP_LOGW(TAG, "send mutex timeout");
        return 0;
    }

    int written = uart_write_bytes(RS485_UART_NUM, (const char *)data, (int)len);
    if (written < 0) written = 0;

    // RS485 half duplex는 송신 완료까지 기다려주는 게 안전
    uart_wait_tx_done(RS485_UART_NUM, to_ticks(timeout_ms));

    xSemaphoreGive(s_rs485_mutex);
    return (size_t)written;
}

size_t rs485_read(uint8_t *out, size_t len, uint32_t timeout_ms)
{
    if (!s_inited || !out || len == 0) return 0;

    if (xSemaphoreTake(s_rs485_mutex, to_ticks(timeout_ms)) != pdTRUE) {
        ESP_LOGW(TAG, "read mutex timeout");
        return 0;
    }

    int r = uart_read_bytes(RS485_UART_NUM, out, (uint32_t)len, to_ticks(timeout_ms));
    if (r < 0) r = 0;

    xSemaphoreGive(s_rs485_mutex);
    return (size_t)r;
}

void rs485_flush_rx(void)
{
    if (!s_inited) return;
    uart_flush_input(RS485_UART_NUM);
}

void rs485_wait_tx_done(uint32_t timeout_ms)
{
    if (!s_inited) return;
    uart_wait_tx_done(RS485_UART_NUM, to_ticks(timeout_ms));
}

uint16_t rs485_set_baud(int baudrate)
{
    if (!s_inited) return 1;
    if (baudrate <= 0) return 2;

    esp_err_t err = uart_set_baudrate(RS485_UART_NUM, baudrate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_baudrate fail (%s)", esp_err_to_name(err));
        return 3;
    }

    ESP_LOGI(TAG, "rs485 baud -> %d", baudrate);
    return 0;
}
