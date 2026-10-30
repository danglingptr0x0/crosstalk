#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include "core/log.h"
#include "re/re.h"

uint32_t crosstalk_re_init(crosstalk_re_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    ret = crosstalk_re_img_init(&ctx->img);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_db_init(&ctx->db);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_diff_init(&ctx->diff);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_re_cc_init(&ctx->cc);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = crosstalk_hdl_elf_init(&ctx->cand_elf);
    if (ret != LDG_ERR_AOK) { return ret; }

    ctx->db_gen_seen = UINT32_MAX;
    ctx->ok = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_shutdown(crosstalk_re_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (crosstalk_hdl_elf_shutdown(&ctx->cand_elf) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cand elf shutdown failed"); }

    if (crosstalk_re_cc_shutdown(&ctx->cc) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc shutdown failed"); }

    if (crosstalk_re_diff_shutdown(&ctx->diff) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff shutdown failed"); }

    if (crosstalk_re_db_shutdown(&ctx->db) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db shutdown failed"); }

    if (crosstalk_re_img_shutdown(&ctx->img) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img shutdown failed"); }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_path_build(const char *img_path, char *out, uint32_t cap)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(img_path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (img_path[len] != LDG_STR_TERM) { len = len + 0x1; }

    if ((uint64_t)len + sizeof(CROSSTALK_RE_DB_SIBLING_EXT) > (uint64_t)cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(out, img_path, len);
    memcpy(out + len, CROSSTALK_RE_DB_SIBLING_EXT, sizeof(CROSSTALK_RE_DB_SIBLING_EXT));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_leaf_get(const char *path, char *out, uint32_t cap)
{
    uint32_t at = 0;
    uint32_t start = 0;
    uint32_t len = 0;

    if (LDG_UNLIKELY(path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (path[at] != LDG_STR_TERM)
    {
        if (path[at] == '/' || path[at] == '\\') { start = at + 0x1; }

        at = at + 0x1;
    }

    len = at - start;

    if (len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(out, path + start, len);
    out[len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}
