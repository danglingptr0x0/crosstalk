#ifndef CROSSTALK_NET_WSHOP_H
#define CROSSTALK_NET_WSHOP_H

#include <stdint.h>

#include <dangling/io/path.h>

#include <libresteam/backend/backend.h>
#include <libresteam/wshop/types.h>

#define CROSSTALK_NET_WSHOP_TYPE_BOARD 0U
#define CROSSTALK_NET_WSHOP_TYPE_CORE 1U
#define CROSSTALK_NET_WSHOP_TYPE_SOFTWARE 2U
#define CROSSTALK_NET_WSHOP_TYPE_CUNT 3U
#define CROSSTALK_NET_WSHOP_TYPE_NONE UINT32_MAX
#define CROSSTALK_NET_WSHOP_CUNT_NONE 0U

#define CROSSTALK_NET_WSHOP_TAG_BOARD "Board"
#define CROSSTALK_NET_WSHOP_MARKER_BOARD ".xtlk"
#define CROSSTALK_NET_WSHOP_TAG_CORE "Core"
#define CROSSTALK_NET_WSHOP_MARKER_CORE ".xtcore"
#define CROSSTALK_NET_WSHOP_TAG_SOFTWARE "Software"
#define CROSSTALK_NET_WSHOP_MARKER_SOFTWARE ".xtimg"

#define CROSSTALK_NET_WSHOP_ITEM_MAX 128U
#define CROSSTALK_NET_WSHOP_PATH_MAX LDG_IO_PATH_MAX
#define CROSSTALK_NET_WSHOP_TAG_CAP 16U
#define CROSSTALK_NET_WSHOP_DESC_CAP 512U

#define CROSSTALK_NET_WSHOP_UPLOAD_IDLE 0U
#define CROSSTALK_NET_WSHOP_UPLOAD_CREATING 1U
#define CROSSTALK_NET_WSHOP_UPLOAD_UPDATING 2U
#define CROSSTALK_NET_WSHOP_UPLOAD_DONE 3U
#define CROSSTALK_NET_WSHOP_UPLOAD_ERR 4U

#define CROSSTALK_NET_WSHOP_CB_ITEM_CREATED 3403U
#define CROSSTALK_NET_WSHOP_CB_ITEM_SUBMITTED 3404U
#define CROSSTALK_NET_WSHOP_RES_CAP 32U
#define CROSSTALK_NET_WSHOP_RES_ERES_OFF 0U
#define CROSSTALK_NET_WSHOP_RES_CREATED_FILE_ID_OFF_PACK4 4U
#define CROSSTALK_NET_WSHOP_RES_CREATED_FILE_ID_OFF_PACK8 8U
#define CROSSTALK_NET_WSHOP_ERES_OK 1U
#define CROSSTALK_NET_WSHOP_FILE_TYPE_COMMUNITY 0U
#define CROSSTALK_NET_WSHOP_CALL_NONE 0U

typedef struct crosstalk_net_wshop_item
{
    char path[CROSSTALK_NET_WSHOP_PATH_MAX];
    char tag[CROSSTALK_NET_WSHOP_TAG_CAP];
    uint64_t file_id;
    uint32_t state;
    uint32_t type;
    uint8_t installed;
    uint8_t pudding[7];
} crosstalk_net_wshop_item_t;

typedef struct crosstalk_net_wshop_upload
{
    char title[LIBRESTEAM_WSHOP_TITLE_MAX];
    char desc[CROSSTALK_NET_WSHOP_DESC_CAP];
    char content_path[CROSSTALK_NET_WSHOP_PATH_MAX];
    char preview_path[CROSSTALK_NET_WSHOP_PATH_MAX];
    uint64_t call;
    uint64_t file_id;
    uint32_t state;
    uint32_t type;
} crosstalk_net_wshop_upload_t;

typedef struct crosstalk_net_wshop_ctx
{
    crosstalk_net_wshop_item_t item[CROSSTALK_NET_WSHOP_ITEM_MAX];
    uint32_t type_idx[CROSSTALK_NET_WSHOP_TYPE_CUNT][CROSSTALK_NET_WSHOP_ITEM_MAX];
    uint32_t type_cunt[CROSSTALK_NET_WSHOP_TYPE_CUNT];
    const libresteam_backend_set_t *set;
    uint32_t item_cunt;
    uint8_t init_ok;
    uint8_t pudding[3];
} crosstalk_net_wshop_ctx_t;

uint32_t crosstalk_net_wshop_init(crosstalk_net_wshop_ctx_t *ctx, const libresteam_backend_set_t *set);
void crosstalk_net_wshop_shutdown(crosstalk_net_wshop_ctx_t *ctx);
uint32_t crosstalk_net_wshop_refresh(crosstalk_net_wshop_ctx_t *ctx);
void crosstalk_net_wshop_poll(crosstalk_net_wshop_ctx_t *ctx);
uint32_t crosstalk_net_wshop_item_cunt_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type);
const crosstalk_net_wshop_item_t* crosstalk_net_wshop_item_get(const crosstalk_net_wshop_ctx_t *ctx, uint32_t type, uint32_t idx);
uint32_t crosstalk_net_wshop_item_pull(const crosstalk_net_wshop_ctx_t *ctx, const crosstalk_net_wshop_item_t *item);
uint8_t crosstalk_net_wshop_item_rdy_is(const crosstalk_net_wshop_item_t *item);
uint32_t crosstalk_net_wshop_upload_start(crosstalk_net_wshop_ctx_t *ctx, crosstalk_net_wshop_upload_t *up, uint32_t type, const char *title, const char *desc, const char *content_path, const char *preview_path);
uint32_t crosstalk_net_wshop_upload_state_get(crosstalk_net_wshop_ctx_t *ctx, crosstalk_net_wshop_upload_t *up, uint32_t *out_state);

#endif
