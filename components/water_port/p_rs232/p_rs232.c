#include "common_data.h"
#include "p_rs232.h"

#include "esp_log.h"
#include "driver/uart.h"

#define TAG "PORT/RS232"

#define RS232_RX_BUF_SIZE    (2048)
#define RS232_TX_BUF_SIZE    (2048)

static bool s_rs232_inited = false;

uint16_t rs232_init(void) {
    if (s_rs232_inited) {
        ESP_LOGW(TAG, "rs232 already inited");
        return 0;
    }

    ESP_LOGI(TAG, "rs232 init (HW UART) port=%d baud=%d TX=%d RX=%d",
             (int)RS232_UART_PORT, RS232_UART_BAUD, PIN_RS232_TX, PIN_RS232_RX);

    const uart_config_t cfg = {
        .baud_rate = RS232_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;

    err = uart_driver_install(RS232_UART_PORT,
                              RS232_RX_BUF_SIZE,
                              RS232_TX_BUF_SIZE,
                              0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return 1;
    }

    err = uart_param_config(RS232_UART_PORT, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return 2;
    }

    err = uart_set_pin(RS232_UART_PORT,
                       PIN_RS232_TX,
                       PIN_RS232_RX,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return 3;
    }

    uart_flush_input(RS232_UART_PORT);

    s_rs232_inited = true;
    return 0;
}

size_t rs232_send(const uint8_t *data, size_t len) {
    if (!s_rs232_inited || data == NULL || len == 0) return 0;

    int written = uart_write_bytes(RS232_UART_PORT, (const char*)data, (int)len);
    if (written < 0) return 0;

    return (size_t)written;
}

size_t rs232_read(uint8_t *out, size_t max_len) {
    if (!s_rs232_inited || out == NULL || max_len == 0) return 0;

    int rd = uart_read_bytes(RS232_UART_PORT, out, (uint32_t)max_len, 0);
    if (rd < 0) return 0;
    return (size_t)rd;
}

size_t rs232_available(void) {
    if (!s_rs232_inited) return 0;

    size_t buffered = 0;
    uart_get_buffered_data_len(RS232_UART_PORT, &buffered);
    return buffered;
}

void rs232_flush(void) {
    if (!s_rs232_inited) return;
    uart_flush_input(RS232_UART_PORT);
}
