#ifndef CROSSTALK_HDL_PACK_H
#define CROSSTALK_HDL_PACK_H

#include <stdint.h>

#include "hdl/aig.h"
#include "hdl/ir.h"

#define CROSSTALK_HDL_ERR_PACK 740
#define CROSSTALK_HDL_ERR_PACK_WIDE 741

typedef struct crosstalk_hdl_pack_ctx
{
    const crosstalk_hdl_aig_ctx_t *aig_ctx;
    const crosstalk_hdl_ir_t *ir_ctx;
    const char *src;
    void *pool;
    char *out;
    uint32_t *lvl;
    uint32_t *pos;
    uint32_t *ord;
    uint32_t *word_kind;
    uint32_t *word_op;
    uint32_t *lvl_cunt;
    uint32_t *lvl_off;
    uint32_t *sort_buff;
    uint32_t *gt_blob;
    uint32_t out_len;
    uint32_t out_cap;
    uint32_t word_cunt;
    uint32_t level_max;
    uint32_t gt_cunt;
    uint32_t gt_cap;
    uint32_t bcast_cunt;
    uint32_t affine_cunt;
    uint32_t table_cunt;
    uint32_t wide_sig;
    uint32_t latch_cunt;
    uint32_t ff_word_cunt;
    uint32_t mem_op_cunt;
    uint8_t pudding[4];
} crosstalk_hdl_pack_ctx_t;

uint32_t crosstalk_hdl_pack_run(crosstalk_hdl_pack_ctx_t *ctx, const crosstalk_hdl_aig_ctx_t *aig_ctx, const char *src, void *pool, char **out_src, uint32_t *out_len);

#endif
