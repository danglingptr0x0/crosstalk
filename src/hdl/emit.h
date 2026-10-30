#ifndef CROSSTALK_HDL_EMIT_H
#define CROSSTALK_HDL_EMIT_H

#include <stdint.h>

#include "hdl/ir.h"

#define CROSSTALK_HDL_ERR_EMIT 735
#define CROSSTALK_HDL_ERR_EMIT_WIDE 736

#define CROSSTALK_HDL_EMIT_BLK_MANY 0xFFFFFFFEU

typedef struct crosstalk_hdl_emit_ctx
{
    const crosstalk_hdl_ir_t *ir_ctx;
    const char *src;
    void *pool;
    char *out;
    uint32_t *order;
    uint32_t *residue;
    uint32_t *deps;
    uint32_t *rn_slot;
    uint32_t *mw_slot;
    uint8_t *rn_direct;
    uint32_t *mem_off;
    uint32_t *eval_ord;
    uint32_t *wr_last;
    uint32_t *sh_slot;
    uint8_t *fused;
    uint32_t *fuse_map;
    uint8_t *fused_seq;
    uint32_t *fuse_map_seq;
    uint8_t *cst_has;
    uint64_t *cst_val;
    uint32_t *vf_prev;
    uint8_t *loc_ok;
    uint8_t *loc_seq_ok;
    uint8_t *guard_need;
    uint32_t *loc_act;
    uint32_t *rgn_of;
    uint64_t *rgn_rd;
    uint64_t *rgn_un;
    uint8_t *rgn_wspan;
    uint32_t sh_cunt;
    uint32_t fuse_cunt;
    uint32_t rgn_cunt;
    uint32_t rw_cunt;
    uint32_t rgn_core_lo;
    uint32_t rgn_core_hi;
    uint8_t rgn_chk;
    uint8_t rgn_or_ra;
    uint8_t pf_is;
    uint8_t rgn_pudding[5];
    uint8_t *placed;
    uint32_t out_len;
    uint32_t out_cap;
    uint32_t order_cunt;
    uint32_t residue_cunt;
    uint32_t rn_cunt;
    uint32_t mw_cunt;
    uint32_t mem_words;
    uint32_t fast_ok;
    uint32_t fast_why;
    uint32_t fast_blocker;
    uint32_t vf_op;
    uint32_t vf_sig;
    uint8_t wide_has;
    uint8_t pudding[7];
} crosstalk_hdl_emit_ctx_t;

uint32_t crosstalk_hdl_emit_run(crosstalk_hdl_emit_ctx_t *ctx, const crosstalk_hdl_ir_t *ir_ctx, const char *src, void *pool, uint8_t strip_is, uint8_t pf_is, char **out_src, uint32_t *out_len);

#endif
