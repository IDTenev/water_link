#include "d_ht16k33.h"
#include "p_i2c.h"
#include "esp_log.h"

#define TAG "DRIVER/HT16K33"

// 7-seg font (gfedcba)
static const uint8_t seg7_font[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

static uint16_t ht_cmd(uint8_t addr, uint8_t cmd)
{
    return i2c_write(addr, &cmd, 1);
}

uint16_t ht16k33_init(uint8_t addr)
{
    uint16_t r;

    // Oscillator ON
    r = ht_cmd(addr, 0x21);
    if (r) { ESP_LOGE(TAG, "osc on fail"); return r; }

    // Display ON, blink OFF
    r = ht_cmd(addr, 0x81);
    if (r) { ESP_LOGE(TAG, "display on fail"); return r; }

    // Brightness max
    r = ht_cmd(addr, 0xEF);
    if (r) { ESP_LOGE(TAG, "brightness fail"); return r; }

    ESP_LOGI(TAG, "init ok (addr=0x%02X)", addr);
    return 0;
}

uint16_t ht16k33_set_brightness(uint8_t addr, uint8_t level_0_15)
{
    if (level_0_15 > 15) level_0_15 = 15;

    uint8_t cmd = 0xE0 | level_0_15;
    return ht_cmd(addr, cmd);
}

uint16_t ht16k33_display_on(uint8_t addr, uint8_t on)
{
    uint8_t cmd = on ? 0x81 : 0x80; // blink off 고정
    return ht_cmd(addr, cmd);
}

uint16_t ht16k33_print_u16(uint8_t addr, uint16_t value, uint8_t leading_zero)
{
    if (value > 9999) value = 9999;

    uint8_t digit[4];
    digit[0] = (value / 1000) % 10;
    digit[1] = (value / 100)  % 10;
    digit[2] = (value / 10)   % 10;
    digit[3] = (value / 1)    % 10;

    // HT16K33 RAM write
    // 시작 주소 0x00
    // digit은 0x00,0x02,0x04,0x06 위치에 들어감
    uint8_t buf[1 + 8] = {0};
    buf[0] = 0x00; // RAM start address

    for (int i = 0; i < 4; i++) {

        uint8_t seg = seg7_font[digit[i]];

        // leading zero 제거
        if (!leading_zero) {
            if (i < 3 && digit[i] == 0) {
                int all_prev_zero = 1;
                for (int k = 0; k < i; k++)
                    if (digit[k] != 0) all_prev_zero = 0;

                if (all_prev_zero) seg = 0x00;
            }
        }

        buf[1 + i*2]     = seg;
        buf[1 + i*2 + 1] = 0x00;
    }

    return i2c_write(addr, buf, sizeof(buf));
}
