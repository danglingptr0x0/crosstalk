#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/proto/emiemi.h>
#include <dangling/str/str.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "eda/io/cast.h"

#define CAST_CHUNK 256
#define CAST_LUMA_R_W 2
#define CAST_LUMA_G_W 5
#define CAST_LUMA_W_TOTAL 8
#define CAST_LUMA_SPAN 256
#define CAST_RAMP_CUNT 5
#define CAST_PX_BYTES 4
#define CAST_CHUNK_PX 128
#define CAST_PX_B_OFF 0
#define CAST_PX_G_OFF 1
#define CAST_PX_R_OFF 2

static LDG_NOINLINE uint32_t cast_payload_build(const crosstalk_eda_io_cast_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, uint8_t *payload)
{
    static const uint8_t ramp[CAST_RAMP_CUNT] = { 0x20, 0xB0, 0xB1, 0xB2, 0xDB };
    uint64_t chunk[CAST_CHUNK_PX * CAST_PX_BYTES] = LDG_ARR_ZERO_INIT;
    uint32_t sum[CROSSTALK_EDA_IO_CAST_COLS] = LDG_ARR_ZERO_INIT;
    uint32_t cell_w = 0;
    uint32_t cell_h = 0;
    uint32_t cy = 0;
    uint32_t ry = 0;
    uint32_t cx = 0;
    uint32_t px = 0;
    uint32_t px_done = 0;
    uint32_t px_cunt = 0;
    uint32_t rd = 0;
    uint32_t red = 0;
    uint32_t grn = 0;
    uint32_t blu = 0;
    uint32_t luma = 0;
    uint32_t avg = 0;
    uint32_t idx = 0;
    uint64_t row_base = 0;
    uint32_t ret = 0;

    cell_w = ctx->src_w / (uint32_t)CROSSTALK_EDA_IO_CAST_COLS;
    cell_h = ctx->src_h / (uint32_t)CROSSTALK_EDA_IO_CAST_ROWS;

    for (cy = 0; cy < (uint32_t)CROSSTALK_EDA_IO_CAST_ROWS; cy++)
    {
        memset(sum, 0x0, sizeof(sum));

        for (ry = 0; ry < cell_h; ry++)
        {
            row_base = ctx->base + ((uint64_t)cy * (uint64_t)cell_h + (uint64_t)ry) * (uint64_t)ctx->src_w * (uint64_t)CAST_PX_BYTES;

            for (px_done = 0; px_done < ctx->src_w; px_done = px_done + px_cunt)
            {
                px_cunt = ctx->src_w - px_done > (uint32_t)CAST_CHUNK_PX ? (uint32_t)CAST_CHUNK_PX : ctx->src_w - px_done;

                ret = crosstalk_eda_ic_fpga_mem_byte_rd(fpga, slot_idx, ctx->sig, row_base + (uint64_t)px_done * (uint64_t)CAST_PX_BYTES, chunk, px_cunt * (uint32_t)CAST_PX_BYTES, &rd);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (LDG_UNLIKELY(rd != px_cunt * (uint32_t)CAST_PX_BYTES)) { return LDG_ERR_BOUNDS; }

                for (px = 0; px < px_cunt; px++)
                {
                    blu = (uint32_t)chunk[px * (uint32_t)CAST_PX_BYTES + CAST_PX_B_OFF];
                    grn = (uint32_t)chunk[px * (uint32_t)CAST_PX_BYTES + CAST_PX_G_OFF];
                    red = (uint32_t)chunk[px * (uint32_t)CAST_PX_BYTES + CAST_PX_R_OFF];
                    luma = (red * CAST_LUMA_R_W + grn * CAST_LUMA_G_W + blu) / CAST_LUMA_W_TOTAL;
                    sum[(px_done + px) / cell_w] = sum[(px_done + px) / cell_w] + luma;
                }
            }
        }

        for (cx = 0; cx < (uint32_t)CROSSTALK_EDA_IO_CAST_COLS; cx++)
        {
            avg = sum[cx] / (cell_w * cell_h);
            idx = avg * CAST_RAMP_CUNT / CAST_LUMA_SPAN;
            if (idx >= CAST_RAMP_CUNT) { idx = CAST_RAMP_CUNT - 0x1; }

            payload[cy * (uint32_t)CROSSTALK_EDA_IO_CAST_COLS + cx] = ramp[idx];
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t cast_drain(crosstalk_eda_io_cast_ctx_t *ctx, crosstalk_eda_io_serial_ctx_t *serial)
{
    uint32_t remain = 0;
    uint32_t put = 0;
    uint32_t ret = 0;

    while (ctx->frame_at < ctx->frame_len)
    {
        remain = ctx->frame_len - ctx->frame_at;
        put = remain > (uint32_t)CAST_CHUNK ? (uint32_t)CAST_CHUNK : remain;

        ret = crosstalk_eda_io_serial_tx_put(serial, &ctx->frame[ctx->frame_at], put);
        if (ret == CROSSTALK_EDA_IO_SERIAL_ERR_TX_FULL) { return LDG_ERR_AOK; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->frame_at = ctx->frame_at + put;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_cast_init(crosstalk_eda_io_cast_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_cast_start(crosstalk_eda_io_cast_ctx_t *ctx, const char *sig, uint64_t base, uint32_t src_w, uint32_t src_h)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || sig == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sig[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(src_w == 0 || src_h == 0 || src_w > (uint32_t)CROSSTALK_EDA_IO_CAST_SRC_W_MAX)) { return CROSSTALK_EDA_IO_CAST_ERR_GEOMETRY; }

    if (LDG_UNLIKELY(src_w % (uint32_t)CROSSTALK_EDA_IO_CAST_COLS != 0 || src_h % (uint32_t)CROSSTALK_EDA_IO_CAST_ROWS != 0)) { return CROSSTALK_EDA_IO_CAST_ERR_GEOMETRY; }

    ret = ldg_str_rbrcpy(ctx->sig, sig, (uint64_t)sizeof(ctx->sig));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    ctx->base = base;
    ctx->src_w = src_w;
    ctx->src_h = src_h;
    ctx->prev_hash = 0;
    ctx->frame_cunt = 0;
    ctx->frame_len = 0;
    ctx->frame_at = 0;
    ctx->next = 0.0;
    ctx->on = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_io_cast_stop(crosstalk_eda_io_cast_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->on = LDG_TRUTH_FALSE;
    ctx->frame_len = 0;
    ctx->frame_at = 0;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_io_cast_on_is(const crosstalk_eda_io_cast_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->on != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_io_cast_tick(crosstalk_eda_io_cast_ctx_t *ctx, const crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, crosstalk_eda_io_serial_ctx_t *serial, double now)
{
    uint8_t payload[CROSSTALK_EDA_IO_CAST_PAYLOAD] = LDG_ARR_ZERO_INIT;
    uint32_t hash = 0;
    uint32_t checksum = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || fpga == 0x0 || serial == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->on == 0) { return LDG_ERR_AOK; }

    ret = cast_drain(ctx, serial);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->frame_len = 0; ctx->frame_at = 0; return ret; }

    if (ctx->frame_at < ctx->frame_len) { return LDG_ERR_AOK; }

    if (now < ctx->next) { return LDG_ERR_AOK; }

    ctx->next = now + CROSSTALK_EDA_IO_CAST_PERIOD_SECS;

    if (crosstalk_eda_io_serial_attached_is(serial) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = cast_payload_build(ctx, fpga, slot_idx, payload);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    hash = ldg_emiemi_fnv1a_compute(payload, (uint32_t)CROSSTALK_EDA_IO_CAST_PAYLOAD);
    if (ctx->frame_cunt > 0 && (uint64_t)hash == ctx->prev_hash) { return LDG_ERR_AOK; }

    ret = ldg_emiemi_encode(payload, (uint32_t)CROSSTALK_EDA_IO_CAST_PAYLOAD, ctx->frame, (uint32_t)sizeof(ctx->frame), &checksum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->frame_len = (uint32_t)CROSSTALK_EDA_IO_CAST_PAYLOAD + (uint32_t)LDG_EMIEMI_HDR_LEN + (uint32_t)LDG_EMIEMI_END_MARKER_LEN;
    ctx->frame_at = 0;
    ctx->prev_hash = (uint64_t)hash;
    ctx->frame_cunt = ctx->frame_cunt + 0x1;

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "cast: frame queued; cunt: %u64; checksum: %08X32", ctx->frame_cunt, checksum);

    ret = cast_drain(ctx, serial);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ctx->frame_len = 0; ctx->frame_at = 0; }

    return ret;
}
