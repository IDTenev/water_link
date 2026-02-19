#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t gpio_init(void);
uint16_t gpio_sw_water_get_level(void);
uint16_t gpio_sw_06_get_level(void);
uint16_t gpio_sw_14_get_level(void);
uint16_t gpio_sw_37_get_level(void);

#ifdef __cplusplus
}
#endif