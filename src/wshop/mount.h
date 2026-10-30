#ifndef CROSSTALK_WSHOP_MOUNT_H
#define CROSSTALK_WSHOP_MOUNT_H

#include <stdint.h>

#include "eda/ic/fpga/fpga.h"
#include "wshop/item.h"

#define CROSSTALK_WSHOP_MOUNT_ERR_KIND 983
#define CROSSTALK_WSHOP_MOUNT_ERR_COMPAT 984
#define CROSSTALK_WSHOP_MOUNT_ERR_IMG 985

uint32_t crosstalk_wshop_mount_core(crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const crosstalk_wshop_item_t *item, const crosstalk_eda_ic_fpga_comp_t *caps);
uint32_t crosstalk_wshop_mount_img(crosstalk_eda_ic_fpga_ctx_t *fpga, uint32_t slot_idx, const crosstalk_wshop_item_t *core, const crosstalk_wshop_item_t *img);

#endif
