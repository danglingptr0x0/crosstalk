#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#include "eda/ic/fpga/remote.h"

#define REMOTE_STEP 1U
#define REMOTE_ALIGN 8U
#define REMOTE_HASH_NONE 0ULL
#define REMOTE_NAME_CAP 64
#define REMOTE_WORDS_MAX 24575U
#define REMOTE_BYTES_MAX 196600U
#define REMOTE_WR_WORDS_MAX 500U
#define REMOTE_FRAME_MAX 16777216ULL
#define REMOTE_SLOT_NONE 0U
#define REMOTE_KIND_NONE 0U
#define REMOTE_ARG_NONE 0ULL
#define REMOTE_EXTRA_NONE 0U
#define REMOTE_TOP_CAP CROSSTALK_EDA_IC_FPGA_TOP_CAP
#define REMOTE_IO_MAP_CAP CROSSTALK_EDA_IC_FPGA_REMOTE_IO_MAP_CAP
#define REMOTE_SRV_OUT_OFF 196608ULL
#define REMOTE_SRV_OUT_CAP 197120U
#define REMOTE_SRV_FRAME_OFF 397312ULL
#define REMOTE_PUDDING_SIZE 4
#define REMOTE_CHUNK_PROBE 0U

typedef enum remote_func
{
    REMOTE_FUNC_NONE = 0,
    REMOTE_FUNC_SLOT_FREQ_GET,
    REMOTE_FUNC_SLOT_HLT_IS,
    REMOTE_FUNC_CYCLES_TOTAL_GET,
    REMOTE_FUNC_VERDICT_GET,
    REMOTE_FUNC_PIN_RD,
    REMOTE_FUNC_SLOT_LIVE_IS,
    REMOTE_FUNC_RUNNING_IS,
    REMOTE_FUNC_RUNNING_TOP_GET,
    REMOTE_FUNC_SIG_PEEK,
    REMOTE_FUNC_MEM_WIDTH_GET,
    REMOTE_FUNC_MEM_OFFSET_GET,
    REMOTE_FUNC_MEM_RD,
    REMOTE_FUNC_PROF_GET,
    REMOTE_FUNC_DSU_GET,
    REMOTE_FUNC_MEM_BYTE_RD,
    REMOTE_FUNC_EMU_STATE_GET,
    REMOTE_FUNC_MEM_ACC_GET,
    REMOTE_FUNC_AUDIO_RATE_GET,
    REMOTE_FUNC_MEM_WATCH_GET,
    REMOTE_FUNC_FAULT_GET,
    REMOTE_FUNC_RING_GET,
    REMOTE_FUNC_DDU_FRAME_GET,
    REMOTE_FUNC_DDU_TEXT_GET,
    REMOTE_FUNC_IO_RD,
    REMOTE_FUNC_IO_MAP_GET,
    REMOTE_FUNC_IR_GET,
    REMOTE_FUNC_RUN_RATE_GET,
    REMOTE_FUNC_RUN_PIN_GET,
    REMOTE_FUNC_RUN_CYCLES_GET,
    REMOTE_FUNC_RUN_LIVE_IS,
    REMOTE_FUNC_SLOT_HLT_SET,
    REMOTE_FUNC_SLOT_GATE_SET,
    REMOTE_FUNC_CLK_DOMAIN_SET,
    REMOTE_FUNC_PIN_WR,
    REMOTE_FUNC_SIG_POKE,
    REMOTE_FUNC_PROF_EN_SET,
    REMOTE_FUNC_MEM_WATCH_CLEAR,
    REMOTE_FUNC_BRK_SET,
    REMOTE_FUNC_IO_WR,
    REMOTE_FUNC_WATCH_SET,
    REMOTE_FUNC_BRK_SKIP_SET,
    REMOTE_FUNC_BRK_COND_SET,
    REMOTE_FUNC_EMU_STEP_SET,
    REMOTE_FUNC_PROF_EN,
    REMOTE_FUNC_HLT_MASK_SET,
    REMOTE_FUNC_MEM_WR,
    REMOTE_FUNC_RUN_START,
    REMOTE_FUNC_RUN_HLT,
    REMOTE_FUNC_RUN_PIN_SET
} remote_func_t;

typedef struct remote_key
{
    uint64_t arg0;
    uint64_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    char name[REMOTE_NAME_CAP];
} remote_key_t;

typedef struct remote_words_hdr
{
    uint32_t cunt;
    uint8_t pudding[REMOTE_PUDDING_SIZE];
} remote_words_hdr_t;

typedef struct remote_dims_hdr
{
    uint32_t width;
    uint32_t height;
} remote_dims_hdr_t;

typedef struct remote_val_width
{
    uint64_t val;
    uint32_t width;
    uint8_t pudding[REMOTE_PUDDING_SIZE];
} remote_val_width_t;

typedef struct remote_io_map
{
    uint64_t base;
    uint64_t span;
    char name[REMOTE_NAME_CAP];
} remote_io_map_t;

typedef struct remote_ir_probe
{
    uint32_t total;
    uint32_t gen;
} remote_ir_probe_t;

typedef struct remote_ir_hdr
{
    uint32_t sig_cunt;
    uint32_t text_len;
    uint32_t gen;
    uint32_t total;
} remote_ir_hdr_t;

static uint64_t remote_key_hash(uint32_t func, uint32_t slot, const void *key, uint32_t key_len)
{
    uint8_t buff[CROSSTALK_EDA_IC_FPGA_REMOTE_KEY_CAP + sizeof(uint32_t) + sizeof(uint32_t)] = LDG_ARR_ZERO_INIT;
    uint64_t hash = 0;

    memcpy(buff, &func, sizeof(func));
    memcpy(buff + sizeof(func), &slot, sizeof(slot));
    if (key_len > 0) { memcpy(buff + sizeof(func) + sizeof(slot), key, key_len); }

    if (ldg_hash_fnv1a_64_compute(buff, (uint64_t)(sizeof(func) + sizeof(slot) + key_len), &hash) != LDG_ERR_AOK) { return REMOTE_HASH_NONE; }

    return hash;
}

static uint32_t remote_entry_find(const crosstalk_eda_ic_fpga_remote_t *remote, uint64_t hash, uint32_t func, uint32_t slot, const void *key, uint32_t key_len)
{
    for (uint32_t idx = 0; idx < remote->entry_cunt; idx++)
    {
        const crosstalk_eda_ic_fpga_remote_entry_t *entry = &remote->entry[idx];

        if (entry->live != LDG_TRUTH_TRUE || entry->key_hash != hash || entry->func != func || entry->slot != slot || entry->key_len != key_len) { continue; }

        if (key_len > 0 && memcmp(entry->key, key, key_len) != 0) { continue; }

        return idx;
    }

    return CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE;
}

static void remote_cache_rst(crosstalk_eda_ic_fpga_remote_t *remote)
{
    for (uint32_t idx = 0; idx < CROSSTALK_EDA_IC_FPGA_REMOTE_ENTRY_CAP; idx++) { remote->entry[idx].live = LDG_TRUTH_FALSE; }

    remote->entry_cunt = 0;
    remote->arena_used = 0;
    remote->req_cursor = 0;
}

