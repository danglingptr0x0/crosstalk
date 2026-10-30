#ifndef CROSSTALK_WSHOP_PUBLISH_H
#define CROSSTALK_WSHOP_PUBLISH_H

#include <stdint.h>

#include "wshop/manifest.h"

#define CROSSTALK_WSHOP_PUBLISH_ERR_WR 986
#define CROSSTALK_WSHOP_PUBLISH_ERR_EMPTY 987
#define CROSSTALK_WSHOP_PUBLISH_ERR_STAGE 988

#define CROSSTALK_WSHOP_PUBLISH_STAGE_SUBDIR "wshop_stage"
#define CROSSTALK_WSHOP_PUBLISH_SRC_EXT ".sv"
#define CROSSTALK_WSHOP_PUBLISH_PREVIEW_NAME "preview.png"
#define CROSSTALK_WSHOP_PUBLISH_DIR_MODE 0755

#define CROSSTALK_WSHOP_PUBLISH_EXT_SV ".sv"
#define CROSSTALK_WSHOP_PUBLISH_CORE_NAME "core.xtcore"
#define CROSSTALK_WSHOP_PUBLISH_IMG_NAME "image.xtimg"
#define CROSSTALK_WSHOP_PUBLISH_LINE_CAP 512

uint32_t crosstalk_wshop_publish_fileset_scan(const char *dir, char *out_files, uint64_t out_cap, uint32_t *out_cunt);
uint32_t crosstalk_wshop_publish_stage(const char *stage_root, const char *top, const char *src, uint32_t src_len, crosstalk_wshop_manifest_t *manifest, char *out_dir, uint64_t out_cap);
uint32_t crosstalk_wshop_publish_manifest_wr(const char *dir, const crosstalk_wshop_manifest_t *manifest);

#endif
