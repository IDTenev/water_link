#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t rs232_init(void);
size_t   rs232_send(const uint8_t *data, size_t len);
size_t   rs232_read(uint8_t *out, size_t max_len);

size_t   rs232_available(void);
void     rs232_flush(void);

#ifdef __cplusplus
}
#endif
