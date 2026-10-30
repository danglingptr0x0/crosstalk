#ifndef CROSSTALK_EDA_IO_SCANCODE_H
#define CROSSTALK_EDA_IO_SCANCODE_H

#include <stdint.h>

#define CROSSTALK_EDA_IO_SCANCODE_NONE 0x00
#define CROSSTALK_EDA_IO_SCANCODE_LSHIFT 0x2A
#define CROSSTALK_EDA_IO_SCANCODE_BREAK 0x80

uint32_t crosstalk_eda_io_scancode_map(uint8_t ch, uint8_t *out_code, uint8_t *out_shift);
uint32_t crosstalk_eda_io_scancode_get(uint8_t ch, uint8_t *out);

#endif
