#ifndef CROSSTALK_GAME_PACK_H
#define CROSSTALK_GAME_PACK_H

#include <stdint.h>

#define CROSSTALK_GAME_PACK_NAME_CAP 128
#define CROSSTALK_GAME_PACK_ENTRY_CAP 64

typedef struct crosstalk_game_pack_ctx
{
    uint8_t *buff;
    uint64_t cap;
    uint64_t len;
    uint32_t entry_cunt;
    uint8_t pudding[4];
} crosstalk_game_pack_ctx_t;

uint32_t crosstalk_game_pack_init(crosstalk_game_pack_ctx_t *ctx, uint8_t *buff, uint64_t cap);
uint32_t crosstalk_game_pack_blob_add(crosstalk_game_pack_ctx_t *ctx, const char *name, const uint8_t *data, uint64_t data_len);
uint32_t crosstalk_game_pack_file_add(crosstalk_game_pack_ctx_t *ctx, const char *name, const char *path);
uint32_t crosstalk_game_pack_seal(crosstalk_game_pack_ctx_t *ctx, uint8_t *out, uint64_t out_cap, uint64_t *out_len, uint32_t *out_checksum);
uint64_t crosstalk_game_pack_sealed_size_get(const crosstalk_game_pack_ctx_t *ctx);

#endif
