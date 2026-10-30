#ifndef CROSSTALK_GAME_BOARD_H
#define CROSSTALK_GAME_BOARD_H

#include <stdint.h>

#include <dangling/io/path.h>

#include "game/game.h"

#define CROSSTALK_GAME_BOARD_CAP 128
#define CROSSTALK_GAME_BOARD_SRC_CAP 72
#define CROSSTALK_GAME_BOARD_DIR_CAP 512
#define CROSSTALK_GAME_BOARD_FILE_CAP 128
#define CROSSTALK_GAME_BOARD_NAME_CAP 64
#define CROSSTALK_GAME_BOARD_EXT ".xtlk"
#define CROSSTALK_GAME_BOARD_EXT_BARE "xtlk"
#define CROSSTALK_GAME_BOARD_ROW_CAP 256
#define CROSSTALK_GAME_BOARD_ITEMS_CAP 24576
#define CROSSTALK_GAME_BOARD_NAMES_CAP 8448
#define CROSSTALK_GAME_BOARD_WSHOP_MARK "[W] "
#define CROSSTALK_GAME_BOARD_LOCK_MARK "[BOARD] "
#define CROSSTALK_GAME_BOARD_COL_GAP " " " "
#define CROSSTALK_GAME_BOARD_ROW_FMT "%.1s%.15s%.39s%.63s" CROSSTALK_GAME_BOARD_COL_GAP "%.31s" CROSSTALK_GAME_BOARD_COL_GAP "%.63s" CROSSTALK_GAME_BOARD_COL_GAP "%.47s"
#define CROSSTALK_GAME_BOARD_EDITORS_CAP 48
#define CROSSTALK_GAME_BOARD_TAG_CAP 32
#define CROSSTALK_GAME_BOARD_MARK_CAP 40
#define CROSSTALK_GAME_BOARD_NAME_DEFAULT "board"
#define CROSSTALK_GAME_BOARD_NAME_ILLEGAL "/\\:;*?\"<>|"
#define CROSSTALK_GAME_BOARD_NAME_SUB '_'
#define CROSSTALK_GAME_BOARD_SHOT_EXT ".png"
#define CROSSTALK_GAME_BOARD_SIZE_CAP 16
#define CROSSTALK_GAME_BOARD_TIME_CAP 32
#define CROSSTALK_GAME_BOARD_TIME_FMT "%Y-%m-%dT%H:%M:%S%z"

typedef enum crosstalk_game_board_src
{
    CROSSTALK_GAME_BOARD_SRC_LOCAL = 0,
    CROSSTALK_GAME_BOARD_SRC_WSHOP
} crosstalk_game_board_src_t;

typedef struct crosstalk_game_board
{
    uint64_t epoch_sec;
    uint64_t size_bytes;
    uint64_t created_epoch_ms;
    crosstalk_game_board_src_t src;
    uint8_t author_ok;
    uint8_t epoch_from_name;
    uint8_t dir_idx;
    uint8_t editor_is;
    uint8_t board_pudding;
    char file[CROSSTALK_GAME_BOARD_FILE_CAP];
    char name[CROSSTALK_GAME_BOARD_NAME_CAP];
    char author[CROSSTALK_GAME_AUTHOR_NAME_CAP];
    char tag[CROSSTALK_GAME_BOARD_TAG_CAP];
    char editors[CROSSTALK_GAME_BOARD_EDITORS_CAP];
} crosstalk_game_board_t;

typedef struct crosstalk_game_board_ctx
{
    crosstalk_game_board_t entry[CROSSTALK_GAME_BOARD_CAP];
    char src_dir[CROSSTALK_GAME_BOARD_SRC_CAP][CROSSTALK_GAME_BOARD_DIR_CAP];
    char items[CROSSTALK_GAME_BOARD_ITEMS_CAP];
    char names[CROSSTALK_GAME_BOARD_NAMES_CAP];
    uint8_t lock[CROSSTALK_GAME_BOARD_CAP];
    uint32_t src_cunt;
    uint32_t cunt;
    uint8_t truncated;
    uint8_t pudding[3];
} crosstalk_game_board_ctx_t;

uint32_t crosstalk_game_board_init(crosstalk_game_board_ctx_t *ctx);
uint32_t crosstalk_game_board_scan(crosstalk_game_board_ctx_t *ctx, const char *dir, crosstalk_game_board_src_t src, const char *tag);
uint32_t crosstalk_game_board_sort(crosstalk_game_board_ctx_t *ctx);
uint32_t crosstalk_game_board_file_parse(const char *file, char *out_name, uint32_t out_name_cap, uint64_t *out_epoch_sec, uint8_t *out_epoch_from_name);
uint32_t crosstalk_game_board_path_build(const crosstalk_game_board_ctx_t *ctx, uint32_t idx, char *out, uint64_t out_size);
uint32_t crosstalk_game_board_remove(crosstalk_game_board_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_game_board_items_build(crosstalk_game_board_ctx_t *ctx);
uint32_t crosstalk_game_board_names_build(crosstalk_game_board_ctx_t *ctx);
uint32_t crosstalk_game_board_name_sanitize(const char *name, char *out, uint32_t out_cap);
uint32_t crosstalk_game_board_file_compose(const char *name, uint64_t epoch_sec, char *out, uint64_t out_size);
uint32_t crosstalk_game_board_ext_swap(const char *file, const char *ext, char *out, uint64_t out_size);
uint32_t crosstalk_game_board_size_fmt(uint64_t bytes, char *out, uint64_t out_size);

#endif
