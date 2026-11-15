#ifndef CROSSTALK_RE_REPL_NATIVE_H
#define CROSSTALK_RE_REPL_NATIVE_H

#include <stdint.h>

#include "re/repl/repl.h"

uint32_t crosstalk_re_repl_native_init(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_native_shutdown(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_native_rst(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_native_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args);
uint32_t crosstalk_re_repl_native_search_tick(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_native_rd(crosstalk_re_repl_ctx_t *ctx, uint64_t addr, void *out, uint64_t len);
uint32_t crosstalk_re_repl_native_addr_get(crosstalk_re_repl_ctx_t *ctx, const char *text, uint64_t *out);
uint32_t crosstalk_re_repl_native_type_get(const char *text, uint32_t *out_bytes, uint8_t *out_fp, uint8_t *out_signed);

#endif
