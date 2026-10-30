#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/io/file.h>
#include <dangling/proto/emiemi.h>

#include "core/log.h"
#include "game/pack.h"

#define CROSSTALK_GAME_PACK_NAME_LEN_SIZE 4
#define CROSSTALK_GAME_PACK_RD_CHUNK 4096
#define CROSSTALK_GAME_PACK_RD_MODE 0U
#define CROSSTALK_GAME_PACK_ENTRY_STEP 1U

static uint32_t pack_name_len_get(const char *name, uint32_t *out)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = (uint64_t)strlen(name);
    if (LDG_UNLIKELY(len == 0 || len >= CROSSTALK_GAME_PACK_NAME_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = (uint32_t)len;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_pack_init(crosstalk_game_pack_ctx_t *ctx, uint8_t *buff, uint64_t cap)
{
    if (LDG_UNLIKELY(ctx == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->buff = buff;
    ctx->cap = cap;
    ctx->len = 0;
    ctx->entry_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_pack_blob_add(crosstalk_game_pack_ctx_t *ctx, const char *name, const uint8_t *data, uint64_t data_len)
{
    uint8_t *inner_buff = 0x0;
    uint64_t inner_len = 0;
    uint64_t frame_len = 0;
    uint32_t name_len = 0;
    uint32_t checksum = 0;
    uint32_t shift_i = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->buff == 0x0 || data == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->entry_cunt >= CROSSTALK_GAME_PACK_ENTRY_CAP)) { return LDG_ERR_FULL; }

    ret = pack_name_len_get(name, &name_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    inner_len = (uint64_t)CROSSTALK_GAME_PACK_NAME_LEN_SIZE + (uint64_t)name_len + data_len;
    if (LDG_UNLIKELY(inner_len > (uint64_t)LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_OVERFLOW; }

    frame_len = (uint64_t)ldg_emiemi_encoded_size_get((ldg_dword_t)inner_len);
    if (LDG_UNLIKELY(frame_len == 0)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(ctx->len + frame_len + inner_len > ctx->cap)) { return LDG_ERR_FULL; }

    inner_buff = ctx->buff + ctx->cap - inner_len;
    for (shift_i = 0; shift_i < CROSSTALK_GAME_PACK_NAME_LEN_SIZE; shift_i = shift_i + 1) { inner_buff[shift_i] = (uint8_t)((name_len >> (shift_i * LDG_BYTE_BITS)) & LDG_BYTE_MASK); }

    memcpy(inner_buff + CROSSTALK_GAME_PACK_NAME_LEN_SIZE, name, (uint64_t)name_len);
    if (data_len > 0) { memmove(inner_buff + CROSSTALK_GAME_PACK_NAME_LEN_SIZE + name_len, data, data_len); }

    ret = ldg_emiemi_encode(inner_buff, (ldg_dword_t)inner_len, ctx->buff + ctx->len, (ldg_dword_t)frame_len, &checksum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->len = ctx->len + frame_len;
    ctx->entry_cunt = ctx->entry_cunt + CROSSTALK_GAME_PACK_ENTRY_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_pack_file_add(crosstalk_game_pack_ctx_t *ctx, const char *name, const char *path)
{
    ldg_io_file_t *file = 0x0;
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    uint8_t *dst = 0x0;
    uint64_t got = 0;
    uint64_t done = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->buff == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_path_info_get(path, &info);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (LDG_UNLIKELY(info.size == 0)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(ctx->len + info.size > ctx->cap)) { return LDG_ERR_FULL; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_GAME_PACK_RD_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    dst = ctx->buff + ctx->cap - info.size;
    while (done < info.size)
    {
        uint64_t want = info.size - done;

        if (want > CROSSTALK_GAME_PACK_RD_CHUNK) { want = CROSSTALK_GAME_PACK_RD_CHUNK; }

        ret = ldg_io_file_rd(file, dst + done, want, &got);
        if (ret != LDG_ERR_AOK || got == 0) { break; }

        done = done + got;
    }

    if (LDG_UNLIKELY(ldg_io_file_close(file) != LDG_ERR_AOK)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "pack file close failed"); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (done != info.size) { return LDG_ERR_IO_RD; }

    return crosstalk_game_pack_blob_add(ctx, name, dst, info.size);
}

uint64_t crosstalk_game_pack_sealed_size_get(const crosstalk_game_pack_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT64_MAX; }

    return (uint64_t)ldg_emiemi_encoded_size_get((ldg_dword_t)ctx->len);
}

uint32_t crosstalk_game_pack_seal(crosstalk_game_pack_ctx_t *ctx, uint8_t *out, uint64_t out_cap, uint64_t *out_len, uint32_t *out_checksum)
{
    uint64_t sealed = 0;
    uint32_t checksum = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->buff == 0x0 || out == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->entry_cunt == 0)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(ctx->len > (uint64_t)LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_OVERFLOW; }

    sealed = (uint64_t)ldg_emiemi_encoded_size_get((ldg_dword_t)ctx->len);
    if (LDG_UNLIKELY(sealed == 0 || sealed > out_cap)) { return LDG_ERR_FULL; }

    ret = ldg_emiemi_encode(ctx->buff, (ldg_dword_t)ctx->len, out, (ldg_dword_t)sealed, &checksum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = sealed;
    if (out_checksum != 0x0) { *out_checksum = checksum; }

    return LDG_ERR_AOK;
}
