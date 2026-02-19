#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

uint16_t ws2812_init(void);
void ws2812_set_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);
uint16_t ws2812_refresh(void);

#ifdef __cplusplus
}
#endif
