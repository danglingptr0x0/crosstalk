#ifndef CROSSTALK_NET_CDN_H
#define CROSSTALK_NET_CDN_H

#include <stdint.h>
#include <inttypes.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/io/file.h>
#include <dangling/thread/pool.h>

#define CROSSTALK_NET_CDN_SLOT_CAP 8
#define CROSSTALK_NET_CDN_URL_CAP 1024
#define CROSSTALK_NET_CDN_BYTE_MAX (8 * LDG_MIB)
#define CROSSTALK_NET_CDN_POOL_WORKERS 2
#define CROSSTALK_NET_CDN_CACHE_MODE 0644
#define CROSSTALK_NET_CDN_NAME_FMT "%08" PRIX32 ".img"
#define CROSSTALK_NET_CDN_SCHEME_HTTPS "https://"
#define CROSSTALK_NET_CDN_SCHEME_HTTP "http://"
#define CROSSTALK_NET_CDN_EXT_PNG ".png"
#define CROSSTALK_NET_CDN_EXT_JPG ".jpg"
#define CROSSTALK_NET_CDN_EXT_JPEG ".jpeg"
#define CROSSTALK_NET_CDN_EXT_WEBP ".webp"
#define CROSSTALK_NET_CDN_STATE_IDLE 0
#define CROSSTALK_NET_CDN_STATE_BUSY 1
#define CROSSTALK_NET_CDN_STATE_DONE 2
#define CROSSTALK_NET_CDN_STATE_FAIL 3

typedef struct crosstalk_net_cdn_slot
{
    ldg_io_file_t file;
    char url[CROSSTALK_NET_CDN_URL_CAP];
    char path[LDG_IO_PATH_MAX];
    uint64_t bytes;
    uint32_t state;
    uint32_t last_err;
    uint8_t live;
    uint8_t file_ok;
    uint8_t pudding[6];
} crosstalk_net_cdn_slot_t;

typedef struct crosstalk_net_cdn_ctx
{
    ldg_thread_pool_t pool;
    crosstalk_net_cdn_slot_t slot[CROSSTALK_NET_CDN_SLOT_CAP];
    char cache_dir[LDG_IO_PATH_MAX];
    uint8_t pool_ok;
    uint8_t pudding[7];
} crosstalk_net_cdn_ctx_t;

uint8_t crosstalk_net_cdn_url_find(const char *text, char *out, uint32_t out_cap);
uint32_t crosstalk_net_cdn_init(crosstalk_net_cdn_ctx_t *ctx, const char *cache_dir);
uint32_t crosstalk_net_cdn_req(crosstalk_net_cdn_ctx_t *ctx, const char *url, uint32_t *out_slot);
uint32_t crosstalk_net_cdn_state_get(const crosstalk_net_cdn_ctx_t *ctx, uint32_t slot, uint32_t *out_state);
uint32_t crosstalk_net_cdn_path_get(const crosstalk_net_cdn_ctx_t *ctx, uint32_t slot, const char **out_path);
uint32_t crosstalk_net_cdn_shutdown(crosstalk_net_cdn_ctx_t *ctx);

#endif