static uint32_t remote_entry_new(crosstalk_eda_ic_fpga_remote_t *remote, uint64_t hash, uint32_t func, uint32_t slot, const void *key, uint32_t key_len, uint32_t val_cap, uint32_t *out_idx)
{
    crosstalk_eda_ic_fpga_remote_entry_t *entry = 0x0;
    uint64_t need = ((uint64_t)val_cap + REMOTE_ALIGN - REMOTE_STEP) & ~((uint64_t)REMOTE_ALIGN - REMOTE_STEP);
    uint32_t idx = CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE;

    *out_idx = CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE;

    for (uint32_t at = 0; at < remote->entry_cunt; at++) { if (remote->entry[at].live != LDG_TRUTH_TRUE && remote->entry[at].val_cap >= val_cap) { idx = at; break; } }

    if (idx == CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE)
    {
        if (remote->entry_cunt >= CROSSTALK_EDA_IC_FPGA_REMOTE_ENTRY_CAP || remote->arena_used + need > (uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_ARENA_SIZE) { remote_cache_rst(remote); }

        if (remote->arena_used + need > (uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_ARENA_SIZE) { return LDG_ERR_FULL; }

        idx = remote->entry_cunt;
        remote->entry_cunt = remote->entry_cunt + REMOTE_STEP;
        remote->entry[idx].val_off = remote->arena_used;
        remote->entry[idx].val_cap = val_cap;
        remote->arena_used = remote->arena_used + need;
    }

    entry = &remote->entry[idx];
    entry->key_hash = hash;
    entry->func = func;
    entry->slot = slot;
    entry->key_len = key_len;
    entry->val_len = 0;
    entry->ret = LDG_ERR_NOT_INIT;
    entry->recv_ms = 0;
    entry->req_ms = 0;
    entry->used_ms = remote->now_ms;
    entry->recv = LDG_TRUTH_FALSE;
    entry->live = LDG_TRUTH_TRUE;

    if (key_len > 0) { memcpy(entry->key, key, key_len); }

    *out_idx = idx;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_init(crosstalk_eda_ic_fpga_remote_t *remote)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(remote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(remote, 0x0, sizeof(*remote));

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_ARENA_SIZE, (void **)&remote->arena);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        remote->arena = 0x0;
        return ret;
    }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_VAL_MAX, (void **)&remote->buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(remote->arena);
        remote->arena = 0x0;
        remote->buff = 0x0;
        return ret;
    }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_FB_WORDS_MAX * (uint64_t)sizeof(uint64_t), (void **)&remote->fb);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(remote->buff);
        ldg_mem_dealloc(remote->arena);
        remote->arena = 0x0;
        remote->buff = 0x0;
        remote->fb = 0x0;
        return ret;
    }

    ret = ldg_mem_alloc((uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_IR_MAX, (void **)&remote->ir_blob);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(remote->fb);
        ldg_mem_dealloc(remote->buff);
        ldg_mem_dealloc(remote->arena);
        remote->arena = 0x0;
        remote->buff = 0x0;
        remote->fb = 0x0;
        remote->ir_blob = 0x0;
    }

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_remote_shutdown(crosstalk_eda_ic_fpga_remote_t *remote)
{
    if (LDG_UNLIKELY(remote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (remote->arena != 0x0) { ldg_mem_dealloc(remote->arena); }

    if (remote->buff != 0x0) { ldg_mem_dealloc(remote->buff); }

    if (remote->fb != 0x0) { ldg_mem_dealloc(remote->fb); }

    if (remote->ir_blob != 0x0) { ldg_mem_dealloc(remote->ir_blob); }

    remote->arena = 0x0;
    remote->buff = 0x0;
    remote->fb = 0x0;
    remote->ir_blob = 0x0;
    remote->ir_ok = LDG_TRUTH_FALSE;
    remote->on = LDG_TRUTH_FALSE;
    remote_cache_rst(remote);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_on_set(crosstalk_eda_ic_fpga_remote_t *remote, uint8_t on)
{
    if (LDG_UNLIKELY(remote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(on == LDG_TRUTH_TRUE && remote->arena == 0x0)) { return LDG_ERR_NOT_INIT; }

    remote_cache_rst(remote);
    remote->wr_hd = 0;
    remote->wr_cunt = 0;
    remote->ir_ok = LDG_TRUTH_FALSE;
    remote->on = on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ic_fpga_remote_on_is(const crosstalk_eda_ic_fpga_remote_t *remote)
{
    if (LDG_UNLIKELY(remote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return remote->on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_eda_ic_fpga_remote_tick(crosstalk_eda_ic_fpga_remote_t *remote, uint64_t now_ms)
{
    if (LDG_UNLIKELY(remote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    remote->now_ms = now_ms;

    for (uint32_t idx = 0; idx < remote->entry_cunt; idx++)
    {
        crosstalk_eda_ic_fpga_remote_entry_t *entry = &remote->entry[idx];

        if (entry->live == LDG_TRUTH_TRUE && now_ms > entry->used_ms + (uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_EVICT_MS) { entry->live = LDG_TRUTH_FALSE; }
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_get(crosstalk_eda_ic_fpga_remote_t *remote, uint32_t func, uint32_t slot, const void *key, uint32_t key_len, void *out_val, uint32_t val_cap, uint32_t *out_len)
{
    crosstalk_eda_ic_fpga_remote_entry_t *entry = 0x0;
    uint64_t hash = 0;
    uint32_t idx = CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(remote == 0x0 || (key_len > 0 && key == 0x0) || (val_cap > 0 && out_val == 0x0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_len != 0x0) { *out_len = 0; }

    if (LDG_UNLIKELY(key_len > CROSSTALK_EDA_IC_FPGA_REMOTE_KEY_CAP || val_cap > CROSSTALK_EDA_IC_FPGA_REMOTE_VAL_MAX)) { return LDG_ERR_OVERFLOW; }

    if (remote->on != LDG_TRUTH_TRUE || remote->arena == 0x0) { return LDG_ERR_NOT_INIT; }

    hash = remote_key_hash(func, slot, key, key_len);
    idx = remote_entry_find(remote, hash, func, slot, key, key_len);

    if (idx == CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE)
    {
        ret = remote_entry_new(remote, hash, func, slot, key, key_len, val_cap, &idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    entry = &remote->entry[idx];
    entry->used_ms = remote->now_ms;

    if (entry->recv != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (val_cap > 0 && entry->val_len > 0) { memcpy(out_val, remote->arena + entry->val_off, entry->val_len < val_cap ? entry->val_len : val_cap); }

    if (out_len != 0x0) { *out_len = entry->val_len < val_cap ? entry->val_len : val_cap; }

    return entry->ret;
}

uint32_t crosstalk_eda_ic_fpga_remote_wr(crosstalk_eda_ic_fpga_remote_t *remote, uint32_t func, uint32_t slot, const void *args, uint32_t args_len)
{
    crosstalk_eda_ic_fpga_remote_wr_t *wr = 0x0;

    if (LDG_UNLIKELY(remote == 0x0 || (args_len > 0 && args == 0x0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(args_len > CROSSTALK_EDA_IC_FPGA_REMOTE_WR_ARGS_CAP)) { return LDG_ERR_OVERFLOW; }

    if (remote->on != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (remote->wr_cunt >= CROSSTALK_EDA_IC_FPGA_REMOTE_WR_CAP) { return LDG_ERR_FULL; }

    wr = &remote->wr[(remote->wr_hd + remote->wr_cunt) % CROSSTALK_EDA_IC_FPGA_REMOTE_WR_CAP];
    wr->func = func;
    wr->slot = slot;
    wr->args_len = args_len;

    if (args_len > 0) { memcpy(wr->args, args, args_len); }

    remote->wr_cunt = remote->wr_cunt + REMOTE_STEP;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_out_next(crosstalk_eda_ic_fpga_remote_t *remote, uint8_t *out, uint32_t cap, uint32_t *out_len, uint32_t *out_kind)
{
    if (LDG_UNLIKELY(remote == 0x0 || out == 0x0 || out_len == 0x0 || out_kind == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    *out_kind = (uint32_t)CROSSTALK_EDA_IC_FPGA_REMOTE_KIND_NONE;

    if (remote->on != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    if (remote->wr_cunt > 0)
    {
        crosstalk_eda_ic_fpga_remote_wr_t *wr = &remote->wr[remote->wr_hd];
        crosstalk_eda_ic_fpga_remote_wr_hdr_t hdr = LDG_STRUCT_ZERO_INIT;

        if (LDG_UNLIKELY((uint64_t)sizeof(hdr) + (uint64_t)wr->args_len > (uint64_t)cap)) { return LDG_ERR_OVERFLOW; }

        hdr.func = wr->func;
        hdr.slot = wr->slot;
        hdr.args_len = wr->args_len;
        memcpy(out, &hdr, sizeof(hdr));
        if (wr->args_len > 0) { memcpy(out + sizeof(hdr), wr->args, wr->args_len); }

        *out_len = (uint32_t)sizeof(hdr) + wr->args_len;
        *out_kind = (uint32_t)CROSSTALK_EDA_IC_FPGA_REMOTE_KIND_WR;
        remote->wr_hd = (remote->wr_hd + REMOTE_STEP) % CROSSTALK_EDA_IC_FPGA_REMOTE_WR_CAP;
        remote->wr_cunt = remote->wr_cunt - REMOTE_STEP;

        return LDG_ERR_AOK;
    }

    for (uint32_t scan = 0; scan < remote->entry_cunt; scan++)
    {
        uint32_t idx = (remote->req_cursor + scan) % remote->entry_cunt;
        crosstalk_eda_ic_fpga_remote_entry_t *entry = &remote->entry[idx];
        crosstalk_eda_ic_fpga_remote_req_hdr_t hdr = LDG_STRUCT_ZERO_INIT;

        if (entry->live != LDG_TRUTH_TRUE) { continue; }

        if (entry->req_ms != 0 && remote->now_ms < entry->req_ms + (uint64_t)CROSSTALK_EDA_IC_FPGA_REMOTE_REFRESH_MS) { continue; }

        if (LDG_UNLIKELY((uint64_t)sizeof(hdr) + (uint64_t)entry->key_len > (uint64_t)cap)) { return LDG_ERR_OVERFLOW; }

        hdr.func = entry->func;
        hdr.slot = entry->slot;
        hdr.key_len = entry->key_len;
        hdr.val_cap = entry->val_cap;
        memcpy(out, &hdr, sizeof(hdr));
        if (entry->key_len > 0) { memcpy(out + sizeof(hdr), entry->key, entry->key_len); }

        entry->req_ms = remote->now_ms;
        remote->req_cursor = (idx + REMOTE_STEP) % remote->entry_cunt;
        *out_len = (uint32_t)sizeof(hdr) + entry->key_len;
        *out_kind = (uint32_t)CROSSTALK_EDA_IC_FPGA_REMOTE_KIND_REQ;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_EMPTY;
}

uint32_t crosstalk_eda_ic_fpga_remote_resp_put(crosstalk_eda_ic_fpga_remote_t *remote, const uint8_t *payload, uint32_t len)
{
    crosstalk_eda_ic_fpga_remote_resp_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    crosstalk_eda_ic_fpga_remote_entry_t *entry = 0x0;
    const uint8_t *key = 0x0;
    uint64_t hash = 0;
    uint32_t idx = CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE;

    if (LDG_UNLIKELY(remote == 0x0 || payload == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (remote->on != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(len < sizeof(hdr))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&hdr, payload, sizeof(hdr));

    if (LDG_UNLIKELY(hdr.key_len > CROSSTALK_EDA_IC_FPGA_REMOTE_KEY_CAP || hdr.val_len > CROSSTALK_EDA_IC_FPGA_REMOTE_VAL_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY((uint64_t)len < (uint64_t)sizeof(hdr) + (uint64_t)hdr.key_len + (uint64_t)hdr.val_len)) { return LDG_ERR_BOUNDS; }

    key = payload + sizeof(hdr);
    hash = remote_key_hash(hdr.func, hdr.slot, key, hdr.key_len);
    idx = remote_entry_find(remote, hash, hdr.func, hdr.slot, key, hdr.key_len);
    if (idx == CROSSTALK_EDA_IC_FPGA_REMOTE_IDX_NONE) { return LDG_ERR_NOT_FOUND; }

    entry = &remote->entry[idx];
    if (hdr.val_len > entry->val_cap) { return LDG_ERR_OVERFLOW; }

    if (hdr.val_len > 0) { memcpy(remote->arena + entry->val_off, key + hdr.key_len, hdr.val_len); }

    entry->val_len = hdr.val_len;
    entry->ret = hdr.ret;
    entry->recv_ms = remote->now_ms;
    entry->recv = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_req_decode(const uint8_t *payload, uint32_t len, crosstalk_eda_ic_fpga_remote_req_hdr_t *out_hdr, const uint8_t **out_key)
{
    if (LDG_UNLIKELY(payload == 0x0 || out_hdr == 0x0 || out_key == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_key = 0x0;

    if (LDG_UNLIKELY(len < sizeof(*out_hdr))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(out_hdr, payload, sizeof(*out_hdr));

    if (LDG_UNLIKELY(out_hdr->key_len > CROSSTALK_EDA_IC_FPGA_REMOTE_KEY_CAP || out_hdr->val_cap > CROSSTALK_EDA_IC_FPGA_REMOTE_VAL_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY((uint64_t)len < (uint64_t)sizeof(*out_hdr) + (uint64_t)out_hdr->key_len)) { return LDG_ERR_BOUNDS; }

    *out_key = payload + sizeof(*out_hdr);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_wr_decode(const uint8_t *payload, uint32_t len, crosstalk_eda_ic_fpga_remote_wr_hdr_t *out_hdr, const uint8_t **out_args)
{
    if (LDG_UNLIKELY(payload == 0x0 || out_hdr == 0x0 || out_args == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_args = 0x0;

    if (LDG_UNLIKELY(len < sizeof(*out_hdr))) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(out_hdr, payload, sizeof(*out_hdr));

    if (LDG_UNLIKELY(out_hdr->args_len > CROSSTALK_EDA_IC_FPGA_REMOTE_WR_ARGS_CAP)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY((uint64_t)len < (uint64_t)sizeof(*out_hdr) + (uint64_t)out_hdr->args_len)) { return LDG_ERR_BOUNDS; }

    *out_args = payload + sizeof(*out_hdr);

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_resp_encode(uint8_t *out, uint32_t cap, const crosstalk_eda_ic_fpga_remote_req_hdr_t *req, const uint8_t *key, uint32_t ret, const void *val, uint32_t val_len, uint32_t *out_len)
{
    crosstalk_eda_ic_fpga_remote_resp_hdr_t hdr = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(out == 0x0 || req == 0x0 || out_len == 0x0 || (req->key_len > 0 && key == 0x0) || (val_len > 0 && val == 0x0))) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(val_len > req->val_cap)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY((uint64_t)sizeof(hdr) + (uint64_t)req->key_len + (uint64_t)val_len > (uint64_t)cap)) { return LDG_ERR_OVERFLOW; }

    hdr.func = req->func;
    hdr.slot = req->slot;
    hdr.key_len = req->key_len;
    hdr.ret = ret;
    hdr.val_len = val_len;

    memcpy(out, &hdr, sizeof(hdr));
    if (req->key_len > 0) { memcpy(out + sizeof(hdr), key, req->key_len); }

    if (val_len > 0) { memcpy(out + sizeof(hdr) + req->key_len, val, val_len); }

    *out_len = (uint32_t)sizeof(hdr) + req->key_len + val_len;

    return LDG_ERR_AOK;
}

static uint32_t remote_fetch(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot, const remote_key_t *key, void *out, uint32_t cap, uint32_t *out_len)
{
    return crosstalk_eda_ic_fpga_remote_get((crosstalk_eda_ic_fpga_remote_t *)ctx->remote, func, slot, key, (uint32_t)sizeof(*key), out, cap, out_len);
}

static uint32_t remote_send(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot, const remote_key_t *key, const void *extra, uint32_t extra_len)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    uint8_t args[CROSSTALK_EDA_IC_FPGA_REMOTE_WR_ARGS_CAP] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY((uint64_t)sizeof(*key) + (uint64_t)extra_len > (uint64_t)sizeof(args))) { return LDG_ERR_OVERFLOW; }

    memcpy(args, key, sizeof(*key));
    if (extra_len > 0) { memcpy(args + sizeof(*key), extra, extra_len); }

    return crosstalk_eda_ic_fpga_remote_wr(remote, func, slot, args, (uint32_t)sizeof(*key) + extra_len);
}

static void remote_key_name_put(remote_key_t *key, const char *name)
{
    uint32_t ret = 0;

    if (name == 0x0) { return; }

    ret = ldg_str_rbrcpy(key->name, name, (uint64_t)sizeof(key->name));
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { key->name[0] = LDG_STR_TERM; }
}

static uint32_t remote_u64_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot, const remote_key_t *key, uint64_t *out)
{
    return remote_fetch(ctx, func, slot, key, out, (uint32_t)sizeof(*out), 0x0);
}

static uint8_t remote_truth_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    uint8_t val = LDG_TRUTH_FALSE;

    if (remote_fetch(ctx, func, slot, &key, &val, (uint32_t)sizeof(val), 0x0) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    return val == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t remote_words_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot, remote_key_t *key, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    uint64_t base = key->arg0;
    uint32_t done = 0;
    uint32_t ret = LDG_ERR_AOK;

    while (done < cunt)
    {
        remote_words_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
        uint32_t chunk = cunt - done < REMOTE_WORDS_MAX ? cunt - done : REMOTE_WORDS_MAX;
        uint32_t got = 0;

        key->arg0 = base + (uint64_t)done;
        key->arg2 = chunk;

        ret = remote_fetch(ctx, func, slot, key, remote->buff, (uint32_t)sizeof(hdr) + chunk * (uint32_t)sizeof(uint64_t), &got);
        if (ret != LDG_ERR_AOK || got < sizeof(hdr)) { break; }

        memcpy(&hdr, remote->buff, sizeof(hdr));
        if (hdr.cunt > chunk) { hdr.cunt = chunk; }

        memcpy(out + done, remote->buff + sizeof(hdr), (uint64_t)hdr.cunt * sizeof(uint64_t));
        done = done + hdr.cunt;

        if (hdr.cunt < chunk) { break; }
    }

    if (out_rd != 0x0) { *out_rd = done; }

    if (done > 0) { return LDG_ERR_AOK; }

    return ret;
}

static uint32_t remote_bytes_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot, uint8_t *dst, uint64_t cap, uint32_t *out_w, uint32_t *out_h)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    uint64_t done = 0;
    uint32_t ret = LDG_ERR_AOK;

    *out_w = 0;
    *out_h = 0;

    if (cap > REMOTE_FRAME_MAX) { cap = REMOTE_FRAME_MAX; }

    while (done < cap)
    {
        remote_dims_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
        uint32_t chunk = cap - done < (uint64_t)REMOTE_BYTES_MAX ? (uint32_t)(cap - done) : REMOTE_BYTES_MAX;
        uint32_t got = 0;

        key.arg0 = done;
        key.arg1 = cap;
        key.arg2 = chunk;

        ret = remote_fetch(ctx, func, slot, &key, remote->buff, (uint32_t)sizeof(hdr) + chunk, &got);
        if (ret != LDG_ERR_AOK || got < sizeof(hdr)) { return ret != LDG_ERR_AOK ? ret : LDG_ERR_NOT_INIT; }

        memcpy(&hdr, remote->buff, sizeof(hdr));
        memcpy(dst + done, remote->buff + sizeof(hdr), got - (uint32_t)sizeof(hdr));
        *out_w = hdr.width;
        *out_h = hdr.height;
        done = done + (uint64_t)chunk;
    }

    return LDG_ERR_AOK;
}

uint8_t crosstalk_eda_ic_fpga_remote_active_is(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->remote == 0x0) { return LDG_TRUTH_FALSE; }

    return crosstalk_eda_ic_fpga_remote_on_is((const crosstalk_eda_ic_fpga_remote_t *)ctx->remote);
}

uint32_t crosstalk_eda_ic_fpga_remote_bind(crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_remote_t *remote)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->remote = remote;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_run_bind(crosstalk_eda_ic_fpga_remote_t *remote, crosstalk_eda_ic_fpga_run_ctx_t *run)
{
    if (LDG_UNLIKELY(remote == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    remote->run = run;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_run_rate_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t *out_hz)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_RUN_RATE_GET, REMOTE_SLOT_NONE, &key, out_hz);
}

uint32_t crosstalk_eda_ic_fpga_remote_run_pin_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t *out)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_RUN_PIN_GET, REMOTE_SLOT_NONE, &key, out);
}

uint32_t crosstalk_eda_ic_fpga_remote_run_cycles_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t *out_cycles)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_RUN_CYCLES_GET, REMOTE_SLOT_NONE, &key, out_cycles);
}

uint8_t crosstalk_eda_ic_fpga_remote_run_live_is(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    return remote_truth_get(ctx, (uint32_t)REMOTE_FUNC_RUN_LIVE_IS, REMOTE_SLOT_NONE);
}

uint32_t crosstalk_eda_ic_fpga_remote_slot_freq_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_SLOT_FREQ_GET, slot_idx, &key, out);
}

uint8_t crosstalk_eda_ic_fpga_remote_slot_hlt_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    return remote_truth_get(ctx, (uint32_t)REMOTE_FUNC_SLOT_HLT_IS, slot_idx);
}

uint8_t crosstalk_eda_ic_fpga_remote_slot_live_is(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    return remote_truth_get(ctx, (uint32_t)REMOTE_FUNC_SLOT_LIVE_IS, slot_idx);
}

uint8_t crosstalk_eda_ic_fpga_remote_running_is(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    return remote_truth_get(ctx, (uint32_t)REMOTE_FUNC_RUNNING_IS, REMOTE_SLOT_NONE);
}

const char* crosstalk_eda_ic_fpga_remote_running_top_get(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    uint32_t got = 0;

    if (remote_fetch(ctx, (uint32_t)REMOTE_FUNC_RUNNING_TOP_GET, REMOTE_SLOT_NONE, &key, remote->top, (uint32_t)sizeof(remote->top), &got) != LDG_ERR_AOK || got == 0) { remote->top[0] = LDG_STR_TERM; }

    remote->top[sizeof(remote->top) - REMOTE_STEP] = LDG_STR_TERM;

    return remote->top;
}

uint32_t crosstalk_eda_ic_fpga_remote_cycles_total_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out_cycles)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_CYCLES_TOTAL_GET, slot_idx, &key, out_cycles);
}

uint32_t crosstalk_eda_ic_fpga_remote_verdict_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, crosstalk_eda_ic_fpga_verdict_t *out)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_fetch(ctx, (uint32_t)REMOTE_FUNC_VERDICT_GET, slot_idx, &key, out, (uint32_t)sizeof(*out), 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_pin_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t *out_val)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    remote_key_name_put(&key, port);

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_PIN_RD, slot_idx, &key, out_val);
}

uint32_t crosstalk_eda_ic_fpga_remote_sig_peek(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_val, uint32_t *out_width)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    remote_val_width_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    remote_key_name_put(&key, sig_name);

    ret = remote_fetch(ctx, (uint32_t)REMOTE_FUNC_SIG_PEEK, slot_idx, &key, &val, (uint32_t)sizeof(val), 0x0);
    *out_val = val.val;
    *out_width = val.width;

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_width_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out_width, uint32_t *out_elem_width)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    remote_val_width_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    remote_key_name_put(&key, sig_name);

    ret = remote_fetch(ctx, (uint32_t)REMOTE_FUNC_MEM_WIDTH_GET, slot_idx, &key, &val, (uint32_t)sizeof(val), 0x0);
    *out_width = val.val;
    *out_elem_width = val.width;

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_offset_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t *out)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    remote_key_name_put(&key, sig_name);

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_MEM_OFFSET_GET, slot_idx, &key, out);
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    remote_key_name_put(&key, sig_name);
    key.arg0 = addr;

    return remote_words_get(ctx, (uint32_t)REMOTE_FUNC_MEM_RD, slot_idx, &key, out, cunt, out_rd);
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_byte_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t byte_at, uint64_t *out, uint32_t cunt, uint32_t *out_rd)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    remote_key_name_put(&key, sig_name);
    key.arg0 = byte_at;

    return remote_words_get(ctx, (uint32_t)REMOTE_FUNC_MEM_BYTE_RD, slot_idx, &key, out, cunt, out_rd);
}

static uint32_t remote_cap_words_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot_idx, uint32_t kind, uint64_t *out, uint32_t cap, uint32_t *out_cunt)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    uint32_t rd = 0;
    uint32_t ret = 0;

    key.arg1 = (uint64_t)kind;

    ret = remote_words_get(ctx, func, slot_idx, &key, out, cap, &rd);
    if (out_cunt != 0x0) { *out_cunt = rd; }

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_remote_dsu_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap)
{
    return remote_cap_words_get(ctx, (uint32_t)REMOTE_FUNC_DSU_GET, slot_idx, REMOTE_KIND_NONE, out, cap, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_emu_state_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t *out, uint32_t cap)
{
    return remote_cap_words_get(ctx, (uint32_t)REMOTE_FUNC_EMU_STATE_GET, slot_idx, kind, out, cap, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_acc_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap)
{
    return remote_cap_words_get(ctx, (uint32_t)REMOTE_FUNC_MEM_ACC_GET, slot_idx, REMOTE_KIND_NONE, out, cap, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_fault_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap)
{
    return remote_cap_words_get(ctx, (uint32_t)REMOTE_FUNC_FAULT_GET, slot_idx, REMOTE_KIND_NONE, out, cap, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_ring_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t *out, uint32_t cap, uint32_t *out_cunt)
{
    return remote_cap_words_get(ctx, (uint32_t)REMOTE_FUNC_RING_GET, slot_idx, REMOTE_KIND_NONE, out, cap, out_cunt);
}

uint32_t crosstalk_eda_ic_fpga_remote_prof_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t kind, uint64_t **out_base, uint32_t *out_cunt)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    uint32_t ret = 0;

    *out_base = 0x0;
    *out_cunt = 0;

    if (kind != (uint32_t)CROSSTALK_HDL_ABI_STATE_EMU_FB) { return LDG_ERR_UNSUPPORTED; }

    ret = remote_cap_words_get(ctx, (uint32_t)REMOTE_FUNC_PROF_GET, slot_idx, kind, remote->fb, CROSSTALK_EDA_IC_FPGA_REMOTE_FB_WORDS_MAX, out_cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_base = remote->fb;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_audio_rate_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t *out)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    return remote_fetch(ctx, (uint32_t)REMOTE_FUNC_AUDIO_RATE_GET, slot_idx, &key, out, (uint32_t)sizeof(*out), 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_watch_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t idx, const crosstalk_eda_ic_fpga_mem_watch_t **out)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    *out = 0x0;

    if (idx >= CROSSTALK_EDA_IC_FPGA_MEM_WATCH_MAX) { return LDG_ERR_BOUNDS; }

    key.arg0 = (uint64_t)idx;

    ret = remote_fetch(ctx, (uint32_t)REMOTE_FUNC_MEM_WATCH_GET, REMOTE_SLOT_NONE, &key, &remote->watch[idx], (uint32_t)sizeof(remote->watch[idx]), 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = &remote->watch[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_ddu_frame_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_w, uint32_t *out_h)
{
    return remote_bytes_get(ctx, (uint32_t)REMOTE_FUNC_DDU_FRAME_GET, slot_idx, dst, cap, out_w, out_h);
}

uint32_t crosstalk_eda_ic_fpga_remote_ddu_text_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, uint32_t *out_cols, uint32_t *out_rows)
{
    return remote_bytes_get(ctx, (uint32_t)REMOTE_FUNC_DDU_TEXT_GET, slot_idx, dst, cap, out_cols, out_rows);
}

uint32_t crosstalk_eda_ic_fpga_remote_io_rd(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t *out)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    key.arg0 = addr;

    return remote_u64_get(ctx, (uint32_t)REMOTE_FUNC_IO_RD, slot_idx, &key, out);
}

uint32_t crosstalk_eda_ic_fpga_remote_io_map_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t idx, uint64_t *out_base, uint64_t *out_span, const char **out_name)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    remote_io_map_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    *out_base = 0;
    *out_span = 0;
    *out_name = 0x0;

    if (idx >= REMOTE_IO_MAP_CAP) { return LDG_ERR_NOT_FOUND; }

    key.arg0 = (uint64_t)idx;

    ret = remote_fetch(ctx, (uint32_t)REMOTE_FUNC_IO_MAP_GET, slot_idx, &key, &val, (uint32_t)sizeof(val), 0x0);
    if (ret != LDG_ERR_AOK) { return ret; }

    val.name[sizeof(val.name) - REMOTE_STEP] = LDG_STR_TERM;
    memcpy(remote->io_name[idx], val.name, sizeof(val.name));
    *out_base = val.base;
    *out_span = val.span;
    *out_name = remote->io_name[idx];

    return LDG_ERR_AOK;
}

static void remote_ir_chunks_drop(crosstalk_eda_ic_fpga_remote_t *remote)
{
    for (uint32_t idx = 0; idx < remote->entry_cunt; idx++)
    {
        crosstalk_eda_ic_fpga_remote_entry_t *entry = &remote->entry[idx];
        remote_key_t key = LDG_STRUCT_ZERO_INIT;

        if (entry->live != LDG_TRUTH_TRUE || entry->func != (uint32_t)REMOTE_FUNC_IR_GET || entry->key_len != (uint32_t)sizeof(key)) { continue; }

        memcpy(&key, entry->key, sizeof(key));
        if (key.arg2 != REMOTE_CHUNK_PROBE) { entry->live = LDG_TRUTH_FALSE; }
    }
}

static uint32_t remote_ir_build(crosstalk_eda_ic_fpga_remote_t *remote)
{
    remote_ir_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    crosstalk_hdl_ir_sig_t *sigs = 0x0;
    uint64_t sig_bytes = 0;

    if (remote->ir_len < (uint32_t)sizeof(hdr)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&hdr, remote->ir_blob, sizeof(hdr));

    sig_bytes = (uint64_t)hdr.sig_cunt * (uint64_t)sizeof(crosstalk_hdl_ir_sig_t);
    if ((uint64_t)sizeof(hdr) + sig_bytes + (uint64_t)hdr.text_len != (uint64_t)remote->ir_len) { return LDG_ERR_BOUNDS; }

    sigs = (crosstalk_hdl_ir_sig_t *)(remote->ir_blob + sizeof(hdr));

    for (uint32_t idx = 0; idx < hdr.sig_cunt; idx++)
    {
        if ((uint64_t)sigs[idx].name.off + (uint64_t)sigs[idx].name.len > (uint64_t)hdr.text_len) { return LDG_ERR_BOUNDS; }

        if ((uint64_t)sigs[idx].mem_file.off + (uint64_t)sigs[idx].mem_file.len > (uint64_t)hdr.text_len) { return LDG_ERR_BOUNDS; }
    }

    memset(&remote->ir, 0x0, sizeof(remote->ir));
    remote->ir.sigs = sigs;
    remote->ir.sig_cunt = hdr.sig_cunt;
    remote->ir.sig_cap = hdr.sig_cunt;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_slot_ir_get(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const crosstalk_hdl_ir_t **out_ir, const char **out_src)
{
    crosstalk_eda_ic_fpga_remote_t *remote = (crosstalk_eda_ic_fpga_remote_t *)ctx->remote;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    remote_ir_probe_t probe = LDG_STRUCT_ZERO_INIT;
    uint64_t done = 0;
    uint32_t got = 0;
    uint32_t ret = 0;
    uint8_t pending = LDG_TRUTH_FALSE;

    *out_ir = 0x0;
    *out_src = 0x0;

    ret = remote_fetch(ctx, (uint32_t)REMOTE_FUNC_IR_GET, slot_idx, &key, &probe, (uint32_t)sizeof(probe), &got);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (got < (uint32_t)sizeof(probe)) { return LDG_ERR_NOT_INIT; }

    if (probe.total > (uint32_t)CROSSTALK_EDA_IC_FPGA_REMOTE_IR_MAX) { return LDG_ERR_OVERFLOW; }

    if (remote->ir_ok != LDG_TRUTH_TRUE || remote->ir_slot != slot_idx || remote->ir_gen != probe.gen)
    {
        remote->ir_ok = LDG_TRUTH_FALSE;

        while (done < (uint64_t)probe.total)
        {
            remote_ir_probe_t part = LDG_STRUCT_ZERO_INIT;
            uint32_t chunk = (uint64_t)probe.total - done < (uint64_t)REMOTE_BYTES_MAX ? (uint32_t)((uint64_t)probe.total - done) : REMOTE_BYTES_MAX;

            key.arg0 = done;
            key.arg1 = (uint64_t)probe.gen;
            key.arg2 = chunk;

            ret = remote_fetch(ctx, (uint32_t)REMOTE_FUNC_IR_GET, slot_idx, &key, remote->buff, (uint32_t)sizeof(part) + chunk, &got);
            if (ret == LDG_ERR_AOK && got == (uint32_t)sizeof(part) + chunk) { memcpy(&part, remote->buff, sizeof(part)); }

            if (ret != LDG_ERR_AOK || got != (uint32_t)sizeof(part) + chunk || part.gen != probe.gen || part.total != probe.total) { pending = LDG_TRUTH_TRUE; }
            else { memcpy(remote->ir_blob + done, remote->buff + sizeof(part), chunk); }

            done = done + (uint64_t)chunk;
        }

        if (pending == LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

        remote->ir_len = probe.total;

        ret = remote_ir_build(remote);
        if (ret != LDG_ERR_AOK) { return ret; }

        remote->ir_gen = probe.gen;
        remote->ir_slot = slot_idx;
        remote->ir_ok = LDG_TRUTH_TRUE;
        remote_ir_chunks_drop(remote);
    }

    *out_ir = &remote->ir;
    *out_src = (const char *)(remote->ir_blob + sizeof(remote_ir_hdr_t) + (uint64_t)remote->ir.sig_cunt * (uint64_t)sizeof(crosstalk_hdl_ir_sig_t));

    return LDG_ERR_AOK;
}

static uint32_t remote_flag_send(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot_idx, uint64_t arg0, uint64_t arg1, uint32_t arg2, const char *name)
{
    remote_key_t key = LDG_STRUCT_ZERO_INIT;

    key.arg0 = arg0;
    key.arg1 = arg1;
    key.arg2 = arg2;
    remote_key_name_put(&key, name);

    return remote_send(ctx, func, slot_idx, &key, 0x0, REMOTE_EXTRA_NONE);
}

uint32_t crosstalk_eda_ic_fpga_remote_slot_hlt_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_SLOT_HLT_SET, slot_idx, (uint64_t)on, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_slot_gate_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_SLOT_GATE_SET, slot_idx, (uint64_t)on, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_clk_domain_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t domain)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_CLK_DOMAIN_SET, slot_idx, (uint64_t)domain, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_pin_wr(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *port, uint64_t val)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_PIN_WR, slot_idx, val, REMOTE_ARG_NONE, REMOTE_KIND_NONE, port);
}

uint32_t crosstalk_eda_ic_fpga_remote_sig_poke(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t val, uint8_t quiet)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_SIG_POKE, slot_idx, val, (uint64_t)quiet, REMOTE_KIND_NONE, sig_name);
}

uint32_t crosstalk_eda_ic_fpga_remote_prof_en_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_PROF_EN_SET, slot_idx, (uint64_t)on, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_prof_en(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t on)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_PROF_EN, slot_idx, (uint64_t)on, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_watch_clear(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_MEM_WATCH_CLEAR, slot_idx, REMOTE_ARG_NONE, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_brk_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint8_t on)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_BRK_SET, slot_idx, addr, (uint64_t)on, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_io_wr(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t addr, uint64_t val)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_IO_WR, slot_idx, addr, val, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_watch_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t lo, uint64_t hi, uint8_t on)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_WATCH_SET, slot_idx, lo, hi, (uint32_t)on, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_brk_skip_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint64_t cunt)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_BRK_SKIP_SET, slot_idx, cunt, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_brk_cond_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t reg, uint64_t min)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_BRK_COND_SET, slot_idx, min, REMOTE_ARG_NONE, (uint32_t)reg, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_emu_step_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t cunt)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_EMU_STEP_SET, REMOTE_SLOT_NONE, cunt, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_hlt_mask_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint32_t mask)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_HLT_MASK_SET, slot_idx, (uint64_t)mask, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_mem_wr(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, const char *sig_name, uint64_t addr, const uint64_t *vals, uint32_t cunt, uint32_t *out_wrote)
{
    uint32_t done = 0;
    uint32_t ret = LDG_ERR_AOK;

    *out_wrote = 0;

    while (done < cunt)
    {
        remote_key_t key = LDG_STRUCT_ZERO_INIT;
        uint32_t chunk = cunt - done < REMOTE_WR_WORDS_MAX ? cunt - done : REMOTE_WR_WORDS_MAX;

        key.arg0 = addr + (uint64_t)done;
        key.arg2 = chunk;
        remote_key_name_put(&key, sig_name);

        ret = remote_send(ctx, (uint32_t)REMOTE_FUNC_MEM_WR, slot_idx, &key, vals + done, chunk * (uint32_t)sizeof(uint64_t));
        if (ret != LDG_ERR_AOK) { break; }

        done = done + chunk;
    }

    *out_wrote = done;

    return ret;
}

