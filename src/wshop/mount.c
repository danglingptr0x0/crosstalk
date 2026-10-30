#include <inttypes.h>
#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "wshop/mount.h"

#define MOUNT_STEP 1

uint32_t crosstalk_wshop_mount_core(crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const crosstalk_wshop_item_t *item, const crosstalk_eda_ic_fpga_comp_t *caps)
{
    uint32_t bytes = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(fpga == 0x0 || item == 0x0 || caps == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (item->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (item->manifest.kind != CROSSTALK_WSHOP_MANIFEST_KIND_CORE) { return CROSSTALK_WSHOP_MOUNT_ERR_KIND; }

    ret = crosstalk_eda_ic_fpga_fileset_program(fpga, slot_idx, item->dir, item->manifest.files, item->manifest.top, caps, item->manifest.clk, &bytes);

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "wshop mount: core; id: %u64; slot: %u32; top: %.63s; isa: %.31s; bytes: %u32; err: %u32", item->file_id, slot_idx, item->manifest.top, item->manifest.isa, bytes, ret);

    return ret;
}

uint32_t crosstalk_wshop_mount_img(crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const crosstalk_wshop_item_t *core, const crosstalk_wshop_item_t *img)
{
    char path[CROSSTALK_WSHOP_ITEM_PATH_CAP] = LDG_ARR_ZERO_INIT;
    const crosstalk_wshop_manifest_img_t *ent = 0x0;
    uint64_t words = 0;
    uint64_t bytes = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(fpga == 0x0 || core == 0x0 || img == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (core->ok != LDG_TRUTH_TRUE || img->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (core->manifest.kind != CROSSTALK_WSHOP_MANIFEST_KIND_CORE) { return CROSSTALK_WSHOP_MOUNT_ERR_KIND; }

    if (img->manifest.kind != CROSSTALK_WSHOP_MANIFEST_KIND_SOFTWARE) { return CROSSTALK_WSHOP_MOUNT_ERR_KIND; }

    if (img->manifest.tgt_core != 0 && img->manifest.tgt_core != core->file_id)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop mount: img pinned elsewhere; img: %u64; wants: %u64; got: %u64", img->file_id, img->manifest.tgt_core, core->file_id);
        return CROSSTALK_WSHOP_MOUNT_ERR_COMPAT;
    }

    if (crosstalk_wshop_manifest_isa_compat_is(&core->manifest, &img->manifest) != LDG_TRUTH_TRUE)
    {
        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "wshop mount: isa mismatch; core: %.31s; img wants: %.31s", core->manifest.isa, img->manifest.tgt_isa);
        return CROSSTALK_WSHOP_MOUNT_ERR_COMPAT;
    }

    for (idx = 0; idx < img->manifest.img_cunt; idx = idx + MOUNT_STEP)
    {
        ent = &img->manifest.img[idx];

        ret = ldg_io_path_join(img->dir, ent->path, path, (uint64_t)sizeof(path));
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = crosstalk_eda_ic_fpga_mem_ld(fpga, slot_idx, ent->mem, path, ent->word_off, &words, &bytes);

        ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, ret == LDG_ERR_AOK ? LDG_LOGGING_LVL_INFO : LDG_LOGGING_LVL_WARN, "wshop mount: img; id: %u64; slot: %u32; mem: %.63s; off: %u64; words: %u64; err: %u32", img->file_id, slot_idx, ent->mem, ent->word_off, words, ret);

        if (ret != LDG_ERR_AOK) { return CROSSTALK_WSHOP_MOUNT_ERR_IMG; }
    }

    return LDG_ERR_AOK;
}
