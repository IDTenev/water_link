#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t w5500_init(void);
uint8_t  w5500_get_pmode_strap(void);
uint8_t  w5500_get_version(void);
bool     w5500_is_ready(void);

#ifdef __cplusplus
}
#endif
