#ifndef CROSSTALK_MNGR_CONF_H
#define CROSSTALK_MNGR_CONF_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>

#include "eda/ic/fpga/emu.h"
#include "emu/emu.h"

#define CROSSTALK_MNGR_ERR_MAGIC 1040
#define CROSSTALK_MNGR_ERR_REV 1041
#define CROSSTALK_MNGR_ERR_TRUNC 1042
#define CROSSTALK_MNGR_ERR_BLK 1043
#define CROSSTALK_MNGR_ERR_FULL 1044
#define CROSSTALK_MNGR_ERR_NAME 1045

#define CROSSTALK_MNGR_CONF_MAGIC 0x4D4D4752U
#define CROSSTALK_MNGR_CONF_REV 1
#define CROSSTALK_MNGR_CONF_HDR_BYTES 12
#define CROSSTALK_MNGR_CONF_BLK_HDR_BYTES 8
#define CROSSTALK_MNGR_CONF_BLK_MACHINE 1
#define CROSSTALK_MNGR_CONF_BLK_IMGS 2
#define CROSSTALK_MNGR_CONF_BLK_DISKS 3
#define CROSSTALK_MNGR_CONF_BLK_NET 4
#define CROSSTALK_MNGR_CONF_BLK_CUNT 4

#define CROSSTALK_MNGR_CONF_NAME_CAP 48
#define CROSSTALK_MNGR_CONF_NOTE_CAP 512
#define CROSSTALK_MNGR_CONF_BLOB_CAP 131072
#define CROSSTALK_MNGR_CONF_EXT ".machine"
#define CROSSTALK_MNGR_CONF_DIR_REL "machines"
#define CROSSTALK_MNGR_CONF_ORG_WIN "Ongakken"
#define CROSSTALK_MNGR_CONF_ORG_UNIX "ongakken"
#define CROSSTALK_MNGR_CONF_APP "emu"
#define CROSSTALK_MNGR_CONF_SEG_CUNT 3

#define CROSSTALK_MNGR_CONF_RAM_MIN ((uint64_t)4 * LDG_MIB)
#define CROSSTALK_MNGR_CONF_RAM_MAX ((uint64_t)512 * LDG_MIB)
#define CROSSTALK_MNGR_CONF_HZ_MIN 1000000
#define CROSSTALK_MNGR_CONF_HZ_MAX 2000000000

typedef struct crosstalk_mngr_machine
{
    char name[CROSSTALK_MNGR_CONF_NAME_CAP];
    char note[CROSSTALK_MNGR_CONF_NOTE_CAP];
    char core_path[LDG_IO_PATH_MAX];
    char font_path[LDG_IO_PATH_MAX];
    char ld_path[CROSSTALK_EMU_LD_MAX][LDG_IO_PATH_MAX];
    char disk_path[CROSSTALK_EDA_IC_FPGA_EMU_DRIVE_MAX][LDG_IO_PATH_MAX];
    char net_path[LDG_IO_PATH_MAX];
    uint64_t ld_off[CROSSTALK_EMU_LD_MAX];
    uint64_t ram_bytes;
    uint64_t f_op_hz;
    double gamma;
    uint32_t ld_cunt;
    uint32_t win_w;
    uint32_t win_h;
    uint32_t key_mode;
    uint8_t audio_on;
    uint8_t serial_on;
    uint8_t disk_persist;
    uint8_t pudding[5];
} crosstalk_mngr_machine_t;

uint32_t crosstalk_mngr_conf_default_get(crosstalk_mngr_machine_t *out);
uint32_t crosstalk_mngr_conf_serialize(const crosstalk_mngr_machine_t *machine, uint8_t *out, uint32_t cap, uint32_t *out_len);
uint32_t crosstalk_mngr_conf_deserialize(crosstalk_mngr_machine_t *out, const uint8_t *buff, uint32_t len);
uint32_t crosstalk_mngr_conf_wr(const crosstalk_mngr_machine_t *machine, const char *path);
uint32_t crosstalk_mngr_conf_rd(crosstalk_mngr_machine_t *out, const char *path);
uint32_t crosstalk_mngr_conf_path_get(const char *dir, const char *name, char *out, uint64_t cap);
uint8_t crosstalk_mngr_conf_name_ok_is(const char *name);

#endif
