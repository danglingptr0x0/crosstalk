#ifndef CROSSTALK_MNGR_MNGR_H
#define CROSSTALK_MNGR_MNGR_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>

#include "mngr/conf.h"

#define CROSSTALK_MNGR_MACHINE_MAX 64
#define CROSSTALK_MNGR_EMU_EXE "dangcsaemu.exe"
#define CROSSTALK_MNGR_CMD_CAP 8192

typedef struct crosstalk_mngr_ctx
{
    crosstalk_mngr_machine_t machine[CROSSTALK_MNGR_MACHINE_MAX];
    char path[CROSSTALK_MNGR_MACHINE_MAX][LDG_IO_PATH_MAX];
    char dir[LDG_IO_PATH_MAX];
    char exe_dir[LDG_IO_PATH_MAX];
    uint32_t cunt;
    uint32_t sel;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_mngr_ctx_t;

uint32_t crosstalk_mngr_init(crosstalk_mngr_ctx_t *ctx);
uint32_t crosstalk_mngr_scan(crosstalk_mngr_ctx_t *ctx);
uint32_t crosstalk_mngr_machine_add(crosstalk_mngr_ctx_t *ctx, const char *name);
uint32_t crosstalk_mngr_machine_save(crosstalk_mngr_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_mngr_machine_drop(crosstalk_mngr_ctx_t *ctx, uint32_t idx);
uint32_t crosstalk_mngr_launch(const crosstalk_mngr_ctx_t *ctx, uint32_t idx);
uint8_t crosstalk_mngr_sel_ok_is(const crosstalk_mngr_ctx_t *ctx);

#endif
