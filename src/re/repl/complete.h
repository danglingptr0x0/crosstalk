#ifndef CROSSTALK_RE_REPL_COMPLETE_H
#define CROSSTALK_RE_REPL_COMPLETE_H

#include <stdint.h>

#include "re/repl/repl.h"
#include "re/repl/subst.h"

#define CROSSTALK_RE_REPL_COMPLETE_CAP 64
#define CROSSTALK_RE_REPL_COMPLETE_ADDR_CAP 64
#define CROSSTALK_RE_REPL_COMPLETE_ADDR_TEXT_CAP 24
#define CROSSTALK_RE_REPL_COMPLETE_ROWS 10
#define CROSSTALK_RE_REPL_COMPLETE_SEL_NONE UINT32_MAX
#define CROSSTALK_RE_REPL_COMPLETE_PUDDING_SIZE 1

typedef struct crosstalk_re_repl_complete_cand
{
    char val[CROSSTALK_RE_REPL_CMD_CAP];
    char label[CROSSTALK_RE_REPL_NAME_CAP];
} crosstalk_re_repl_complete_cand_t;

typedef struct crosstalk_re_repl_complete
{
    crosstalk_re_repl_complete_cand_t cand[CROSSTALK_RE_REPL_COMPLETE_CAP];
    crosstalk_re_repl_complete_cand_t held;
    char addr[CROSSTALK_RE_REPL_COMPLETE_ADDR_CAP][CROSSTALK_RE_REPL_COMPLETE_ADDR_TEXT_CAP];
    char last[CROSSTALK_RE_REPL_CMD_CAP];
    char buff[CROSSTALK_RE_REPL_CMD_CAP];
    uint32_t cunt;
    uint32_t start;
    uint32_t end;
    uint32_t sel;
    uint32_t addr_next;
    uint8_t more;
    uint8_t space;
    uint8_t on;
    uint8_t pudding[CROSSTALK_RE_REPL_COMPLETE_PUDDING_SIZE];
} crosstalk_re_repl_complete_t;

uint32_t crosstalk_re_repl_complete_init(crosstalk_re_repl_complete_t *complete);
uint32_t crosstalk_re_repl_complete_rst(crosstalk_re_repl_complete_t *complete);
uint32_t crosstalk_re_repl_complete_addr_capture(crosstalk_re_repl_complete_t *complete, const char *text, uint32_t len);
uint32_t crosstalk_re_repl_complete_tab(crosstalk_re_repl_complete_t *complete, const crosstalk_re_repl_subst_t *subst, char *line, uint32_t cap, uint32_t *len, uint8_t *out_list);

#endif
