#ifndef CROSSTALK_UI_KEY_H
#define CROSSTALK_UI_KEY_H

#include <stdint.h>

#include <dangling/term/vt.h>

uint32_t crosstalk_ui_key_us_map(ldg_term_key_event_t *ev);
uint32_t crosstalk_ui_key_mnemonic_assign(const char *const *label, const uint8_t *skip, uint32_t cunt, uint32_t *out_pos, uint8_t *out_ch);
uint32_t crosstalk_ui_key_mnemonic_ch_get(const ldg_term_key_event_t *irq, uint8_t *out);
uint32_t crosstalk_ui_key_mnemonic_find(const uint8_t *ch, uint32_t cunt, const ldg_term_key_event_t *irq, uint32_t *out_idx);

#endif
