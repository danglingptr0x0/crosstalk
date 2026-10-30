#ifndef CROSSTALK_RE_DIFF_H
#define CROSSTALK_RE_DIFF_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>

#define CROSSTALK_RE_DIFF_ERR_SIDE 810
#define CROSSTALK_RE_DIFF_ERR_MASK_FULL 811
#define CROSSTALK_RE_DIFF_ERR_SNAP 812

#define CROSSTALK_RE_DIFF_SIDE_REF 0
#define CROSSTALK_RE_DIFF_SIDE_CAND 1
#define CROSSTALK_RE_DIFF_SIDE_CUNT 2

#define CROSSTALK_RE_DIFF_VERDICT_NONE 0
#define CROSSTALK_RE_DIFF_VERDICT_MATCH 1
#define CROSSTALK_RE_DIFF_VERDICT_DIVERGED 2

#define CROSSTALK_RE_DIFF_MASK_MAX 32
#define CROSSTALK_RE_DIFF_SIG_CAP 64
#define CROSSTALK_RE_DIFF_PAGE_BYTES 4096
#define CROSSTALK_RE_DIFF_NODE_MAX 65536
#define CROSSTALK_RE_DIFF_PAGE_MAX 131072
#define CROSSTALK_RE_DIFF_CYCLES_DEFAULT 1000000
#define CROSSTALK_RE_DIFF_SIG_DEFAULT "mem"
#define CROSSTALK_RE_DIFF_RAM_WORD_BYTES 4
#define CROSSTALK_RE_DIFF_FNV64_INIT 0xCBF29CE484222325ULL
#define CROSSTALK_RE_DIFF_FNV64_PRIME 0x100000001B3ULL
#define CROSSTALK_RE_DIFF_PTR_TAG 0x5054520000000000ULL
#define CROSSTALK_RE_DIFF_ARG_MAX 4
#define CROSSTALK_RE_DIFF_NAME_CAP 64
#define CROSSTALK_RE_DIFF_SP_NONE 0

typedef struct crosstalk_re_diff_side
{
    char path[LDG_IO_PATH_MAX];
    char sig[CROSSTALK_RE_DIFF_SIG_CAP];
    uint64_t *node;
    uint64_t *page_hash;
    uint64_t word_off;
    uint64_t base;
    uint64_t span;
    uint64_t node_hash;
    uint64_t mem_hash;
    uint64_t ram_hash;
    uint64_t cycles_ran;
    uint32_t node_cunt;
    uint32_t mem_words;
    uint32_t page_cunt;
    uint8_t set;
    uint8_t taken;
    uint8_t pudding[2];
} crosstalk_re_diff_side_t;

typedef struct crosstalk_re_diff_ctx
{
    ldg_mem_pool_t *pool;
    crosstalk_re_diff_side_t side[CROSSTALK_RE_DIFF_SIDE_CUNT];
    char mask_name[CROSSTALK_RE_DIFF_MASK_MAX][CROSSTALK_RE_DIFF_SIG_CAP];
    uint32_t mask_idx[CROSSTALK_RE_DIFF_MASK_MAX];
    uint32_t mask_cunt;
    uint32_t verdict;
    uint32_t node_diff_cunt;
    uint32_t node_first;
    uint32_t page_diff_cunt;
    uint32_t page_first;
    uint64_t cycles;
    char func[CROSSTALK_RE_DIFF_NAME_CAP];
    uint64_t entry[CROSSTALK_RE_DIFF_SIDE_CUNT];
    uint64_t arg[CROSSTALK_RE_DIFF_ARG_MAX];
    uint64_t sp;
    uint32_t arg_cunt;
    uint8_t func_mode;
    uint8_t mem_diff;
    uint8_t ok;
    uint8_t pudding[1];
} crosstalk_re_diff_ctx_t;

uint32_t crosstalk_re_diff_init(crosstalk_re_diff_ctx_t *ctx);
uint32_t crosstalk_re_diff_shutdown(crosstalk_re_diff_ctx_t *ctx);
uint32_t crosstalk_re_diff_clear(crosstalk_re_diff_ctx_t *ctx);
uint32_t crosstalk_re_diff_side_set(crosstalk_re_diff_ctx_t *ctx, uint32_t side, const char *path, const char *sig, uint64_t word_off);
uint32_t crosstalk_re_diff_side_get(const crosstalk_re_diff_ctx_t *ctx, uint32_t side, const crosstalk_re_diff_side_t **out);
uint32_t crosstalk_re_diff_cycles_set(crosstalk_re_diff_ctx_t *ctx, uint64_t cycles);
uint32_t crosstalk_re_diff_mask_add(crosstalk_re_diff_ctx_t *ctx, const char *sig);
uint32_t crosstalk_re_diff_mask_idx_set(crosstalk_re_diff_ctx_t *ctx, uint32_t at, uint32_t idx);
uint32_t crosstalk_re_diff_mask_clear(crosstalk_re_diff_ctx_t *ctx);
uint32_t crosstalk_re_diff_snap_take(crosstalk_re_diff_ctx_t *ctx, uint32_t side, const uint64_t *node, uint32_t node_cunt, const uint64_t *mem, uint32_t mem_words, const uint8_t *ram, uint64_t ram_span, uint64_t base, uint64_t span, uint64_t cycles_ran);
uint32_t crosstalk_re_diff_verdict_compute(crosstalk_re_diff_ctx_t *ctx);
uint32_t crosstalk_re_diff_verdict_name_get(uint32_t verdict, const char **out);
uint32_t crosstalk_re_diff_func_set(crosstalk_re_diff_ctx_t *ctx, const char *name, uint64_t entry_ref, uint64_t entry_cand);
uint32_t crosstalk_re_diff_func_clear(crosstalk_re_diff_ctx_t *ctx);
uint32_t crosstalk_re_diff_arg_set(crosstalk_re_diff_ctx_t *ctx, uint32_t idx, uint64_t val);
uint32_t crosstalk_re_diff_sp_set(crosstalk_re_diff_ctx_t *ctx, uint64_t sp);
uint8_t crosstalk_re_diff_ready_is(const crosstalk_re_diff_ctx_t *ctx);

#endif
