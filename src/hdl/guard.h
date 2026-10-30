#ifndef CROSSTALK_HDL_GUARD_H
#define CROSSTALK_HDL_GUARD_H

#include <stdint.h>

#ifdef _WIN32
#include <setjmp.h>
#else
#include <setjmp.h>
#include <signal.h>
#endif

#include <dangling/core/types.h>

#define CROSSTALK_HDL_ERR_GUARD_FULL 762
#define CROSSTALK_HDL_ERR_GUARD_INSTALL 763
#define CROSSTALK_HDL_ERR_GUARD_RANGE 764
#define CROSSTALK_HDL_ERR_GUARD_FAULT 765
#define CROSSTALK_HDL_ERR_GUARD_LIVE 766

#define CROSSTALK_HDL_GUARD_RANGE_MAX 8
#define CROSSTALK_HDL_GUARD_STEP 1
#define CROSSTALK_HDL_GUARD_BAIL 1
#define CROSSTALK_HDL_GUARD_VEH_FIRST 1
#define CROSSTALK_HDL_GUARD_REG_RIP 16

typedef struct crosstalk_hdl_guard_range
{
    uint64_t base;
    uint64_t span;
    uint32_t slot;
    uint8_t live;
    uint8_t pudding[3];
} crosstalk_hdl_guard_range_t;

typedef struct crosstalk_hdl_guard_ctx
{
    crosstalk_hdl_guard_range_t range[CROSSTALK_HDL_GUARD_RANGE_MAX];
#ifdef _WIN32
    jmp_buf bail;
    void *handle;
#else
    sigjmp_buf bail;
    struct sigaction prev_segv;
    struct sigaction prev_ill;
    struct sigaction prev_fpe;
    struct sigaction prev_bus;
#endif
    uint64_t fault_addr;
    uint32_t fault_slot;
    uint32_t fault_cunt;
    uint8_t armed;
    uint8_t ok;
    uint8_t pudding[6];
} crosstalk_hdl_guard_ctx_t;

#ifdef _WIN32
#define CROSSTALK_HDL_GUARD_ENTER(ctx) (((ctx)->armed = LDG_TRUTH_TRUE), setjmp((ctx)->bail) == 0)
#else
#define CROSSTALK_HDL_GUARD_ENTER(ctx) (((ctx)->armed = LDG_TRUTH_TRUE), sigsetjmp((ctx)->bail, CROSSTALK_HDL_GUARD_BAIL) == 0)
#endif

#define CROSSTALK_HDL_GUARD_LEAVE(ctx) ((ctx)->armed = LDG_TRUTH_FALSE)

uint32_t crosstalk_hdl_guard_init(crosstalk_hdl_guard_ctx_t *ctx);
uint32_t crosstalk_hdl_guard_shutdown(crosstalk_hdl_guard_ctx_t *ctx);
uint32_t crosstalk_hdl_guard_range_add(crosstalk_hdl_guard_ctx_t *ctx, uint64_t base, uint64_t span, uint32_t slot);
uint32_t crosstalk_hdl_guard_range_drop(crosstalk_hdl_guard_ctx_t *ctx, uint32_t slot);
uint32_t crosstalk_hdl_guard_fault_get(const crosstalk_hdl_guard_ctx_t *ctx, uint32_t *out);

#endif
