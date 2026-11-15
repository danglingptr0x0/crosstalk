#ifndef CROSSTALK_RE_REPL_THREAD_H
#define CROSSTALK_RE_REPL_THREAD_H

#include <stdint.h>

#include "re/repl/repl.h"

#define CROSSTALK_RE_REPL_THREAD_ID_CAP 256
#define CROSSTALK_RE_REPL_THREAD_REG_CUNT 18
#define CROSSTALK_RE_REPL_THREAD_REG_SP 4
#define CROSSTALK_RE_REPL_THREAD_REG_LOC 16
#define CROSSTALK_RE_REPL_THREAD_REG_FLAGS 17
#define CROSSTALK_RE_REPL_THREAD_STACK_WORDS 64
#define CROSSTALK_RE_REPL_THREAD_STACK_WORD_BYTES 8
#define CROSSTALK_RE_REPL_THREAD_CODE_BYTES 256
#define CROSSTALK_RE_REPL_THREAD_MOD_NAME_CAP 32
#define CROSSTALK_RE_REPL_THREAD_PUDDING_SIZE 9

typedef enum crosstalk_re_repl_thread_tag
{
    CROSSTALK_RE_REPL_THREAD_TAG_NONE = 0,
    CROSSTALK_RE_REPL_THREAD_TAG_STACK,
    CROSSTALK_RE_REPL_THREAD_TAG_PTR,
    CROSSTALK_RE_REPL_THREAD_TAG_CODE
} crosstalk_re_repl_thread_tag_t;

typedef struct crosstalk_re_repl_thread
{
    uint64_t id[CROSSTALK_RE_REPL_THREAD_ID_CAP];
    uint64_t reg[CROSSTALK_RE_REPL_THREAD_REG_CUNT];
    uint64_t stack[CROSSTALK_RE_REPL_THREAD_STACK_WORDS];
    uint64_t stack_mod_off[CROSSTALK_RE_REPL_THREAD_STACK_WORDS];
    char stack_mod[CROSSTALK_RE_REPL_THREAD_STACK_WORDS][CROSSTALK_RE_REPL_THREAD_MOD_NAME_CAP];
    uint8_t code[CROSSTALK_RE_REPL_THREAD_CODE_BYTES];
    uint8_t reg_tag[CROSSTALK_RE_REPL_THREAD_REG_CUNT];
    uint8_t stack_tag[CROSSTALK_RE_REPL_THREAD_STACK_WORDS];
    uint8_t tag_pudding[6];
    uint64_t sel_id;
    uint64_t stack_addr;
    uint64_t code_addr;
    uint64_t next_ns;
    uint32_t id_cunt;
    uint32_t sel;
    uint32_t stack_cunt;
    uint32_t code_cunt;
    uint32_t last_err;
    uint8_t regs_ok;
    uint8_t watch;
    uint8_t live;
    uint8_t pudding[CROSSTALK_RE_REPL_THREAD_PUDDING_SIZE];
} crosstalk_re_repl_thread_t;

uint32_t crosstalk_re_repl_thread_init(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_thread_shutdown(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_thread_rst(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_thread_watch_set(crosstalk_re_repl_ctx_t *ctx, uint8_t on);
uint32_t crosstalk_re_repl_thread_tick(crosstalk_re_repl_ctx_t *ctx);
uint32_t crosstalk_re_repl_thread_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args);
uint32_t crosstalk_re_repl_thread_ptr_get(const crosstalk_re_repl_ctx_t *ctx, const crosstalk_re_repl_thread_t **out);
uint32_t crosstalk_re_repl_thread_reg_name_get(uint32_t idx, const char **out);

#endif