uint32_t crosstalk_eda_ic_fpga_remote_run_start(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_RUN_START, REMOTE_SLOT_NONE, REMOTE_ARG_NONE, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_run_hlt(const crosstalk_eda_ic_fpga_ctx_t *ctx)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_RUN_HLT, REMOTE_SLOT_NONE, REMOTE_ARG_NONE, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

uint32_t crosstalk_eda_ic_fpga_remote_run_pin_set(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint64_t hz)
{
    return remote_flag_send(ctx, (uint32_t)REMOTE_FUNC_RUN_PIN_SET, REMOTE_SLOT_NONE, hz, REMOTE_ARG_NONE, REMOTE_KIND_NONE, 0x0);
}

static uint32_t remote_serve_words(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t func, uint32_t slot_idx, const remote_key_t *key, uint8_t *val, uint32_t val_cap, uint32_t *out_len)
{
    remote_words_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint64_t *words = (uint64_t *)(val + sizeof(hdr));
    uint32_t cap = key->arg2;
    uint32_t rd = 0;
    uint32_t ret = 0;

    *out_len = 0;

    if ((uint64_t)sizeof(hdr) + (uint64_t)cap * sizeof(uint64_t) > (uint64_t)val_cap) { return LDG_ERR_OVERFLOW; }

    switch (func)
    {
        case REMOTE_FUNC_MEM_RD:
            ret = crosstalk_eda_ic_fpga_mem_rd(ctx, slot_idx, key->name, key->arg0, words, cap, &rd);
            break;
        case REMOTE_FUNC_MEM_BYTE_RD:
            ret = crosstalk_eda_ic_fpga_mem_byte_rd(ctx, slot_idx, key->name, key->arg0, words, cap, &rd);
            break;
        case REMOTE_FUNC_DSU_GET:
            ret = crosstalk_eda_ic_fpga_dsu_get(ctx, slot_idx, words, cap);
            rd = cap;
            break;
        case REMOTE_FUNC_EMU_STATE_GET:
            ret = crosstalk_eda_ic_fpga_emu_state_get(ctx, slot_idx, (uint32_t)key->arg1, words, cap);
            rd = cap;
            break;
        case REMOTE_FUNC_MEM_ACC_GET:
            ret = crosstalk_eda_ic_fpga_mem_acc_get(ctx, slot_idx, words, cap);
            rd = cap;
            break;
        case REMOTE_FUNC_FAULT_GET:
            ret = crosstalk_eda_ic_fpga_fault_get(ctx, slot_idx, words, cap);
            rd = cap;
            break;
        case REMOTE_FUNC_RING_GET:
            ret = crosstalk_eda_ic_fpga_ring_get(ctx, slot_idx, words, cap, &rd);
            break;
        default:
        {
            uint64_t *base = 0x0;
            uint32_t cunt = 0;

            if (key->arg1 != (uint64_t)CROSSTALK_HDL_ABI_STATE_EMU_FB) { return LDG_ERR_UNSUPPORTED; }

            ret = crosstalk_eda_ic_fpga_prof_get(ctx, slot_idx, (uint32_t)key->arg1, &base, &cunt);
            if (ret == LDG_ERR_AOK && base != 0x0 && key->arg0 < (uint64_t)cunt)
            {
                uint64_t remain = (uint64_t)cunt - key->arg0;

                rd = remain < (uint64_t)cap ? (uint32_t)remain : cap;
                memcpy(words, base + key->arg0, (uint64_t)rd * sizeof(uint64_t));
            }
        }
        break;
    }

    if (rd > cap) { rd = cap; }

    hdr.cunt = rd;
    memcpy(val, &hdr, sizeof(hdr));
    *out_len = (uint32_t)sizeof(hdr) + rd * (uint32_t)sizeof(uint64_t);

    return ret;
}

static uint32_t remote_serve_bytes(const crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_remote_t *srv, uint32_t func, uint32_t slot_idx, const remote_key_t *key, uint8_t *val, uint32_t val_cap, uint32_t *out_len)
{
    remote_dims_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint8_t *frame = srv->arena + REMOTE_SRV_FRAME_OFF;
    uint64_t frame_cap = key->arg1;
    uint64_t off = key->arg0;
    uint32_t chunk = key->arg2;
    uint32_t ret = 0;

    *out_len = 0;

    if (frame_cap > REMOTE_FRAME_MAX || off + (uint64_t)chunk > frame_cap || (uint64_t)sizeof(hdr) + (uint64_t)chunk > (uint64_t)val_cap) { return LDG_ERR_OVERFLOW; }

    if (func == (uint32_t)REMOTE_FUNC_DDU_FRAME_GET) { ret = crosstalk_eda_ic_fpga_ddu_frame_get(ctx, slot_idx, frame, frame_cap, &hdr.width, &hdr.height); }
    else { ret = crosstalk_eda_ic_fpga_ddu_text_get(ctx, slot_idx, frame, frame_cap, &hdr.width, &hdr.height); }

    memcpy(val, &hdr, sizeof(hdr));
    memcpy(val + sizeof(hdr), frame + off, chunk);
    *out_len = (uint32_t)sizeof(hdr) + chunk;

    return ret;
}

static uint32_t remote_ir_tok_pack(crosstalk_hdl_tok_t *tok, const char *src, uint32_t src_len, uint8_t *text, uint64_t text_cap, uint32_t *io_len)
{
    if (tok->len == 0)
    {
        tok->off = 0;
        return LDG_ERR_AOK;
    }

    if ((uint64_t)tok->off + (uint64_t)tok->len > (uint64_t)src_len) { return LDG_ERR_BOUNDS; }

    if ((uint64_t)*io_len + (uint64_t)tok->len + (uint64_t)LDG_STR_TERM_SIZE > text_cap) { return LDG_ERR_OVERFLOW; }

    memcpy(text + *io_len, src + tok->off, tok->len);
    text[*io_len + tok->len] = (uint8_t)LDG_STR_TERM;
    tok->off = *io_len;
    *io_len = *io_len + tok->len + (uint32_t)LDG_STR_TERM_SIZE;

    return LDG_ERR_AOK;
}

static uint32_t remote_ir_pack(const crosstalk_eda_ic_fpga_ctx_t *ctx, uint32_t slot_idx, uint8_t *dst, uint64_t cap, remote_ir_hdr_t *out_hdr)
{
    const crosstalk_hdl_ir_t *ir = 0x0;
    const char *src = 0x0;
    remote_ir_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint64_t sig_bytes = 0;
    uint64_t text_cap = 0;
    uint8_t *text = 0x0;
    uint32_t src_len = 0;
    uint32_t ret = 0;

    ret = crosstalk_eda_ic_fpga_slot_ir_get(ctx, slot_idx, &ir, &src);
    if (ret != LDG_ERR_AOK) { return ret; }

    sig_bytes = (uint64_t)ir->sig_cunt * (uint64_t)sizeof(crosstalk_hdl_ir_sig_t);
    if ((uint64_t)sizeof(hdr) + sig_bytes > cap) { return LDG_ERR_OVERFLOW; }

    src_len = ctx->slot[slot_idx].elab_src_len;
    text = dst + sizeof(hdr) + sig_bytes;
    text_cap = cap - (uint64_t)sizeof(hdr) - sig_bytes;

    for (uint32_t idx = 0; idx < ir->sig_cunt; idx++)
    {
        crosstalk_hdl_ir_sig_t sig = ir->sigs[idx];

        ret = remote_ir_tok_pack(&sig.name, src, src_len, text, text_cap, &hdr.text_len);
        if (ret == LDG_ERR_AOK) { ret = remote_ir_tok_pack(&sig.mem_file, src, src_len, text, text_cap, &hdr.text_len); }

        if (ret != LDG_ERR_AOK) { return ret; }

        sig.qual_off = 0;
        sig.qual_len = 0;
        memcpy(dst + sizeof(hdr) + (uint64_t)idx * (uint64_t)sizeof(sig), &sig, sizeof(sig));
    }

    hdr.sig_cunt = ir->sig_cunt;
    hdr.gen = ctx->slot[slot_idx].prog_gen;
    hdr.total = (uint32_t)((uint64_t)sizeof(hdr) + sig_bytes + (uint64_t)hdr.text_len);
    memcpy(dst, &hdr, sizeof(hdr));
    *out_hdr = hdr;

    return LDG_ERR_AOK;
}

static uint32_t remote_serve_ir(const crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_remote_t *srv, uint32_t slot_idx, const remote_key_t *key, uint8_t *val, uint32_t val_cap, uint32_t *out_len)
{
    remote_ir_probe_t probe = LDG_STRUCT_ZERO_INIT;
    remote_ir_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    uint8_t *blob = srv->arena + REMOTE_SRV_FRAME_OFF;
    uint32_t chunk = key->arg2;
    uint32_t ret = 0;

    *out_len = 0;

    if ((uint64_t)sizeof(probe) + (uint64_t)chunk > (uint64_t)val_cap) { return LDG_ERR_OVERFLOW; }

    ret = remote_ir_pack(ctx, slot_idx, blob, REMOTE_FRAME_MAX, &hdr);
    if (ret != LDG_ERR_AOK) { return ret; }

    probe.total = hdr.total;
    probe.gen = hdr.gen;
    memcpy(val, &probe, sizeof(probe));
    *out_len = (uint32_t)sizeof(probe);

    if (chunk == REMOTE_CHUNK_PROBE || key->arg1 != (uint64_t)hdr.gen || key->arg0 + (uint64_t)chunk > (uint64_t)hdr.total) { return LDG_ERR_AOK; }

    memcpy(val + sizeof(probe), blob + key->arg0, chunk);
    *out_len = (uint32_t)sizeof(probe) + chunk;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_serve(const crosstalk_eda_ic_fpga_ctx_t *ctx, crosstalk_eda_ic_fpga_remote_t *srv, const uint8_t *payload, uint32_t len, const uint8_t **out_frame, uint32_t *out_len)
{
    crosstalk_eda_ic_fpga_remote_req_hdr_t req = LDG_STRUCT_ZERO_INIT;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    const uint8_t *key_raw = 0x0;
    uint8_t *val = 0x0;
    uint8_t *frame_out = 0x0;
    uint32_t val_len = 0;
    uint32_t res = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || srv == 0x0 || payload == 0x0 || out_frame == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_frame = 0x0;
    *out_len = 0;

    if (LDG_UNLIKELY(srv->arena == 0x0)) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_eda_ic_fpga_remote_req_decode(payload, len, &req, &key_raw);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (req.key_len != (uint32_t)sizeof(key)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&key, key_raw, sizeof(key));
    key.name[sizeof(key.name) - REMOTE_STEP] = LDG_STR_TERM;

    val = srv->arena;
    frame_out = srv->arena + REMOTE_SRV_OUT_OFF;

    switch (req.func)
    {
        case REMOTE_FUNC_SLOT_FREQ_GET:
            res = crosstalk_eda_ic_fpga_slot_freq_get(ctx, req.slot, (uint64_t *)val);
            val_len = (uint32_t)sizeof(uint64_t);
            break;
        case REMOTE_FUNC_CYCLES_TOTAL_GET:
            res = crosstalk_eda_ic_fpga_cycles_total_get(ctx, req.slot, (uint64_t *)val);
            val_len = (uint32_t)sizeof(uint64_t);
            break;
        case REMOTE_FUNC_PIN_RD:
            res = crosstalk_eda_ic_fpga_pin_rd(ctx, req.slot, key.name, (uint64_t *)val);
            val_len = (uint32_t)sizeof(uint64_t);
            break;
        case REMOTE_FUNC_MEM_OFFSET_GET:
            res = crosstalk_eda_ic_fpga_mem_offset_get(ctx, req.slot, key.name, (uint64_t *)val);
            val_len = (uint32_t)sizeof(uint64_t);
            break;
        case REMOTE_FUNC_IO_RD:
            res = crosstalk_eda_ic_fpga_io_rd(ctx, req.slot, key.arg0, (uint64_t *)val);
            val_len = (uint32_t)sizeof(uint64_t);
            break;
        case REMOTE_FUNC_SLOT_HLT_IS:
            val[0] = crosstalk_eda_ic_fpga_slot_hlt_is(ctx, req.slot);
            res = LDG_ERR_AOK;
            val_len = (uint32_t)sizeof(uint8_t);
            break;
        case REMOTE_FUNC_SLOT_LIVE_IS:
            val[0] = crosstalk_eda_ic_fpga_slot_live_is(ctx, req.slot);
            res = LDG_ERR_AOK;
            val_len = (uint32_t)sizeof(uint8_t);
            break;
        case REMOTE_FUNC_RUNNING_IS:
            val[0] = crosstalk_eda_ic_fpga_running_is(ctx);
            res = LDG_ERR_AOK;
            val_len = (uint32_t)sizeof(uint8_t);
            break;
        case REMOTE_FUNC_RUNNING_TOP_GET:
        {
            const char *top = crosstalk_eda_ic_fpga_running_top_get(ctx);
            uint64_t top_len = 0;

            if (top == 0x0) { top = ""; }

            if (ldg_str_len_get(top, (uint64_t)REMOTE_TOP_CAP - REMOTE_STEP, &top_len) != LDG_ERR_AOK) { top_len = 0; }

            memcpy(val, top, top_len);
            val[top_len] = LDG_STR_TERM;
            val_len = (uint32_t)top_len + REMOTE_STEP;
            res = LDG_ERR_AOK;
        }
        break;
        case REMOTE_FUNC_VERDICT_GET:
            res = crosstalk_eda_ic_fpga_verdict_get(ctx, req.slot, (crosstalk_eda_ic_fpga_verdict_t *)val);
            val_len = (uint32_t)sizeof(crosstalk_eda_ic_fpga_verdict_t);
            break;
        case REMOTE_FUNC_SIG_PEEK:
        case REMOTE_FUNC_MEM_WIDTH_GET:
        {
            remote_val_width_t vw = LDG_STRUCT_ZERO_INIT;

            if (req.func == (uint32_t)REMOTE_FUNC_SIG_PEEK) { res = crosstalk_eda_ic_fpga_sig_peek(ctx, req.slot, key.name, &vw.val, &vw.width); }
            else { res = crosstalk_eda_ic_fpga_mem_width_get(ctx, req.slot, key.name, &vw.val, &vw.width); }

            memcpy(val, &vw, sizeof(vw));
            val_len = (uint32_t)sizeof(vw);
        }
        break;
        case REMOTE_FUNC_AUDIO_RATE_GET:
            res = crosstalk_eda_ic_fpga_audio_rate_get(ctx, req.slot, (uint32_t *)val);
            val_len = (uint32_t)sizeof(uint32_t);
            break;
        case REMOTE_FUNC_MEM_WATCH_GET:
        {
            const crosstalk_eda_ic_fpga_mem_watch_t *watch = 0x0;

            res = crosstalk_eda_ic_fpga_mem_watch_get(ctx, (uint32_t)key.arg0, &watch);
            if (res == LDG_ERR_AOK && watch != 0x0) { memcpy(val, watch, sizeof(*watch)); }
            else { memset(val, 0x0, sizeof(*watch)); }

            val_len = (uint32_t)sizeof(*watch);
        }
        break;
        case REMOTE_FUNC_IO_MAP_GET:
        {
            remote_io_map_t io = LDG_STRUCT_ZERO_INIT;
            const char *name = 0x0;

            res = crosstalk_eda_ic_fpga_io_map_get(ctx, req.slot, (uint32_t)key.arg0, &io.base, &io.span, &name);

            if (res == LDG_ERR_AOK && name != 0x0)
            {
                uint32_t cpy = ldg_str_rbrcpy(io.name, name, (uint64_t)sizeof(io.name));

                if (cpy != LDG_ERR_AOK && cpy != LDG_ERR_STR_TRUNC) { io.name[0] = LDG_STR_TERM; }
            }

            memcpy(val, &io, sizeof(io));
            val_len = (uint32_t)sizeof(io);
        }
        break;
        case REMOTE_FUNC_DDU_FRAME_GET:
        case REMOTE_FUNC_DDU_TEXT_GET:
            res = remote_serve_bytes(ctx, srv, req.func, req.slot, &key, val, req.val_cap, &val_len);
            break;
        case REMOTE_FUNC_IR_GET:
            res = remote_serve_ir(ctx, srv, req.slot, &key, val, req.val_cap, &val_len);
            break;
        case REMOTE_FUNC_RUN_RATE_GET:
        case REMOTE_FUNC_RUN_PIN_GET:
        case REMOTE_FUNC_RUN_CYCLES_GET:
            if (srv->run == 0x0) { res = LDG_ERR_NOT_INIT; }
            else if (req.func == (uint32_t)REMOTE_FUNC_RUN_RATE_GET) { res = crosstalk_eda_ic_fpga_run_rate_get(srv->run, (uint64_t *)val); }
            else if (req.func == (uint32_t)REMOTE_FUNC_RUN_PIN_GET) { res = crosstalk_eda_ic_fpga_run_pin_get(srv->run, (uint64_t *)val); }
            else { res = crosstalk_eda_ic_fpga_run_cycles_get(srv->run, (uint64_t *)val); }

            val_len = (uint32_t)sizeof(uint64_t);
            break;
        case REMOTE_FUNC_RUN_LIVE_IS:
            val[0] = srv->run == 0x0 ? LDG_TRUTH_FALSE : crosstalk_eda_ic_fpga_run_live_is(srv->run);
            res = LDG_ERR_AOK;
            val_len = (uint32_t)sizeof(uint8_t);
            break;
        case REMOTE_FUNC_MEM_RD:
        case REMOTE_FUNC_MEM_BYTE_RD:
        case REMOTE_FUNC_DSU_GET:
        case REMOTE_FUNC_EMU_STATE_GET:
        case REMOTE_FUNC_MEM_ACC_GET:
        case REMOTE_FUNC_FAULT_GET:
        case REMOTE_FUNC_RING_GET:
        case REMOTE_FUNC_PROF_GET:
            res = remote_serve_words(ctx, req.func, req.slot, &key, val, req.val_cap, &val_len);
            break;
        default:
            return LDG_ERR_UNSUPPORTED;
    }

    if (val_len > req.val_cap) { val_len = req.val_cap; }

    ret = crosstalk_eda_ic_fpga_remote_resp_encode(frame_out, (uint32_t)REMOTE_SRV_OUT_CAP, &req, key_raw, res, val, val_len, out_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_frame = frame_out;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_eda_ic_fpga_remote_apply(crosstalk_eda_ic_fpga_ctx_t *ctx, const uint8_t *payload, uint32_t len)
{
    crosstalk_eda_ic_fpga_remote_wr_hdr_t hdr = LDG_STRUCT_ZERO_INIT;
    remote_key_t key = LDG_STRUCT_ZERO_INIT;
    const uint8_t *args = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || payload == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_eda_ic_fpga_remote_wr_decode(payload, len, &hdr, &args);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (hdr.args_len < (uint32_t)sizeof(key)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memcpy(&key, args, sizeof(key));
    key.name[sizeof(key.name) - REMOTE_STEP] = LDG_STR_TERM;

    switch (hdr.func)
    {
        case REMOTE_FUNC_SLOT_HLT_SET:
            return crosstalk_eda_ic_fpga_slot_hlt_set(ctx, hdr.slot, (uint8_t)key.arg0);
        case REMOTE_FUNC_SLOT_GATE_SET:
            return crosstalk_eda_ic_fpga_slot_gate_set(ctx, hdr.slot, (uint8_t)key.arg0);
        case REMOTE_FUNC_CLK_DOMAIN_SET:
            return crosstalk_eda_ic_fpga_clk_domain_set(ctx, hdr.slot, (uint32_t)key.arg0);
        case REMOTE_FUNC_PIN_WR:
            return crosstalk_eda_ic_fpga_pin_wr(ctx, hdr.slot, key.name, key.arg0);
        case REMOTE_FUNC_SIG_POKE:
            return crosstalk_eda_ic_fpga_sig_poke(ctx, hdr.slot, key.name, key.arg0, (uint8_t)key.arg1);
        case REMOTE_FUNC_PROF_EN_SET:
            return crosstalk_eda_ic_fpga_prof_en_set(ctx, hdr.slot, (uint8_t)key.arg0);
        case REMOTE_FUNC_PROF_EN:
            return crosstalk_eda_ic_fpga_prof_en(ctx, hdr.slot, (uint8_t)key.arg0);
        case REMOTE_FUNC_MEM_WATCH_CLEAR:
            return crosstalk_eda_ic_fpga_mem_watch_clear(ctx, hdr.slot);
        case REMOTE_FUNC_BRK_SET:
            return crosstalk_eda_ic_fpga_brk_set(ctx, hdr.slot, key.arg0, (uint8_t)key.arg1);
        case REMOTE_FUNC_IO_WR:
            return crosstalk_eda_ic_fpga_io_wr(ctx, hdr.slot, key.arg0, key.arg1);
        case REMOTE_FUNC_WATCH_SET:
            return crosstalk_eda_ic_fpga_watch_set(ctx, hdr.slot, key.arg0, key.arg1, (uint8_t)key.arg2);
        case REMOTE_FUNC_BRK_SKIP_SET:
            return crosstalk_eda_ic_fpga_brk_skip_set(ctx, hdr.slot, key.arg0);
        case REMOTE_FUNC_BRK_COND_SET:
            return crosstalk_eda_ic_fpga_brk_cond_set(ctx, hdr.slot, (uint8_t)key.arg2, key.arg0);
        case REMOTE_FUNC_EMU_STEP_SET:
            return crosstalk_eda_ic_fpga_emu_step_set(ctx, key.arg0);
        case REMOTE_FUNC_HLT_MASK_SET:
            return crosstalk_eda_ic_fpga_hlt_mask_set(ctx, hdr.slot, (uint32_t)key.arg0);
        case REMOTE_FUNC_MEM_WR:
        {
            uint64_t vals[REMOTE_WR_WORDS_MAX] = LDG_ARR_ZERO_INIT;
            uint32_t cunt = key.arg2;
            uint32_t wrote = 0;

            if (cunt > REMOTE_WR_WORDS_MAX || (uint64_t)hdr.args_len < (uint64_t)sizeof(key) + (uint64_t)cunt * sizeof(uint64_t)) { return LDG_ERR_BOUNDS; }

            memcpy(vals, args + sizeof(key), (uint64_t)cunt * sizeof(uint64_t));

            return crosstalk_eda_ic_fpga_mem_wr(ctx, hdr.slot, key.name, key.arg0, vals, cunt, &wrote);
        }
        case REMOTE_FUNC_RUN_START:
        case REMOTE_FUNC_RUN_HLT:
        case REMOTE_FUNC_RUN_PIN_SET:
        {
            const crosstalk_eda_ic_fpga_remote_t *srv = (const crosstalk_eda_ic_fpga_remote_t *)ctx->remote;

            if (srv == 0x0 || srv->run == 0x0) { return LDG_ERR_NOT_INIT; }

            if (hdr.func == (uint32_t)REMOTE_FUNC_RUN_START) { return crosstalk_eda_ic_fpga_run_start(srv->run); }

            if (hdr.func == (uint32_t)REMOTE_FUNC_RUN_HLT) { return crosstalk_eda_ic_fpga_run_hlt(srv->run); }

            return crosstalk_eda_ic_fpga_run_pin_set(srv->run, key.arg0);
        }
        default:
            return LDG_ERR_UNSUPPORTED;
    }
}
