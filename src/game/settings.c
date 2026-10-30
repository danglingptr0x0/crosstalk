#include "game/settings.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define SETTINGS_DOUBLE_CUNT 40
#define SETTINGS_UINT32_CUNT 8
#define SETTINGS_TOGGLE_CUNT 22
#define SETTINGS_BLOB_LEN (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT)
#define SETTINGS_DOUBLE_CUNT_V1 38
#define SETTINGS_TOGGLE_CUNT_V1 11
#define SETTINGS_BLOB_LEN_V1 (SETTINGS_DOUBLE_CUNT_V1 * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V1)
#define SETTINGS_TOGGLE_CUNT_V2 16
#define SETTINGS_BLOB_LEN_V2 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V2)
#define SETTINGS_TOGGLE_CUNT_V3 18
#define SETTINGS_BLOB_LEN_V3 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V3)
#define SETTINGS_TOGGLE_CUNT_V4 19
#define SETTINGS_BLOB_LEN_V4 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V4)
#define SETTINGS_TOGGLE_CUNT_V5 20
#define SETTINGS_BLOB_LEN_V5 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V5)
#define SETTINGS_TOGGLE_CUNT_V6 21
#define SETTINGS_BLOB_LEN_V6 (SETTINGS_DOUBLE_CUNT * 8 + SETTINGS_UINT32_CUNT * 4 + SETTINGS_TOGGLE_CUNT_V6)
#define SETTINGS_QUANT_SCALE 10000.0
#define LAST_STATE_DOUBLE_CUNT 20
#define LAST_STATE_BLOB_LEN (LAST_STATE_DOUBLE_CUNT * 8)

static void buff_wr(uint8_t *out, uint32_t *offset, const void *src, uint32_t len)
{
    memcpy(out + *offset, src, len);
    *offset = *offset + len;
}

static void buff_rd(const uint8_t *buff, uint32_t *offset, void *dst, uint32_t len)
{
    memcpy(dst, buff + *offset, len);
    *offset = *offset + len;
}

static const double* settings_double_at_get(const crosstalk_game_settings_t *settings, uint32_t idx)
{
    return &settings->tonemap_exposure + idx;
}

static const uint32_t* settings_uint32_at_get(const crosstalk_game_settings_t *settings, uint32_t idx)
{
    return &settings->res_w + idx;
}

static const uint8_t* settings_toggle_at_get(const crosstalk_game_settings_t *settings, uint32_t idx)
{
    return &settings->bloom_on + idx;
}

static double* settings_double_at_wr_get(crosstalk_game_settings_t *settings, uint32_t idx)
{
    return &settings->tonemap_exposure + idx;
}

static uint32_t* settings_uint32_at_wr_get(crosstalk_game_settings_t *settings, uint32_t idx)
{
    return &settings->res_w + idx;
}

static uint8_t* settings_toggle_at_wr_get(crosstalk_game_settings_t *settings, uint32_t idx)
{
    return &settings->bloom_on + idx;
}

uint8_t crosstalk_game_settings_fab_unlocked_is(const crosstalk_game_settings_t *settings)
{
    if (LDG_UNLIKELY(settings == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (settings->fab_safety_off != LDG_TRUTH_FALSE && settings->fab_interlock_off != LDG_TRUTH_FALSE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_game_settings_quantize(crosstalk_game_settings_t *settings)
{
    double *slot = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(settings == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1)
    {
        slot = settings_double_at_wr_get(settings, idx);
        *slot = round(*slot * SETTINGS_QUANT_SCALE) / SETTINGS_QUANT_SCALE;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_serialize(const crosstalk_game_settings_t *settings, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(settings == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = SETTINGS_BLOB_LEN;
    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < SETTINGS_BLOB_LEN) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_wr(out, &offset, settings_double_at_get(settings, idx), 8); }

    for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_wr(out, &offset, settings_uint32_at_get(settings, idx), 4); }

    for (idx = 0; idx < SETTINGS_TOGGLE_CUNT; idx = idx + 1) { buff_wr(out, &offset, settings_toggle_at_get(settings, idx), 1); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_settings_deserialize(crosstalk_game_settings_t *settings, const uint8_t *buff, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(settings == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == SETTINGS_BLOB_LEN_V1)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT_V1; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V1; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V2)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V2; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V3)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V3; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V4)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V4; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V5)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V5; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len == SETTINGS_BLOB_LEN_V6)
    {
        for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

        for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

        for (idx = 0; idx < SETTINGS_TOGGLE_CUNT_V6; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

        return LDG_ERR_AOK;
    }

    if (len != SETTINGS_BLOB_LEN) { return LDG_ERR_IO_FMT; }

    for (idx = 0; idx < SETTINGS_DOUBLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_double_at_wr_get(settings, idx), 8); }

    for (idx = 0; idx < SETTINGS_UINT32_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_uint32_at_wr_get(settings, idx), 4); }

    for (idx = 0; idx < SETTINGS_TOGGLE_CUNT; idx = idx + 1) { buff_rd(buff, &offset, settings_toggle_at_wr_get(settings, idx), 1); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_last_state_serialize(const crosstalk_game_last_state_t *state, uint8_t *out, uint32_t cap, uint32_t *out_len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(state == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = LAST_STATE_BLOB_LEN;
    if (out == 0x0) { return LDG_ERR_AOK; }

    if (cap < LAST_STATE_BLOB_LEN) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < LAST_STATE_DOUBLE_CUNT; idx = idx + 1) { buff_wr(out, &offset, &state->cam_hdg + idx, 8); }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_game_last_state_deserialize(crosstalk_game_last_state_t *state, const uint8_t *buff, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t idx = 0;
    uint32_t avail = 0;

    if (LDG_UNLIKELY(state == 0x0 || buff == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    avail = len / 8;
    if (avail > LAST_STATE_DOUBLE_CUNT) { avail = LAST_STATE_DOUBLE_CUNT; }

    for (idx = 0; idx < avail; idx = idx + 1) { buff_rd(buff, &offset, &state->cam_hdg + idx, 8); }

    return LDG_ERR_AOK;
}
