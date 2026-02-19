#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t switch_get_water_status(void);
uint16_t switch_get_06_status(void);
uint16_t switch_get_14_status(void);
uint16_t switch_get_37_status(void);

#ifdef __cplusplus
}
#endif
