#ifndef CROSSTALK_EDA_IC_FPGA_EMU_H
#define CROSSTALK_EDA_IC_FPGA_EMU_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/io/path.h>
#include <dangling/mem/alloc.h>

#include "hdl/abi.h"
#include "hdl/img.h"

#define CROSSTALK_EDA_IC_FPGA_EMU_ERR_PATH 960
#define CROSSTALK_EDA_IC_FPGA_EMU_ERR_SPAN 961
#define CROSSTALK_EDA_IC_FPGA_EMU_ERR_BIND 962
#define CROSSTALK_EDA_IC_FPGA_EMU_SPAN_MAX ((uint64_t)256 * LDG_MIB)
#define CROSSTALK_EDA_IC_FPGA_EMU_FILE_MODE 0644
#define CROSSTALK_EDA_IC_FPGA_EMU_REV_NONE 0
#define CROSSTALK_EDA_IC_FPGA_EMU_RAM_BYTES ((uint64_t)512 * LDG_MIB)
#define CROSSTALK_EDA_IC_FPGA_EMU_POOL_VAR_SIZE 0
#define CROSSTALK_EDA_IC_FPGA_EMU_POOL_BYTES_MIN 4096
#define CROSSTALK_EDA_IC_FPGA_EMU_DISK_SPAN_NONE 0
#define CROSSTALK_EDA_IC_FPGA_EMU_BLK_STEP 1
#define CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK "MEMCAP"
#define CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK_LEN 6
#define CROSSTALK_EDA_IC_FPGA_EMU_CAP_MARK_SLOT 8
#define CROSSTALK_EDA_IC_FPGA_EMU_ELEM_BYTES 8
#define CROSSTALK_EDA_IC_FPGA_EMU_CK_SECS 1.0
#define CROSSTALK_EDA_IC_FPGA_EMU_SETTLE_NS ((uint64_t)500 * LDG_NS_PER_MS)
#define CROSSTALK_EDA_IC_FPGA_EMU_ERR_POOL 963
#define CROSSTALK_EDA_IC_FPGA_EMU_ERR_DISK 964
#define CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX 8
#define CROSSTALK_EDA_IC_FPGA_RAM_WINDOW_TOP 0x0001000000000000ULL
#define CROSSTALK_EDA_IC_FPGA_EMU_CLK_HZ 750000000

#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_MAX 64
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP 24
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_MAGIC 0x554D4531U
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_REV 1
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_HDR_BYTES 12
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_HDR_BYTES 8
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_TOTALS 1
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_PROGS 2
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_TYPING 3
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_BLK_CUNT 3
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_TOTALS_WORDS 6
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_TYPING_WORDS 4
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_WORDS 5
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_GAP_MS_MAX 4000
#define CROSSTALK_EDA_IC_FPGA_EMU_STAT_BS 0x08

typedef struct crosstalk_eda_ic_fpga_emu_stat_prog
{
    char name[CROSSTALK_EDA_IC_FPGA_EMU_STAT_NAME_CAP];
    uint64_t run_ms;
    uint64_t hlt_ms;
    uint64_t launches;
    uint64_t last_epoch_ms;
    uint64_t instr;
} crosstalk_eda_ic_fpga_emu_stat_prog_t;

typedef struct crosstalk_eda_ic_fpga_emu_stat
{
    crosstalk_eda_ic_fpga_emu_stat_prog_t prog[CROSSTALK_EDA_IC_FPGA_EMU_STAT_PROG_MAX];
    uint64_t run_ms;
    uint64_t hlt_ms;
    uint64_t sesh_cunt;
    uint64_t instr;
    uint64_t cycles;
    uint64_t first_epoch_ms;
    uint64_t chars;
    uint64_t keys;
    uint64_t backspaces;
    uint64_t rate_peak;
    uint64_t step_at_ms;
    uint32_t prog_cunt;
    uint32_t cur;
    uint8_t live_prev;
    uint8_t pudding[7];
} crosstalk_eda_ic_fpga_emu_stat_t;

typedef struct crosstalk_eda_ic_fpga_emu_ctx
{
    crosstalk_hdl_img_t img;
    crosstalk_hdl_abi_t abi;
    ldg_byte_t *blob;
    ldg_byte_t *pool;
    ldg_mem_pool_t *pool_backing;
    ldg_byte_t *disk[CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX];
    uint64_t blob_span;
    uint64_t pool_span;
    uint64_t disk_span[CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX];
    uint64_t src_mtime_ns;
    uint64_t src_span;
    double ck_next;
    char path[LDG_IO_PATH_MAX];
    char disk_path[CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX][LDG_IO_PATH_MAX];
    uint8_t ok;
    uint8_t disk_persist;
    uint8_t pudding[6];
} crosstalk_eda_ic_fpga_emu_ctx_t;

uint32_t crosstalk_eda_ic_fpga_emu_init(crosstalk_eda_ic_fpga_emu_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_emu_shutdown(crosstalk_eda_ic_fpga_emu_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_emu_attach(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, const char *path);
uint32_t crosstalk_eda_ic_fpga_emu_rst(crosstalk_eda_ic_fpga_emu_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_emu_detach(crosstalk_eda_ic_fpga_emu_ctx_t *ctx);
uint8_t crosstalk_eda_ic_fpga_emu_live_is(const crosstalk_eda_ic_fpga_emu_ctx_t *ctx);
uint32_t crosstalk_eda_ic_fpga_emu_stat_init(crosstalk_eda_ic_fpga_emu_stat_t *stat);
uint32_t crosstalk_eda_ic_fpga_emu_stat_step(crosstalk_eda_ic_fpga_emu_stat_t *stat, uint8_t live_is, uint8_t hlt_is, uint64_t now_ms);
uint32_t crosstalk_eda_ic_fpga_emu_stat_prog_open(crosstalk_eda_ic_fpga_emu_stat_t *stat, const char *name, uint64_t now_ms);
uint32_t crosstalk_eda_ic_fpga_emu_stat_prog_close(crosstalk_eda_ic_fpga_emu_stat_t *stat);
uint32_t crosstalk_eda_ic_fpga_emu_stat_typed_put(crosstalk_eda_ic_fpga_emu_stat_t *stat, const uint8_t *bytes, uint32_t len, uint64_t now_ms);
uint32_t crosstalk_eda_ic_fpga_emu_stat_serialize(const crosstalk_eda_ic_fpga_emu_stat_t *stat, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_eda_ic_fpga_emu_stat_deserialize(crosstalk_eda_ic_fpga_emu_stat_t *stat, const uint8_t *buff, uint32_t len);
uint8_t crosstalk_eda_ic_fpga_emu_stale_is(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, double now);
uint32_t crosstalk_eda_ic_fpga_emu_abi_get(const crosstalk_eda_ic_fpga_emu_ctx_t *ctx, const crosstalk_hdl_abi_t **out);
uint32_t crosstalk_eda_ic_fpga_emu_disk_ld(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive, const char *path);
uint32_t crosstalk_eda_ic_fpga_emu_disk_sync(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive);
uint32_t crosstalk_eda_ic_fpga_emu_disk_eject(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive);
uint32_t crosstalk_eda_ic_fpga_emu_disk_bind(crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive);
uint8_t crosstalk_eda_ic_fpga_emu_disk_live_is(const crosstalk_eda_ic_fpga_emu_ctx_t *ctx, uint32_t drive);

#endif
