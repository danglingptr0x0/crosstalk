#ifndef CROSSTALK_RE_REPL_SNITCH_H
#define CROSSTALK_RE_REPL_SNITCH_H

#include <stdint.h>

#include "re/repl/repl.h"

uint32_t crosstalk_re_repl_snitch_init(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_snitch_shutdown(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_snitch_rst(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_snitch_line_send(crosstalk_re_repl_ctx_t *ctx, const char *line);
uint32_t crosstalk_re_repl_snitch_cancel(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_snitch_tick(crosstalk_re_repl_ctx_t *ctx);

#endif
