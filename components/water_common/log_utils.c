#include "log_utils.h"
#include "esp_log.h"
#include <stdio.h>
void log_hex(const char *tag, const char *prefix, const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        ESP_LOGI(tag, "%s (empty)", prefix);
        return;
    }

    // 최대 256바이트 기준 (넉넉하게)
    static char buf[3 * 256 + 1];

    size_t pos = 0;

    for (size_t i = 0; i < len; i++) {
        pos += snprintf(&buf[pos], sizeof(buf) - pos, "%02X ", data[i]);
        if (pos >= sizeof(buf)) break;
    }

    ESP_LOGI(tag, "%s %s", prefix, buf);
}

void log_can(const char *tag, const char *prefix, uint32_t id, const uint8_t *data, uint8_t dlc)
{
    static char buf[3 * 8 + 1]; // CAN 최대 8바이트

    size_t pos = 0;

    for (int i = 0; i < dlc; i++) {
        pos += snprintf(&buf[pos], sizeof(buf) - pos, "%02X ", data[i]);
        if (pos >= sizeof(buf)) break;
    }

    ESP_LOGI(tag, "%s [%03lX] %s", prefix, id, buf);
}