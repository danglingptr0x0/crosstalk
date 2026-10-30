#ifndef CROSSTALK_GAME_CAPTION_H
#define CROSSTALK_GAME_CAPTION_H

#include <stdint.h>

#include <dangling/term/caption.h>

#include "game/fx/sfx.h"

#define CROSSTALK_GAME_CAPTION_ROTARY_TEXT "[the click of a rotary]"
#define CROSSTALK_GAME_CAPTION_LEVER_TEXT "[the click of a lever switch]"
#define CROSSTALK_GAME_CAPTION_SOUND_SECS 1.25
#define CROSSTALK_GAME_CAPTION_SPEECH_LINE_FRAC 300U
#define CROSSTALK_GAME_CAPTION_SOUND_LINE_FRAC 880U
#define CROSSTALK_GAME_CAPTION_MARGIN_FRAC 30U
#define CROSSTALK_GAME_CAPTION_SPEECH_LINE_SHOWN 4
#define CROSSTALK_GAME_CAPTION_SOUND_LINE_SHOWN 2
#define CROSSTALK_GAME_CAPTION_VTT_EXT ".vtt"
#define CROSSTALK_GAME_CAPTION_RAW_CAP 131072
#define CROSSTALK_GAME_CAPTION_SPKR_CAP 32
#define CROSSTALK_GAME_CAPTION_SPKR_RED 0xBFU
#define CROSSTALK_GAME_CAPTION_SPKR_GREEN 0xFFU
#define CROSSTALK_GAME_CAPTION_SPKR_BLUE 0x00U
#define CROSSTALK_GAME_CAPTION_MS_PER_SEC 1000.0
#define CROSSTALK_GAME_CAPTION_STEP 1
#define CROSSTALK_GAME_CAPTION_TS_RADIX 10
#define CROSSTALK_GAME_CAPTION_OPEN_MODE 0
#define CROSSTALK_GAME_CAPTION_ARROW_LEN 3
#define CROSSTALK_GAME_CAPTION_NONE 0
#define CROSSTALK_GAME_CAPTION_NOTE_SPKR "NOTE speaker:"
#define CROSSTALK_GAME_CAPTION_NOTE_SPKR_LEN 13
#define CROSSTALK_GAME_CAPTION_DIGIT_VAL(character) ((uint32_t)((character) - '0'))

typedef struct crosstalk_game_caption_ctx
{
    ldg_term_caption_t *eng;
    char raw[CROSSTALK_GAME_CAPTION_RAW_CAP];
    char cue_text[LDG_TERM_CAPTION_TEXT_CAP];
    char cue_spkr[CROSSTALK_GAME_CAPTION_SPKR_CAP];
    char dflt_spkr[CROSSTALK_GAME_CAPTION_SPKR_CAP];
    double clock;
    uint32_t vtt_cunt;
    uint8_t ok;
    uint8_t spkr_seen;
    uint8_t pudding[2];
} crosstalk_game_caption_ctx_t;

uint32_t crosstalk_game_caption_init(crosstalk_game_caption_ctx_t *ctx);
uint32_t crosstalk_game_caption_shutdown(crosstalk_game_caption_ctx_t *ctx);
uint32_t crosstalk_game_caption_vtt_ld(crosstalk_game_caption_ctx_t *ctx, const char *path);
uint32_t crosstalk_game_caption_vtt_drop(crosstalk_game_caption_ctx_t *ctx);
uint32_t crosstalk_game_caption_tick(crosstalk_game_caption_ctx_t *ctx, double clock_secs);
uint32_t crosstalk_game_caption_draw(crosstalk_game_caption_ctx_t *ctx, ldg_term_render_t *render, const ldg_term_rect_t *rect);
uint32_t crosstalk_game_caption_sfx_cb(void *user, crosstalk_game_fx_sfx_kind_t sfx_kind);

#endif
