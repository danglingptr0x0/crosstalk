#ifndef CROSSTALK_HDL_ABI_H
#define CROSSTALK_HDL_ABI_H

#include <stdint.h>

#include <dangling/core/types.h>

#include "hdl/img.h"

#define CROSSTALK_HDL_ERR_ABI_SYM 771
#define CROSSTALK_HDL_ERR_ABI_REV 772
#define CROSSTALK_HDL_ERR_ABI_KIND 773

#define CROSSTALK_HDL_ABI_REV 1

#define CROSSTALK_HDL_ABI_SYM_REV "hdl_abi_rev"
#define CROSSTALK_HDL_ABI_SYM_INIT "hdl_abi_init"
#define CROSSTALK_HDL_ABI_SYM_MEM_INIT "hdl_abi_mem_init"
#define CROSSTALK_HDL_ABI_SYM_N_CYCLE "hdl_abi_n_cycle"
#define CROSSTALK_HDL_ABI_SYM_SERIAL_DRAIN "hdl_abi_serial_drain"
#define CROSSTALK_HDL_ABI_SYM_CYCLE "hdl_abi_cycle"
#define CROSSTALK_HDL_ABI_SYM_SIG_FIND "hdl_abi_sig_find"
#define CROSSTALK_HDL_ABI_SYM_POKE "hdl_abi_poke"
#define CROSSTALK_HDL_ABI_SYM_PEEK "hdl_abi_peek"
#define CROSSTALK_HDL_ABI_SYM_MEM_ST "hdl_abi_mem_st"
#define CROSSTALK_HDL_ABI_SYM_MEM_LD "hdl_abi_mem_ld"
#define CROSSTALK_HDL_ABI_SYM_STATE "hdl_abi_state_get"
#define CROSSTALK_HDL_ABI_SYM_TOUCH "hdl_abi_touch"
#define CROSSTALK_HDL_ABI_SYM_HLT_MASK "hdl_abi_hlt_mask_set"
#define CROSSTALK_HDL_ABI_SYM_DISK_SET "hdl_abi_disk_set"
#define CROSSTALK_HDL_ABI_SYM_DISK_DRIVE_SET "hdl_abi_disk_drive_set"
#define CROSSTALK_HDL_ABI_SYM_DISK_BLK_WIDTH "hdl_abi_disk_blk_width_get"
#define CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_IS "hdl_abi_disk_dirty_is"
#define CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_LO "hdl_abi_disk_dirty_lo_get"
#define CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_HI "hdl_abi_disk_dirty_hi_get"
#define CROSSTALK_HDL_ABI_SYM_DISK_DIRTY_CLEAR "hdl_abi_disk_dirty_clear"
#define CROSSTALK_HDL_ABI_SYM_AUDIO_DRAIN "hdl_abi_audio_drain"
#define CROSSTALK_HDL_ABI_SYM_AUDIO_RATE "hdl_abi_audio_rate_get"
#define CROSSTALK_HDL_ABI_SYM_NET_DRAIN "hdl_abi_net_drain"
#define CROSSTALK_HDL_ABI_SYM_NET_FEED "hdl_abi_net_feed"
#define CROSSTALK_HDL_ABI_SYM_PROF_EN "hdl_abi_prof_en"
#define CROSSTALK_HDL_ABI_SYM_BRK_SET "hdl_abi_brk_set"
#define CROSSTALK_HDL_ABI_SYM_BRK_SKIP_SET "hdl_abi_brk_skip_set"
#define CROSSTALK_HDL_ABI_SYM_BRK_COND_SET "hdl_abi_brk_cond_set"
#define CROSSTALK_HDL_ABI_SYM_TIME_SET "hdl_abi_time_set"
#define CROSSTALK_HDL_ABI_SYM_EPOCH_SET "hdl_abi_epoch_set"
#define CROSSTALK_HDL_ABI_SYM_DDU_FRAME_GET "hdl_abi_ddu_frame_get"
#define CROSSTALK_HDL_ABI_SYM_DDU_TEXT_GET "hdl_abi_ddu_text_get"
#define CROSSTALK_HDL_ABI_DDU_STATUS_SIG "ddu.status"
#define CROSSTALK_HDL_ABI_SYM_WATCH_SET "hdl_abi_watch_set"
#define CROSSTALK_HDL_ABI_SYM_IO_RD "hdl_abi_io_rd"
#define CROSSTALK_HDL_ABI_SYM_IO_MAP "hdl_abi_io_map_get"
#define CROSSTALK_HDL_ABI_SYM_IO_WR "hdl_abi_io_wr"
#define CROSSTALK_HDL_ABI_SYM_RELAY_REQ_TAKE "hdl_abi_relay_req_take"
#define CROSSTALK_HDL_ABI_SYM_RELAY_REPLY_PUSH "hdl_abi_relay_reply_push"
#define CROSSTALK_HDL_ABI_SYM_RELAY_PAYLOAD_CAP "hdl_abi_relay_payload_cap_get"
#define CROSSTALK_HDL_ABI_BRK_COND_NONE 0xFF
#define CROSSTALK_HDL_ABI_BRK_REG_MAX 23

