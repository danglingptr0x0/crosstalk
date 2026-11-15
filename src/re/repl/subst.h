#ifndef CROSSTALK_RE_REPL_SUBST_H
#define CROSSTALK_RE_REPL_SUBST_H

#include <stdint.h>

#include "re/repl/repl.h"

#define CROSSTALK_RE_REPL_SUBST_CAP 32
#define CROSSTALK_RE_REPL_SUBST_NAME_CAP 32

typedef struct crosstalk_re_repl_subst_entry
{
    char name[CROSSTALK_RE_REPL_SUBST_NAME_CAP];
    char text[CROSSTALK_RE_REPL_CMD_CAP];
} crosstalk_re_repl_subst_entry_t;

typedef struct crosstalk_re_repl_subst
{
    crosstalk_re_repl_subst_entry_t alias[CROSSTALK_RE_REPL_SUBST_CAP];
    crosstalk_re_repl_subst_entry_t var[CROSSTALK_RE_REPL_SUBST_CAP];
    char buff[CROSSTALK_RE_REPL_CMD_CAP];
} crosstalk_re_repl_subst_t;

uint32_t crosstalk_re_repl_subst_init(crosstalk_re_repl_subst_t *subst);
uint32_t crosstalk_re_repl_subst_apply(crosstalk_re_repl_subst_t *subst, crosstalk_re_repl_ctx_t *repl, char *line, uint32_t cap, uint8_t *out_handled);

#endif
