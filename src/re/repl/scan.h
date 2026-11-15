#ifndef CROSSTALK_RE_REPL_SCAN_H
#define CROSSTALK_RE_REPL_SCAN_H

#include <stdint.h>

#include "re/repl/repl.h"

uint32_t crosstalk_re_repl_scan_init(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_scan_shutdown(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_scan_rst(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_scan_tick(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_scan_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args);

#endif