#define CROSSTALK_HDL_ABI_STATE_NODE 0
#define CROSSTALK_HDL_ABI_STATE_MEM 1
#define CROSSTALK_HDL_ABI_STATE_CLK 2
#define CROSSTALK_HDL_ABI_STATE_NODE_HI 3
#define CROSSTALK_HDL_ABI_STATE_MEM_HI 4
#define CROSSTALK_HDL_ABI_STATE_RST 5
#define CROSSTALK_HDL_ABI_STATE_KIND_CUNT 6
#define CROSSTALK_HDL_ABI_STATE_SIG_VALS 6
#define CROSSTALK_HDL_ABI_STATE_PF_T 7
#define CROSSTALK_HDL_ABI_STATE_PF_C 8
#define CROSSTALK_HDL_ABI_STATE_PF_RING_TOT 9
#define CROSSTALK_HDL_ABI_STATE_PF_RING_COMB 10
#define CROSSTALK_HDL_ABI_STATE_PF_RING_BLK 11
#define CROSSTALK_HDL_ABI_STATE_PF_RING_CMT 12
#define CROSSTALK_HDL_ABI_STATE_PF_META 13
#define CROSSTALK_HDL_ABI_STATE_PF_SUB 14
#define CROSSTALK_HDL_ABI_STATE_PF_NM 15
#define CROSSTALK_HDL_ABI_STATE_EMU_FAULT 20
#define CROSSTALK_HDL_ABI_FAULT_CUNT 8
#define CROSSTALK_HDL_ABI_FAULT_VALID_AT 0
#define CROSSTALK_HDL_ABI_FAULT_CAUSE_AT 1
#define CROSSTALK_HDL_ABI_FAULT_LOC_AT 2
#define CROSSTALK_HDL_ABI_FAULT_INSTR_AT 3
#define CROSSTALK_HDL_ABI_FAULT_ADDR_AT 4
#define CROSSTALK_HDL_ABI_FAULT_SIZE_AT 5
#define CROSSTALK_HDL_ABI_FAULT_SP_AT 6
#define CROSSTALK_HDL_ABI_FAULT_STEP_AT 7
#define CROSSTALK_HDL_ABI_STATE_EMU_OP 16
#define CROSSTALK_HDL_ABI_OP_CUNT 16
#define CROSSTALK_HDL_ABI_STATE_EMU_TRAP 18
#define CROSSTALK_HDL_ABI_TRAP_CUNT 8
#define CROSSTALK_HDL_ABI_STATE_EMU_HOST 19
#define CROSSTALK_HDL_ABI_HOST_CUNT 4
#define CROSSTALK_HDL_ABI_HOST_RETIRED_AT 0
#define CROSSTALK_HDL_ABI_HOST_NS_AT 1
#define CROSSTALK_HDL_ABI_HOST_CYCLES_AT 2
#define CROSSTALK_HDL_ABI_HOST_EN_AT 3
#define CROSSTALK_HDL_ABI_STATE_EMU_MEM 17
#define CROSSTALK_HDL_ABI_MEM_CUNT 4
#define CROSSTALK_HDL_ABI_MEM_RAM_RD_AT 0
#define CROSSTALK_HDL_ABI_MEM_RAM_WR_AT 1
#define CROSSTALK_HDL_ABI_MEM_MMIO_RD_AT 2
#define CROSSTALK_HDL_ABI_MEM_MMIO_WR_AT 3
#define CROSSTALK_HDL_ABI_STATE_EMU_RING 22
#define CROSSTALK_HDL_ABI_RING_STRIDE 4
#define CROSSTALK_HDL_ABI_RING_FROM_AT 0
#define CROSSTALK_HDL_ABI_RING_INSTR_AT 1
#define CROSSTALK_HDL_ABI_RING_TO_AT 2
#define CROSSTALK_HDL_ABI_RING_SP_AT 3
#define CROSSTALK_HDL_ABI_RING_ENTRY_MAX 4096
#define CROSSTALK_HDL_ABI_STATE_EMU_FB 23
#define CROSSTALK_HDL_ABI_FB_CUNT 4
#define CROSSTALK_HDL_ABI_FB_ADDR_AT 0
#define CROSSTALK_HDL_ABI_FB_FMT_AT 1
#define CROSSTALK_HDL_ABI_FB_MODE_AT 2
#define CROSSTALK_HDL_ABI_FB_FRAME_AT 3
#define CROSSTALK_HDL_ABI_STATE_EMU_CQ 24
#define CROSSTALK_HDL_ABI_STATE_EMU_MEM_MUT 25
#define CROSSTALK_HDL_ABI_MEM_MUT_CUNT 1
#define CROSSTALK_HDL_ABI_MEM_MUT_ADDR 0
#define CROSSTALK_HDL_ABI_CQ_CUNT 6
#define CROSSTALK_HDL_ABI_CQ_BASE_AT 0
#define CROSSTALK_HDL_ABI_CQ_SIZE_AT 1
#define CROSSTALK_HDL_ABI_CQ_TAIL_AT 2
#define CROSSTALK_HDL_ABI_CQ_HD_AT 3
#define CROSSTALK_HDL_ABI_CQ_CTRL_AT 4
#define CROSSTALK_HDL_ABI_CQ_FRAME_AT 5
#define CROSSTALK_HDL_ABI_CQ_CTRL_EN 0x1
#define CROSSTALK_HDL_ABI_CQ_HD_SIG "ddu.cq_hd"
#define CROSSTALK_HDL_ABI_CQ_CONSUMER_SIG "ddu.cq_consumer"
#define CROSSTALK_HDL_ABI_CQ_FRAME_SIG "ddu.fb_frame"
#define CROSSTALK_HDL_ABI_FB_FMT_NONE 0
#define CROSSTALK_HDL_ABI_FB_FMT_XRGB8888 1
#define CROSSTALK_HDL_ABI_FB_FMT_TEXT_RASTER 2
#define CROSSTALK_HDL_ABI_FB_FMT_CARD 3
#define CROSSTALK_HDL_ABI_FB_MODE_W_MASK 0xFFFF
#define CROSSTALK_HDL_ABI_FB_MODE_H_SHIFT 16
#define CROSSTALK_HDL_ABI_STATE_EMU_DSU 21
#define CROSSTALK_HDL_ABI_DSU_CUNT 5
#define CROSSTALK_HDL_ABI_DSU_BASE_AT 0
#define CROSSTALK_HDL_ABI_DSU_TAIL_AT 1
#define CROSSTALK_HDL_ABI_DSU_CURR_AT 2
#define CROSSTALK_HDL_ABI_DSU_RATE_AT 3
#define CROSSTALK_HDL_ABI_DSU_RUN_AT 4
#define CROSSTALK_HDL_ABI_AUDIO_SILENCE 128
#define CROSSTALK_HDL_ABI_CAUSE_MIN 1
#define CROSSTALK_HDL_ABI_CAUSE_MAX 6
#define CROSSTALK_HDL_ABI_CAUSE_INVALID 1
#define CROSSTALK_HDL_ABI_CAUSE_UNALIGNED 2
#define CROSSTALK_HDL_ABI_CAUSE_STACK_OVER 3
#define CROSSTALK_HDL_ABI_CAUSE_STACK_UNDER 4
#define CROSSTALK_HDL_ABI_CAUSE_DIV_ZERO 5
#define CROSSTALK_HDL_ABI_CAUSE_DR14 6
#define CROSSTALK_HDL_ABI_PF_RING_CAP 4096
#define CROSSTALK_HDL_ABI_PF_META_CAP 16
#define CROSSTALK_HDL_ABI_PF_SUB_CAP 8
#define CROSSTALK_HDL_ABI_PF_NM_BYTES 32

