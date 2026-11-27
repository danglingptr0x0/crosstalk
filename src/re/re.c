#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>
#include <dangling/str/str.h>

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

    if (crosstalk_hdl_elf_shutdown(&ctx->cand_elf) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cand elf shutdown failed"); }

    if (crosstalk_re_cc_shutdown(&ctx->cc) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re cc shutdown failed"); }

    if (crosstalk_re_diff_shutdown(&ctx->diff) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re diff shutdown failed"); }

    if (crosstalk_re_db_shutdown(&ctx->db) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re db shutdown failed"); }

    if (crosstalk_re_img_shutdown(&ctx->img) != LDG_ERR_AOK) { ldg_logging_ln_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "re img shutdown failed"); }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_db_path_build(const char *img_path, char *out, uint32_t cap)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(img_path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (img_path[len] != LDG_STR_TERM) { len++; }

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
        if (path[at] == '/' || path[at] == '\\') { start = at + 1; }

        at++;
    }

    len = at - start;

    if (len + LDG_STR_TERM_SIZE > cap) { return LDG_ERR_STR_TRUNC; }

    memcpy(out, path + start, len);
    out[len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_stat_init(crosstalk_re_stat_t *stat)
{
    if (LDG_UNLIKELY(stat == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(stat, 0x0, sizeof(*stat));

    stat->curr = UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_stat_step(crosstalk_re_stat_t *stat, uint8_t live_is, uint64_t now)
{
    uint64_t delta = 0;

    if (LDG_UNLIKELY(stat == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (live_is != LDG_TRUTH_TRUE)
    {
        stat->live_prev = LDG_TRUTH_FALSE;
        stat->step_at = 0;

        return LDG_ERR_AOK;
    }

    if (stat->live_prev != LDG_TRUTH_TRUE)
    {
        stat->live_prev = LDG_TRUTH_TRUE;
        stat->step_at = now;
        stat->sesh_cunt++;

        if (stat->first_at == 0) { stat->first_at = now; }

        return LDG_ERR_AOK;
    }

    if (now <= stat->step_at) { return LDG_ERR_AOK; }

    delta = now - stat->step_at;
    stat->step_at = now;

    if (delta > (uint64_t)CROSSTALK_RE_STAT_GAP_MAX) { return LDG_ERR_AOK; }

    stat->attach_time = stat->attach_time + delta;

    if (stat->curr < stat->exe_cunt) { stat->exe[stat->curr].attach_time = stat->exe[stat->curr].attach_time + delta; }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_stat_exe_close(crosstalk_re_stat_t *stat)
{
    if (LDG_UNLIKELY(stat == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    stat->curr = UINT32_MAX;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_stat_exe_open(crosstalk_re_stat_t *stat, const char *name, uint64_t now)
{
    uint64_t least = UINT64_MAX;
    uint32_t slot = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(stat == 0x0 || name == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    stat->curr = UINT32_MAX;

    if (name[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (uint32_t at = 0; at < stat->exe_cunt && slot == UINT32_MAX; at++) { if (strncmp(stat->exe[at].name, name, sizeof(stat->exe[at].name)) == 0) { slot = at; } }

    if (slot == UINT32_MAX)
    {
        if (stat->exe_cunt < (uint32_t)CROSSTALK_RE_STAT_EXE_MAX)
        {
            slot = stat->exe_cunt;
            stat->exe_cunt++;
        }
        else { for (uint32_t at = 0; at < stat->exe_cunt; at++)
            {
                if (stat->exe[at].attach_time > least) { continue; }

                least = stat->exe[at].attach_time;
                slot = at;
            }
        }

        if (slot == UINT32_MAX) { return LDG_ERR_FULL; }

        memset(&stat->exe[slot], 0x0, sizeof(stat->exe[slot]));

        ret = ldg_str_rbrcpy(stat->exe[slot].name, name, (uint64_t)sizeof(stat->exe[slot].name));
        if (ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC) { return ret; }
    }

    stat->exe[slot].attaches++;
    stat->exe[slot].last_at = now;
    stat->curr = slot;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_stat_serialize(const crosstalk_re_stat_t *stat, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t totals_bytes = (uint32_t)CROSSTALK_RE_STAT_TOTALS_WORDS * (uint32_t)sizeof(uint64_t);
    uint32_t exes_bytes = 0;
    uint32_t hdr[CROSSTALK_RE_STAT_HDR_BYTES / sizeof(uint32_t)] = LDG_ARR_ZERO_INIT;
    uint32_t blk[CROSSTALK_RE_STAT_BLK_HDR_BYTES / sizeof(uint32_t)] = LDG_ARR_ZERO_INIT;
    uint32_t need = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(stat == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    exes_bytes = (uint32_t)sizeof(stat->exe_cunt) + stat->exe_cunt * (uint32_t)CROSSTALK_RE_STAT_EXE_BYTES;
    need = (uint32_t)CROSSTALK_RE_STAT_HDR_BYTES + (uint32_t)CROSSTALK_RE_STAT_BLK_CUNT * (uint32_t)CROSSTALK_RE_STAT_BLK_HDR_BYTES + totals_bytes + exes_bytes;

    *out_len = need;

    if (out == 0x0 || cap == 0) { return LDG_ERR_AOK; }

    if (cap < need) { return LDG_ERR_BOUNDS; }

    hdr[CROSSTALK_RE_STAT_HDR_MAGIC] = (uint32_t)CROSSTALK_RE_STAT_MAGIC;
    hdr[CROSSTALK_RE_STAT_HDR_REV] = (uint32_t)CROSSTALK_RE_STAT_REV;
    hdr[CROSSTALK_RE_STAT_HDR_BLK_CUNT] = (uint32_t)CROSSTALK_RE_STAT_BLK_CUNT;
    memcpy(out + pos, hdr, sizeof(hdr));
    pos = pos + (uint32_t)sizeof(hdr);

    blk[CROSSTALK_RE_STAT_BLK_TAG] = (uint32_t)CROSSTALK_RE_STAT_BLK_TOTALS;
    blk[CROSSTALK_RE_STAT_BLK_SPAN] = totals_bytes;
    memcpy(out + pos, blk, sizeof(blk));
    pos = pos + (uint32_t)sizeof(blk);

    memcpy(out + pos, &stat->attach_time, sizeof(stat->attach_time));
    pos = pos + (uint32_t)sizeof(stat->attach_time);
    memcpy(out + pos, &stat->sesh_cunt, sizeof(stat->sesh_cunt));
    pos = pos + (uint32_t)sizeof(stat->sesh_cunt);
    memcpy(out + pos, &stat->first_at, sizeof(stat->first_at));
    pos = pos + (uint32_t)sizeof(stat->first_at);

    blk[CROSSTALK_RE_STAT_BLK_TAG] = (uint32_t)CROSSTALK_RE_STAT_BLK_EXES;
    blk[CROSSTALK_RE_STAT_BLK_SPAN] = exes_bytes;
    memcpy(out + pos, blk, sizeof(blk));
    pos = pos + (uint32_t)sizeof(blk);

    memcpy(out + pos, &stat->exe_cunt, sizeof(stat->exe_cunt));
    pos = pos + (uint32_t)sizeof(stat->exe_cunt);

    for (uint32_t at = 0; at < stat->exe_cunt; at++)
    {
        memcpy(out + pos, stat->exe[at].name, sizeof(stat->exe[at].name));
        pos = pos + (uint32_t)sizeof(stat->exe[at].name);
        memcpy(out + pos, &stat->exe[at].attach_time, sizeof(stat->exe[at].attach_time));
        pos = pos + (uint32_t)sizeof(stat->exe[at].attach_time);
        memcpy(out + pos, &stat->exe[at].attaches, sizeof(stat->exe[at].attaches));
        pos = pos + (uint32_t)sizeof(stat->exe[at].attaches);
        memcpy(out + pos, &stat->exe[at].last_at, sizeof(stat->exe[at].last_at));
        pos = pos + (uint32_t)sizeof(stat->exe[at].last_at);
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_stat_deserialize(crosstalk_re_stat_t *stat, const uint8_t *buff, uint32_t len)
{
    uint32_t hdr[CROSSTALK_RE_STAT_HDR_BYTES / sizeof(uint32_t)] = LDG_ARR_ZERO_INIT;
    uint32_t blk[CROSSTALK_RE_STAT_BLK_HDR_BYTES / sizeof(uint32_t)] = LDG_ARR_ZERO_INIT;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(stat == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len < (uint32_t)CROSSTALK_RE_STAT_HDR_BYTES) { return LDG_ERR_BOUNDS; }

    memcpy(hdr, buff, sizeof(hdr));

    if (hdr[CROSSTALK_RE_STAT_HDR_MAGIC] != (uint32_t)CROSSTALK_RE_STAT_MAGIC) { return LDG_ERR_IO_FMT; }

    if (hdr[CROSSTALK_RE_STAT_HDR_REV] != (uint32_t)CROSSTALK_RE_STAT_REV) { return LDG_ERR_UNSUPPORTED; }

    ret = crosstalk_re_stat_init(stat);
    if (ret != LDG_ERR_AOK) { return ret; }

    pos = (uint32_t)sizeof(hdr);

    for (uint32_t blk_at = 0; blk_at < hdr[CROSSTALK_RE_STAT_HDR_BLK_CUNT]; blk_at++)
    {
        uint32_t at_pos = 0;

        if (len - pos < (uint32_t)sizeof(blk)) { break; }

        memcpy(blk, buff + pos, sizeof(blk));
        pos = pos + (uint32_t)sizeof(blk);

        if (blk[CROSSTALK_RE_STAT_BLK_SPAN] > len - pos) { break; }

        at_pos = pos;

        if (blk[CROSSTALK_RE_STAT_BLK_TAG] == (uint32_t)CROSSTALK_RE_STAT_BLK_TOTALS && blk[CROSSTALK_RE_STAT_BLK_SPAN] >= (uint32_t)CROSSTALK_RE_STAT_TOTALS_WORDS * (uint32_t)sizeof(uint64_t))
        {
            memcpy(&stat->attach_time, buff + at_pos, sizeof(stat->attach_time));
            at_pos = at_pos + (uint32_t)sizeof(stat->attach_time);
            memcpy(&stat->sesh_cunt, buff + at_pos, sizeof(stat->sesh_cunt));
            at_pos = at_pos + (uint32_t)sizeof(stat->sesh_cunt);
            memcpy(&stat->first_at, buff + at_pos, sizeof(stat->first_at));
        }
        else if (blk[CROSSTALK_RE_STAT_BLK_TAG] == (uint32_t)CROSSTALK_RE_STAT_BLK_EXES && blk[CROSSTALK_RE_STAT_BLK_SPAN] >= (uint32_t)sizeof(stat->exe_cunt))
        {
            uint32_t cunt = 0;

            memcpy(&cunt, buff + at_pos, sizeof(cunt));
            at_pos = at_pos + (uint32_t)sizeof(cunt);

            if (cunt > (uint32_t)CROSSTALK_RE_STAT_EXE_MAX) { cunt = (uint32_t)CROSSTALK_RE_STAT_EXE_MAX; }

            while (stat->exe_cunt < cunt && at_pos + (uint32_t)CROSSTALK_RE_STAT_EXE_BYTES <= pos + blk[CROSSTALK_RE_STAT_BLK_SPAN])
            {
                crosstalk_re_stat_exe_t *exe = &stat->exe[stat->exe_cunt];

                memcpy(exe->name, buff + at_pos, sizeof(exe->name));
                exe->name[CROSSTALK_RE_STAT_NAME_CAP - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
                at_pos = at_pos + (uint32_t)sizeof(exe->name);
                memcpy(&exe->attach_time, buff + at_pos, sizeof(exe->attach_time));
                at_pos = at_pos + (uint32_t)sizeof(exe->attach_time);
                memcpy(&exe->attaches, buff + at_pos, sizeof(exe->attaches));
                at_pos = at_pos + (uint32_t)sizeof(exe->attaches);
                memcpy(&exe->last_at, buff + at_pos, sizeof(exe->last_at));
                at_pos = at_pos + (uint32_t)sizeof(exe->last_at);

                stat->exe_cunt++;
            }
        }

        pos = pos + blk[CROSSTALK_RE_STAT_BLK_SPAN];
    }

    return LDG_ERR_AOK;
}
