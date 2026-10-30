#include <stdint.h>
#include <string.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/logging/logging.h>

#include <dangling/io/file.h>
#include <dangling/str/str.h>

#include "core/log.h"
#include "game/caption.h"

static uint8_t caption_arrow_find(const char *src, uint32_t hd, uint32_t tail)
{
    uint32_t idx = hd;

    while (idx + CROSSTALK_GAME_CAPTION_ARROW_LEN <= tail)
    {
        if (src[idx] == '-' && src[idx + CROSSTALK_GAME_CAPTION_STEP] == '-' && src[idx + CROSSTALK_GAME_CAPTION_STEP + CROSSTALK_GAME_CAPTION_STEP] == '>') { return LDG_TRUTH_TRUE; }

        idx = idx + CROSSTALK_GAME_CAPTION_STEP;
    }

    return LDG_TRUTH_FALSE;
}

static uint8_t caption_digit_take(const char *src, uint32_t len, uint32_t *pos, uint32_t *out)
{
    uint32_t val = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t seen = CROSSTALK_GAME_CAPTION_NONE;

    while (*pos < len && ldg_char_digit_is((uint8_t)src[*pos]) == LDG_TRUTH_TRUE)
    {
        val = val * CROSSTALK_GAME_CAPTION_TS_RADIX + CROSSTALK_GAME_CAPTION_DIGIT_VAL(src[*pos]);
        *pos = *pos + CROSSTALK_GAME_CAPTION_STEP;
        seen = seen + CROSSTALK_GAME_CAPTION_STEP;
    }

    *out = val;
    if (seen == CROSSTALK_GAME_CAPTION_NONE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t caption_ts_parse(const char *src, uint32_t len, uint32_t *pos, double *out_secs)
{
    uint32_t lhs = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t mid = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t rhs = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t ms = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t hours = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t mins = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t secs = CROSSTALK_GAME_CAPTION_NONE;

    while (*pos < len && (src[*pos] == ' ' || src[*pos] == '\t')) { *pos = *pos + CROSSTALK_GAME_CAPTION_STEP; }

    if (caption_digit_take(src, len, pos, &lhs) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (*pos >= len || src[*pos] != ':') { return LDG_TRUTH_FALSE; }

    *pos = *pos + CROSSTALK_GAME_CAPTION_STEP;
    if (caption_digit_take(src, len, pos, &mid) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (*pos < len && src[*pos] == ':')
    {
        *pos = *pos + CROSSTALK_GAME_CAPTION_STEP;
        if (caption_digit_take(src, len, pos, &rhs) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        hours = lhs;
        mins = mid;
        secs = rhs;
    }
    else
    {
        mins = lhs;
        secs = mid;
    }

    if (*pos < len && (src[*pos] == '.' || src[*pos] == ','))
    {
        *pos = *pos + CROSSTALK_GAME_CAPTION_STEP;
        if (caption_digit_take(src, len, pos, &ms) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }
    }

    *out_secs = (double)hours * (double)LDG_SECS_PER_HOUR + (double)mins * (double)LDG_SECS_PER_MIN + (double)secs + (double)ms / CROSSTALK_GAME_CAPTION_MS_PER_SEC;
    return LDG_TRUTH_TRUE;
}

static uint64_t caption_ns_get(double secs)
{
    if (secs <= 0.0) { return (uint64_t)CROSSTALK_GAME_CAPTION_NONE; }

    return (uint64_t)(secs * (double)LDG_NS_PER_SEC);
}

static uint32_t caption_sound_push(crosstalk_game_caption_ctx_t *ctx, const char *text)
{
    ldg_term_caption_cue_desc_t cue = LDG_STRUCT_ZERO_INIT;
    uint64_t id = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || text == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    cue.text = text;
    cue.text_len = (uint64_t)strlen(text);
    cue.spkr = 0x0;
    cue.spkr_len = (uint64_t)CROSSTALK_GAME_CAPTION_NONE;
    cue.start = caption_ns_get(ctx->clock);
    cue.end = caption_ns_get(ctx->clock + CROSSTALK_GAME_CAPTION_SOUND_SECS);
    cue.kind = (uint32_t)LDG_TERM_CAPTION_KIND_SOUND;
    cue.pos_en = LDG_TRUTH_FALSE;
    cue.color_en = LDG_TRUTH_FALSE;

    ret = ldg_term_caption_cue_add(ctx->eng, &cue, &id);
    if (ret == LDG_ERR_FULL)
    {
        ret = ldg_term_caption_clear(ctx->eng);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = ldg_term_caption_cue_add(ctx->eng, &cue, &id);
    }

    return ret;
}

static void caption_spkr_take(crosstalk_game_caption_ctx_t *ctx, const char *src, uint32_t hd, uint32_t tail)
{
    uint32_t out = CROSSTALK_GAME_CAPTION_NONE;

    while (hd < tail && src[hd] != ' ' && src[hd] != '.') { hd = hd + CROSSTALK_GAME_CAPTION_STEP; }

    while (hd < tail && (src[hd] == ' ' || src[hd] == '.')) { hd = hd + CROSSTALK_GAME_CAPTION_STEP; }

    while (hd < tail && out < CROSSTALK_GAME_CAPTION_SPKR_CAP - LDG_STR_TERM_SIZE)
    {
        if ((uint8_t)src[hd] < LDG_ASCII_PRINTABLE_MIN || (uint8_t)src[hd] > LDG_ASCII_PRINTABLE_MAX) { hd = hd + CROSSTALK_GAME_CAPTION_STEP; continue; }

        ctx->cue_spkr[out] = src[hd];
        out = out + CROSSTALK_GAME_CAPTION_STEP;
        hd = hd + CROSSTALK_GAME_CAPTION_STEP;
    }

    ctx->cue_spkr[out] = LDG_STR_TERM;
    if (out != CROSSTALK_GAME_CAPTION_NONE) { ctx->spkr_seen = LDG_TRUTH_TRUE; }
}

static void caption_note_spkr_take(crosstalk_game_caption_ctx_t *ctx, const char *src, uint32_t hd, uint32_t tail)
{
    const char *tag = CROSSTALK_GAME_CAPTION_NOTE_SPKR;
    uint32_t idx = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t out = CROSSTALK_GAME_CAPTION_NONE;

    if (hd + CROSSTALK_GAME_CAPTION_NOTE_SPKR_LEN > tail) { return; }

    while (idx < CROSSTALK_GAME_CAPTION_NOTE_SPKR_LEN)
    {
        if (src[hd + idx] != tag[idx]) { return; }

        idx = idx + CROSSTALK_GAME_CAPTION_STEP;
    }

    hd = hd + CROSSTALK_GAME_CAPTION_NOTE_SPKR_LEN;
    while (hd < tail && ldg_char_space_is((uint8_t)src[hd]) == LDG_TRUTH_TRUE) { hd = hd + CROSSTALK_GAME_CAPTION_STEP; }

    while (tail > hd && ldg_char_space_is((uint8_t)src[tail - CROSSTALK_GAME_CAPTION_STEP]) == LDG_TRUTH_TRUE) { tail = tail - CROSSTALK_GAME_CAPTION_STEP; }

    while (hd < tail && out < CROSSTALK_GAME_CAPTION_SPKR_CAP - LDG_STR_TERM_SIZE)
    {
        if ((uint8_t)src[hd] < LDG_ASCII_PRINTABLE_MIN || (uint8_t)src[hd] > LDG_ASCII_PRINTABLE_MAX) { hd = hd + CROSSTALK_GAME_CAPTION_STEP; continue; }

        ctx->dflt_spkr[out] = src[hd];
        out = out + CROSSTALK_GAME_CAPTION_STEP;
        hd = hd + CROSSTALK_GAME_CAPTION_STEP;
    }

    ctx->dflt_spkr[out] = LDG_STR_TERM;
    if (out != CROSSTALK_GAME_CAPTION_NONE) { ctx->spkr_seen = LDG_TRUTH_TRUE; }
}

static void caption_line_append(crosstalk_game_caption_ctx_t *ctx, const char *src, uint32_t hd, uint32_t tail, uint32_t *text_len)
{
    if (*text_len != CROSSTALK_GAME_CAPTION_NONE && *text_len < LDG_TERM_CAPTION_TEXT_CAP - LDG_STR_TERM_SIZE)
    {
        ctx->cue_text[*text_len] = ' ';
        *text_len = *text_len + CROSSTALK_GAME_CAPTION_STEP;
    }

    while (hd < tail && *text_len < LDG_TERM_CAPTION_TEXT_CAP - LDG_STR_TERM_SIZE)
    {
        if (src[hd] == '<')
        {
            uint32_t tag_hd = hd + CROSSTALK_GAME_CAPTION_STEP;
            uint32_t tag_tail = tag_hd;

            while (tag_tail < tail && src[tag_tail] != '>') { tag_tail = tag_tail + CROSSTALK_GAME_CAPTION_STEP; }

            if (tag_hd < tag_tail && src[tag_hd] == 'v') { caption_spkr_take(ctx, src, tag_hd, tag_tail); }

            hd = (tag_tail < tail) ? tag_tail + CROSSTALK_GAME_CAPTION_STEP : tail;
            continue;
        }

        if ((uint8_t)src[hd] < LDG_ASCII_PRINTABLE_MIN || (uint8_t)src[hd] > LDG_ASCII_PRINTABLE_MAX) { hd = hd + CROSSTALK_GAME_CAPTION_STEP; continue; }

        ctx->cue_text[*text_len] = src[hd];
        *text_len = *text_len + CROSSTALK_GAME_CAPTION_STEP;
        hd = hd + CROSSTALK_GAME_CAPTION_STEP;
    }

    ctx->cue_text[*text_len] = LDG_STR_TERM;
}

uint32_t crosstalk_game_caption_vtt_drop(crosstalk_game_caption_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->vtt_cunt = CROSSTALK_GAME_CAPTION_NONE;
    ctx->spkr_seen = LDG_TRUTH_FALSE;
    ctx->dflt_spkr[0] = LDG_STR_TERM;
    return ldg_term_caption_clear(ctx->eng);
}

uint32_t crosstalk_game_caption_vtt_ld(crosstalk_game_caption_ctx_t *ctx, const char *path)
{
    ldg_term_caption_style_t style = LDG_STRUCT_ZERO_INIT;
    ldg_term_caption_cue_desc_t cue = LDG_STRUCT_ZERO_INIT;
    ldg_io_info_t st = LDG_STRUCT_ZERO_INIT;
    uint64_t id = 0;
    ldg_io_file_t *file = 0x0;
    uint64_t bytes_rd = 0;
    uint32_t len = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t pos = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t text_len = CROSSTALK_GAME_CAPTION_NONE;
    uint32_t ret = 0;
    double secs_in = 0.0;
    double secs_out = 0.0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ret = crosstalk_game_caption_vtt_drop(ctx);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_path_info_get(path, &st);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (st.size == 0) { return LDG_ERR_EMPTY; }

    len = (st.size >= (uint64_t)CROSSTALK_GAME_CAPTION_RAW_CAP) ? (uint32_t)(CROSSTALK_GAME_CAPTION_RAW_CAP - LDG_STR_TERM_SIZE) : (uint32_t)st.size;

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, CROSSTALK_GAME_CAPTION_OPEN_MODE, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_file_rd(file, (ldg_byte_t *)ctx->raw, (uint64_t)len, &bytes_rd);
    if (ret == LDG_ERR_AOK) { ret = ldg_io_file_close(file); }
    else { ldg_io_file_close(file); }

    if (ret != LDG_ERR_AOK) { return ret; }

    len = (uint32_t)bytes_rd;
    ctx->raw[len] = LDG_STR_TERM;

    while (pos < len)
    {
        uint32_t line_hd = pos;
        uint32_t line_tail = pos;
        uint32_t ts_pos = CROSSTALK_GAME_CAPTION_NONE;

        while (line_tail < len && ctx->raw[line_tail] != '\n') { line_tail = line_tail + CROSSTALK_GAME_CAPTION_STEP; }

        pos = (line_tail < len) ? line_tail + CROSSTALK_GAME_CAPTION_STEP : len;

        if (caption_arrow_find(ctx->raw, line_hd, line_tail) != LDG_TRUTH_TRUE)
        {
            caption_note_spkr_take(ctx, ctx->raw, line_hd, line_tail);
            continue;
        }

        ts_pos = line_hd;
        if (caption_ts_parse(ctx->raw, line_tail, &ts_pos, &secs_in) != LDG_TRUTH_TRUE) { continue; }

        while (ts_pos < line_tail && ctx->raw[ts_pos] != '>') { ts_pos = ts_pos + CROSSTALK_GAME_CAPTION_STEP; }

        if (ts_pos >= line_tail) { continue; }

        ts_pos = ts_pos + CROSSTALK_GAME_CAPTION_STEP;
        if (caption_ts_parse(ctx->raw, line_tail, &ts_pos, &secs_out) != LDG_TRUTH_TRUE) { continue; }

        text_len = CROSSTALK_GAME_CAPTION_NONE;
        ctx->cue_text[0] = LDG_STR_TERM;
        ctx->cue_spkr[0] = LDG_STR_TERM;

        while (pos < len)
        {
            uint32_t txt_hd = pos;
            uint32_t txt_tail = pos;

            while (txt_tail < len && ctx->raw[txt_tail] != '\n') { txt_tail = txt_tail + CROSSTALK_GAME_CAPTION_STEP; }

            pos = (txt_tail < len) ? txt_tail + CROSSTALK_GAME_CAPTION_STEP : len;

            while (txt_tail > txt_hd && ctx->raw[txt_tail - CROSSTALK_GAME_CAPTION_STEP] == '\r') { txt_tail = txt_tail - CROSSTALK_GAME_CAPTION_STEP; }

            if (txt_tail == txt_hd) { break; }

            caption_line_append(ctx, ctx->raw, txt_hd, txt_tail, &text_len);
        }

        if (text_len == CROSSTALK_GAME_CAPTION_NONE) { continue; }

        if (LDG_UNLIKELY(text_len >= LDG_TERM_CAPTION_TEXT_CAP - LDG_STR_TERM_SIZE)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption: vtt cue exceeds the text cap and is truncated; split the cue"); }

        cue.text = ctx->cue_text;
        cue.text_len = (uint64_t)text_len;
        if (ctx->cue_spkr[0] == LDG_STR_TERM && ctx->dflt_spkr[0] != LDG_STR_TERM) { cue.spkr = ctx->dflt_spkr; }
        else { cue.spkr = (ctx->cue_spkr[0] != LDG_STR_TERM) ? ctx->cue_spkr : 0x0; }

        cue.spkr_len = cue.spkr != 0x0 ? (uint64_t)strlen(cue.spkr) : (uint64_t)CROSSTALK_GAME_CAPTION_NONE;
        cue.start = caption_ns_get(secs_in);
        cue.end = caption_ns_get(secs_out);
        cue.kind = (uint32_t)LDG_TERM_CAPTION_KIND_SPEECH;
        cue.pos_en = LDG_TRUTH_FALSE;
        cue.color_en = LDG_TRUTH_FALSE;

        if (LDG_UNLIKELY(ctx->vtt_cunt >= LDG_TERM_CAPTION_CUE_MAX))
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption: vtt exceeds the cue pool; trailing cues dropped");
            break;
        }

        ret = ldg_term_caption_cue_add(ctx->eng, &cue, &id);
        if (LDG_UNLIKELY(ret == LDG_ERR_STR_TRUNC))
        {
            ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption: vtt cue rejected; text exceeds the cue cap; split the cue");
            continue;
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_FULL)) { break; }

        ctx->vtt_cunt = ctx->vtt_cunt + CROSSTALK_GAME_CAPTION_STEP;
    }

    if (ldg_term_caption_style_get(ctx->eng, &style) == LDG_ERR_AOK)
    {
        style.spkr_en = ctx->spkr_seen;
        ret = ldg_term_caption_style_set(ctx->eng, &style);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_FULL)) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption style set failed"); }
    }

    ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_INFO, "caption: vtt loaded; path: %.255s; cues: %u32", path, ctx->vtt_cunt);
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_caption_init(crosstalk_game_caption_ctx_t *ctx)
{
    ldg_term_caption_style_t style = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = (crosstalk_game_caption_ctx_t)LDG_STRUCT_ZERO_INIT;

    ret = ldg_term_caption_create(&ctx->eng);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_caption_style_default_get(&style);
    if (ret == LDG_ERR_AOK)
    {
        style.line_shown[LDG_TERM_CAPTION_KIND_SPEECH] = CROSSTALK_GAME_CAPTION_SPEECH_LINE_SHOWN;
        style.line_shown[LDG_TERM_CAPTION_KIND_SOUND] = CROSSTALK_GAME_CAPTION_SOUND_LINE_SHOWN;
        style.line_frac[LDG_TERM_CAPTION_KIND_SPEECH] = CROSSTALK_GAME_CAPTION_SPEECH_LINE_FRAC;
        style.line_frac[LDG_TERM_CAPTION_KIND_SOUND] = CROSSTALK_GAME_CAPTION_SOUND_LINE_FRAC;
        style.margin_frac = CROSSTALK_GAME_CAPTION_MARGIN_FRAC;
        style.anchor = (uint8_t)LDG_TERM_CAPTION_ANCHOR_BOTTOM;
        style.spkr_color.red = (uint8_t)CROSSTALK_GAME_CAPTION_SPKR_RED;
        style.spkr_color.green = (uint8_t)CROSSTALK_GAME_CAPTION_SPKR_GREEN;
        style.spkr_color.blue = (uint8_t)CROSSTALK_GAME_CAPTION_SPKR_BLUE;
        style.spkr_en = LDG_TRUTH_FALSE;
        style.bearing_en = LDG_TRUTH_FALSE;
        ret = ldg_term_caption_style_set(ctx->eng, &style);
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_caption_mask_set(ctx->eng, LDG_TERM_CAPTION_MASK_ALL); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (ldg_term_caption_destroy(ctx->eng) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "caption destroy failed"); }

        ctx->eng = 0x0;
        return ret;
    }

    ctx->ok = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_caption_shutdown(crosstalk_game_caption_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok == LDG_TRUTH_TRUE) { ret = ldg_term_caption_destroy(ctx->eng); }

    *ctx = (crosstalk_game_caption_ctx_t)LDG_STRUCT_ZERO_INIT;
    return ret;
}

uint32_t crosstalk_game_caption_tick(crosstalk_game_caption_ctx_t *ctx, double clock_secs)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    ctx->clock = clock_secs;
    return ldg_term_caption_clock_set(ctx->eng, caption_ns_get(clock_secs));
}

uint32_t crosstalk_game_caption_draw(crosstalk_game_caption_ctx_t *ctx, ldg_term_render_t *render, const ldg_term_rect_t *rect)
{
    if (LDG_UNLIKELY(ctx == 0x0 || render == 0x0 || rect == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    return ldg_term_caption_draw(ctx->eng, render, rect);
}

uint32_t crosstalk_game_caption_sfx_cb(void *user, crosstalk_game_fx_sfx_kind_t sfx_kind)
{
    crosstalk_game_caption_ctx_t *ctx = (crosstalk_game_caption_ctx_t *)user;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->ok != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (sfx_kind == CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CW || sfx_kind == CROSSTALK_GAME_FX_SFX_KIND_ROTARY_CCW) { return caption_sound_push(ctx, CROSSTALK_GAME_CAPTION_ROTARY_TEXT); }

    if (sfx_kind == CROSSTALK_GAME_FX_SFX_KIND_LEVER) { return caption_sound_push(ctx, CROSSTALK_GAME_CAPTION_LEVER_TEXT); }

    return LDG_ERR_AOK;
}