#define CROSSTALK_HDL_ABI_SIG_NONE 4294967295U

typedef void (*crosstalk_hdl_abi_init_cb)(void);
typedef uint32_t (*crosstalk_hdl_abi_mem_init_cb)(void *ram, uint64_t len, uint64_t ram_limit);
typedef uint32_t (*crosstalk_hdl_abi_n_cycle_cb)(uint32_t clk_sig, uint64_t cunt, uint64_t *ran);
typedef uint32_t (*crosstalk_hdl_abi_serial_drain_cb)(uint8_t *dst, uint64_t cap, uint64_t *got);
typedef uint32_t (*crosstalk_hdl_abi_rev_cb)(void);
typedef uint32_t (*crosstalk_hdl_abi_cycle_cb)(uint32_t clk_sig);
typedef uint32_t (*crosstalk_hdl_abi_sig_find_cb)(const char *name);
typedef void (*crosstalk_hdl_abi_poke_cb)(uint32_t sig, uint64_t val);
typedef uint64_t (*crosstalk_hdl_abi_peek_cb)(uint32_t sig);
typedef void (*crosstalk_hdl_abi_mem_st_cb)(uint32_t sig, uint64_t addr, uint64_t val);
typedef uint64_t (*crosstalk_hdl_abi_mem_ld_cb)(uint32_t sig, uint64_t addr);
typedef uint32_t (*crosstalk_hdl_abi_state_cb)(uint32_t kind, uint64_t **out_base, uint32_t *out_cunt);
typedef void (*crosstalk_hdl_abi_touch_cb)(void);
typedef uint32_t (*crosstalk_hdl_abi_hlt_mask_set_cb)(uint32_t mask);
typedef uint32_t (*crosstalk_hdl_abi_disk_set_cb)(void *media, uint64_t len);
typedef uint32_t (*crosstalk_hdl_abi_disk_drive_set_cb)(uint32_t drive, void *media, uint64_t len);
typedef uint32_t (*crosstalk_hdl_abi_disk_blk_width_get_cb)(void);
typedef uint8_t (*crosstalk_hdl_abi_disk_dirty_is_cb)(uint32_t drive);
typedef uint64_t (*crosstalk_hdl_abi_disk_dirty_lo_get_cb)(uint32_t drive);
typedef uint64_t (*crosstalk_hdl_abi_disk_dirty_hi_get_cb)(uint32_t drive);
typedef uint32_t (*crosstalk_hdl_abi_disk_dirty_clear_cb)(uint32_t drive);
typedef uint32_t (*crosstalk_hdl_abi_audio_drain_cb)(uint8_t *dst, uint64_t cap, uint64_t *got);
typedef uint32_t (*crosstalk_hdl_abi_audio_rate_get_cb)(void);
typedef uint32_t (*crosstalk_hdl_abi_net_drain_cb)(uint8_t *dst, uint64_t cap, uint64_t *got);
typedef uint32_t (*crosstalk_hdl_abi_net_feed_cb)(const uint8_t *src, uint64_t len, uint64_t *taken);
typedef uint32_t (*crosstalk_hdl_abi_prof_en_cb)(uint32_t on);
typedef uint32_t (*crosstalk_hdl_abi_brk_set_cb)(uint64_t addr, uint8_t on);
typedef uint32_t (*crosstalk_hdl_abi_brk_skip_set_cb)(uint32_t cunt);
typedef uint32_t (*crosstalk_hdl_abi_brk_cond_set_cb)(uint8_t reg, uint64_t min);
typedef uint32_t (*crosstalk_hdl_abi_watch_set_cb)(uint64_t lo, uint64_t hi, uint8_t on);
typedef uint32_t (*crosstalk_hdl_abi_io_rd_cb)(uint64_t addr, uint64_t *out);
typedef uint32_t (*crosstalk_hdl_abi_io_map_get_cb)(uint32_t idx, uint64_t *out_base, uint64_t *out_span, const char **out_name);
typedef uint32_t (*crosstalk_hdl_abi_io_wr_cb)(uint64_t addr, uint64_t val);
typedef uint32_t (*crosstalk_hdl_abi_time_set_cb)(uint64_t ns);
typedef uint32_t (*crosstalk_hdl_abi_epoch_set_cb)(uint64_t sec);
typedef uint32_t (*crosstalk_hdl_abi_ddu_frame_get_cb)(uint8_t *dst, uint64_t cap, uint32_t *out_w, uint32_t *out_h);
typedef uint32_t (*crosstalk_hdl_abi_ddu_text_get_cb)(uint8_t *dst, uint64_t cap, uint32_t *out_cols, uint32_t *out_rows);
typedef uint32_t (*crosstalk_hdl_abi_relay_req_take_cb)(uint8_t *dst, uint32_t cap, uint32_t *got);
typedef uint32_t (*crosstalk_hdl_abi_relay_reply_push_cb)(const uint8_t *payload, uint32_t len);
typedef uint32_t (*crosstalk_hdl_abi_relay_payload_cap_get_cb)(void);

