#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t ht16k33_init(uint8_t addr);
uint16_t ht16k33_set_brightness(uint8_t addr, uint8_t level_0_15);
uint16_t ht16k33_display_on(uint8_t addr, uint8_t on);
uint16_t ht16k33_print_u16(uint8_t addr, uint16_t value, uint8_t leading_zero);

#ifdef __cplusplus
}
#endif
