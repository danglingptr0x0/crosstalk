#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "hdl/abi.h"

static uint32_t abi_sym_bind(const crosstalk_hdl_img_t *img, const char *name, const uint32_t *rva, uint32_t idx, void **out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(img == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rva != 0x0)
    {
        *out = (void *)(img->base + rva[idx]);

        return LDG_ERR_AOK;
    }

    ret = crosstalk_hdl_img_sym_get(img, name, out);
    if (ret != LDG_ERR_AOK)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "abi: symbol missing; name: %.63s", name);

        return CROSSTALK_HDL_ERR_ABI_SYM;
    }

    return LDG_ERR_AOK;
}

static void abi_sym_opt(const crosstalk_hdl_img_t *img, const char *name, const uint32_t *rva, uint32_t idx, void **out)
{
    if (LDG_UNLIKELY(img == 0x0 || name == 0x0 || out == 0x0)) { return; }

    if (rva != 0x0)
    {
        *out = (void *)(img->base + rva[idx]);

        return;
    }

    if (crosstalk_hdl_img_sym_get(img, name, out) != LDG_ERR_AOK) { *out = 0x0; }
}

uint32_t crosstalk_hdl_abi_bind(const crosstalk_hdl_img_t *img, const uint32_t *rva, uint32_t rva_cunt, crosstalk_hdl_abi_t *out)
{
    uint32_t rev = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(img == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (rva != 0x0 && rva_cunt < (uint32_t)CROSSTALK_HDL_ABI_RVA_CUNT)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "abi: rva table too small; given: %u32; wanted: %u32", rva_cunt, (uint32_t)CROSSTALK_HDL_ABI_RVA_CUNT);

        return CROSSTALK_HDL_ERR_ABI_SYM;
    }

    memset(out, 0x0, sizeof(*out));

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_REV, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_REV, (void **)&out->rev);
    if (ret != LDG_ERR_AOK) { return ret; }

    rev = out->rev();
    if (rev != (uint32_t)CROSSTALK_HDL_ABI_REV)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "abi: revision mismatch; binary: %u32; game: %u32", rev, (uint32_t)CROSSTALK_HDL_ABI_REV);

        return CROSSTALK_HDL_ERR_ABI_REV;
    }

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_INIT, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_INIT, (void **)&out->init);
    if (ret != LDG_ERR_AOK) { return ret; }

    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_CYCLE, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_CYCLE, (void **)&out->cycle);

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_SIG_FIND, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_SIG_FIND, (void **)&out->sig_find);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_POKE, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_POKE, (void **)&out->poke);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_PEEK, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_PEEK, (void **)&out->peek);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_MEM_ST, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_MEM_ST, (void **)&out->mem_st);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_MEM_LD, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_MEM_LD, (void **)&out->mem_ld);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = abi_sym_bind(img, CROSSTALK_HDL_ABI_SYM_STATE, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_STATE_GET, (void **)&out->state_get);
    if (ret != LDG_ERR_AOK) { return ret; }

    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_TOUCH, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_TOUCH, (void **)&out->touch);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_MEM_INIT, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_MEM_INIT, (void **)&out->mem_init);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_N_CYCLE, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_N_CYCLE, (void **)&out->n_cycle);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_SERIAL_DRAIN, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_SERIAL_DRAIN, (void **)&out->serial_drain);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_HLT_MASK, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_HLT_MASK_SET, (void **)&out->hlt_mask_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_SET, (void **)&out->disk_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_DRIVE_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_DRIVE_SET, (void **)&out->disk_drive_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_BLK_WIDTH, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_BLK_WIDTH_GET, (void **)&out->disk_blk_width_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_IS, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_IS, (void **)&out->disk_dirty_is);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_LO, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_LO_GET, (void **)&out->disk_dirty_lo_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_HI, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_HI_GET, (void **)&out->disk_dirty_hi_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_CLEAR, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_CLEAR, (void **)&out->disk_dirty_clear);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_AUDIO_DRAIN, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_AUDIO_DRAIN, (void **)&out->audio_drain);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_AUDIO_RATE, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_AUDIO_RATE_GET, (void **)&out->audio_rate_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_PROF_EN, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_PROF_EN, (void **)&out->prof_en);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_BRK_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_BRK_SET, (void **)&out->brk_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_BRK_SKIP_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_BRK_SKIP_SET, (void **)&out->brk_skip_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_BRK_COND_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_BRK_COND_SET, (void **)&out->brk_cond_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_TIME_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_TIME_SET, (void **)&out->time_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_WATCH_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_WATCH_SET, (void **)&out->watch_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_IO_RD, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_IO_RD, (void **)&out->io_rd);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_IO_MAP, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_IO_MAP_GET, (void **)&out->io_map_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_IO_WR, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_IO_WR, (void **)&out->io_wr);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_RELAY_REQ_TAKE, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_RELAY_REQ_TAKE, (void **)&out->relay_req_take);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_RELAY_REPLY_PUSH, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_RELAY_REPLY_PUSH, (void **)&out->relay_reply_push);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_RELAY_PAYLOAD_CAP, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_RELAY_PAYLOAD_CAP_GET, (void **)&out->relay_payload_cap_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_EPOCH_SET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_EPOCH_SET, (void **)&out->epoch_set);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DDU_FRAME_GET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DDU_FRAME_GET, (void **)&out->ddu_frame_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_DDU_TEXT_GET, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_DDU_TEXT_GET, (void **)&out->ddu_text_get);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_NET_DRAIN, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_NET_DRAIN, (void **)&out->net_drain);
    abi_sym_opt(img, CROSSTALK_HDL_ABI_SYM_NET_FEED, rva, (uint32_t)CROSSTALK_HDL_ABI_RVA_NET_FEED, (void **)&out->net_feed);

    if (out->cycle == 0x0 && out->n_cycle == 0x0)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "abi: no stepping export; wanted: %s or %s", CROSSTALK_HDL_ABI_SYM_CYCLE, CROSSTALK_HDL_ABI_SYM_N_CYCLE);

        return CROSSTALK_HDL_ERR_ABI_SYM;
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "abi: bound; rva: %u32; rev: %u32; step: %u32; init mem: %u32; batch: %u32; serial drain: %u32", rva != 0x0 ? 0x1 : 0x0, rev, out->cycle != 0x0 ? 0x1 : 0x0, out->mem_init != 0x0 ? 0x1 : 0x0, out->n_cycle != 0x0 ? 0x1 : 0x0, out->serial_drain != 0x0 ? 0x1 : 0x0);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "abi: bound io; hlt mask: %u32; disk: %u32; disk drives: %u32; audio: %u32; net drain: %u32; net feed: %u32; host time: %u32; watch: %u32; io peek: %u32; io poke: %u32", out->hlt_mask_set != 0x0 ? 0x1 : 0x0, out->disk_set != 0x0 ? 0x1 : 0x0, out->disk_drive_set != 0x0 ? 0x1 : 0x0, out->audio_drain != 0x0 ? 0x1 : 0x0, out->net_drain != 0x0 ? 0x1 : 0x0, out->net_feed != 0x0 ? 0x1 : 0x0, out->time_set != 0x0 ? 0x1 : 0x0, out->watch_set != 0x0 ? 0x1 : 0x0, out->io_rd != 0x0 ? 0x1 : 0x0, out->io_wr != 0x0 ? 0x1 : 0x0);
    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "abi: bound relay; req take: %u32; reply push: %u32; payload cap: %u32", out->relay_req_take != 0x0 ? 0x1 : 0x0, out->relay_reply_push != 0x0 ? 0x1 : 0x0, out->relay_payload_cap_get != 0x0 ? 0x1 : 0x0);

    out->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
