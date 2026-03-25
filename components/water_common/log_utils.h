#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void log_hex(const char *tag, const char *prefix, const uint8_t *data, size_t len);
void log_can(const char *tag, const char *prefix, uint32_t id, const uint8_t *data, uint8_t dlc);

#ifdef __cplusplus
}
#endif