typedef enum crosstalk_hdl_abi_rva
{
    CROSSTALK_HDL_ABI_RVA_REV = 0,
    CROSSTALK_HDL_ABI_RVA_INIT,
    CROSSTALK_HDL_ABI_RVA_MEM_INIT,
    CROSSTALK_HDL_ABI_RVA_N_CYCLE,
    CROSSTALK_HDL_ABI_RVA_SERIAL_DRAIN,
    CROSSTALK_HDL_ABI_RVA_CYCLE,
    CROSSTALK_HDL_ABI_RVA_SIG_FIND,
    CROSSTALK_HDL_ABI_RVA_POKE,
    CROSSTALK_HDL_ABI_RVA_PEEK,
    CROSSTALK_HDL_ABI_RVA_MEM_ST,
    CROSSTALK_HDL_ABI_RVA_MEM_LD,
    CROSSTALK_HDL_ABI_RVA_STATE_GET,
    CROSSTALK_HDL_ABI_RVA_TOUCH,
    CROSSTALK_HDL_ABI_RVA_HLT_MASK_SET,
    CROSSTALK_HDL_ABI_RVA_DISK_SET,
    CROSSTALK_HDL_ABI_RVA_DISK_DRIVE_SET,
    CROSSTALK_HDL_ABI_RVA_DISK_BLK_WIDTH_GET,
    CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_IS,
    CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_LO_GET,
    CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_HI_GET,
    CROSSTALK_HDL_ABI_RVA_DISK_DIRTY_CLEAR,
    CROSSTALK_HDL_ABI_RVA_AUDIO_DRAIN,
    CROSSTALK_HDL_ABI_RVA_AUDIO_RATE_GET,
    CROSSTALK_HDL_ABI_RVA_PROF_EN,
    CROSSTALK_HDL_ABI_RVA_BRK_SET,
    CROSSTALK_HDL_ABI_RVA_BRK_SKIP_SET,
    CROSSTALK_HDL_ABI_RVA_BRK_COND_SET,
    CROSSTALK_HDL_ABI_RVA_WATCH_SET,
    CROSSTALK_HDL_ABI_RVA_TIME_SET,
    CROSSTALK_HDL_ABI_RVA_IO_RD,
    CROSSTALK_HDL_ABI_RVA_IO_MAP_GET,
    CROSSTALK_HDL_ABI_RVA_IO_WR,
    CROSSTALK_HDL_ABI_RVA_RELAY_REQ_TAKE,
    CROSSTALK_HDL_ABI_RVA_RELAY_REPLY_PUSH,
    CROSSTALK_HDL_ABI_RVA_RELAY_PAYLOAD_CAP_GET,
    CROSSTALK_HDL_ABI_RVA_EPOCH_SET,
    CROSSTALK_HDL_ABI_RVA_DDU_FRAME_GET,
    CROSSTALK_HDL_ABI_RVA_NET_DRAIN,
    CROSSTALK_HDL_ABI_RVA_NET_FEED,
    CROSSTALK_HDL_ABI_RVA_DDU_TEXT_GET,
    CROSSTALK_HDL_ABI_RVA_CUNT
} crosstalk_hdl_abi_rva_t;

typedef struct crosstalk_hdl_abi
{
    crosstalk_hdl_abi_rev_cb rev;
    crosstalk_hdl_abi_init_cb init;
    crosstalk_hdl_abi_mem_init_cb mem_init;
    crosstalk_hdl_abi_n_cycle_cb n_cycle;
    crosstalk_hdl_abi_serial_drain_cb serial_drain;
    crosstalk_hdl_abi_cycle_cb cycle;
    crosstalk_hdl_abi_sig_find_cb sig_find;
    crosstalk_hdl_abi_poke_cb poke;
    crosstalk_hdl_abi_peek_cb peek;
    crosstalk_hdl_abi_mem_st_cb mem_st;
    crosstalk_hdl_abi_mem_ld_cb mem_ld;
    crosstalk_hdl_abi_state_cb state_get;
    crosstalk_hdl_abi_touch_cb touch;
    crosstalk_hdl_abi_hlt_mask_set_cb hlt_mask_set;
    crosstalk_hdl_abi_disk_set_cb disk_set;
    crosstalk_hdl_abi_disk_drive_set_cb disk_drive_set;
    crosstalk_hdl_abi_disk_blk_width_get_cb disk_blk_width_get;
    crosstalk_hdl_abi_disk_dirty_is_cb disk_dirty_is;
    crosstalk_hdl_abi_disk_dirty_lo_get_cb disk_dirty_lo_get;
    crosstalk_hdl_abi_disk_dirty_hi_get_cb disk_dirty_hi_get;
    crosstalk_hdl_abi_disk_dirty_clear_cb disk_dirty_clear;
    crosstalk_hdl_abi_audio_drain_cb audio_drain;
    crosstalk_hdl_abi_audio_rate_get_cb audio_rate_get;
    crosstalk_hdl_abi_prof_en_cb prof_en;
    crosstalk_hdl_abi_brk_set_cb brk_set;
    crosstalk_hdl_abi_brk_skip_set_cb brk_skip_set;
    crosstalk_hdl_abi_brk_cond_set_cb brk_cond_set;
    crosstalk_hdl_abi_watch_set_cb watch_set;
    crosstalk_hdl_abi_time_set_cb time_set;
    crosstalk_hdl_abi_io_rd_cb io_rd;
    crosstalk_hdl_abi_io_map_get_cb io_map_get;
    crosstalk_hdl_abi_io_wr_cb io_wr;
    crosstalk_hdl_abi_relay_req_take_cb relay_req_take;
    crosstalk_hdl_abi_relay_reply_push_cb relay_reply_push;
    crosstalk_hdl_abi_relay_payload_cap_get_cb relay_payload_cap_get;
    crosstalk_hdl_abi_epoch_set_cb epoch_set;
    crosstalk_hdl_abi_ddu_frame_get_cb ddu_frame_get;
    crosstalk_hdl_abi_net_drain_cb net_drain;
    crosstalk_hdl_abi_net_feed_cb net_feed;
    crosstalk_hdl_abi_ddu_text_get_cb ddu_text_get;
    uint8_t ok;
    uint8_t pudding[7];
} crosstalk_hdl_abi_t;

uint32_t crosstalk_hdl_abi_bind(const crosstalk_hdl_img_t *img, const uint32_t *rva, uint32_t rva_cunt, crosstalk_hdl_abi_t *out);

#endif
