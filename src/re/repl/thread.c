#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/region.h>
#include <dangling/mem/secure.h>
#include <dangling/mem/vmem.h>
#include <dangling/str/str.h>
#include <dangling/sys/mod.h>
#include <dangling/sys/proc.h>
#include <dangling/thread/ref.h>
#include <dangling/time/time.h>

#include "re/repl/repl.h"
#include "re/repl/thread.h"

#define THREAD_STEP 1
#define THREAD_REFRESH_NS (500 * LDG_NS_PER_MS)
#define THREAD_STACK_SPAN LDG_MIB
#define THREAD_MOD_LOOKUP_CAP 16
#define THREAD_PAGE_CAP 32
#define THREAD_ARG_VERB 0
#define THREAD_ARG_X 1
#define THREAD_ARGC_VERB 1
#define THREAD_ARGC_X 2
#define THREAD_WORD_LIST "threads"
#define THREAD_MARK_SEL "*"
#define THREAD_MARK_NONE ""

uint32_t crosstalk_re_repl_thread_reg_name_get(uint32_t idx, const char **out)
{
    static const char *const name[CROSSTALK_RE_REPL_THREAD_REG_CUNT] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rip", "rflags" };

    if (LDG_UNLIKELY(out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (idx >= (uint32_t)CROSSTALK_RE_REPL_THREAD_REG_CUNT) { return LDG_ERR_BOUNDS; }

    *out = name[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_thread_init(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->thread = 0x0;

    ret = ldg_mem_alloc((uint64_t)sizeof(crosstalk_re_repl_thread_t), &ctx->thread);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_secure_zero(ctx->thread, (uint64_t)sizeof(crosstalk_re_repl_thread_t));
}

uint32_t crosstalk_re_repl_thread_shutdown(crosstalk_re_repl_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->thread == 0x0) { return LDG_ERR_AOK; }

    ret = ldg_mem_dealloc(ctx->thread);
    if (ret == LDG_ERR_AOK) { ctx->thread = 0x0; }

    return ret;
}

uint32_t crosstalk_re_repl_thread_rst(crosstalk_re_repl_ctx_t *ctx)
{
    crosstalk_re_repl_thread_t *thread = 0x0;
    uint8_t watch = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->thread == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    thread = (crosstalk_re_repl_thread_t *)ctx->thread;
    watch = thread->watch;

    ret = ldg_mem_secure_zero(thread, (uint64_t)sizeof(*thread));
    if (ret != LDG_ERR_AOK) { return ret; }

    thread->watch = watch;
    thread->live = ctx->attached;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_thread_watch_set(crosstalk_re_repl_ctx_t *ctx, uint8_t on)
{
    crosstalk_re_repl_thread_t *thread = 0x0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->thread == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    thread = (crosstalk_re_repl_thread_t *)ctx->thread;
    thread->watch = on == LDG_TRUTH_TRUE ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_re_repl_thread_ptr_get(const crosstalk_re_repl_ctx_t *ctx, const crosstalk_re_repl_thread_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (const crosstalk_re_repl_thread_t *)ctx->thread;

    return *out != 0x0 ? LDG_ERR_AOK : LDG_ERR_NOT_INIT;
}

static uint32_t thread_ids_ld(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_thread_t *thread)
{
    uint64_t cunt = 0;
    uint32_t ret = 0;

    ret = ldg_thread_id_list(&ctx->proc, thread->id, (uint64_t)CROSSTALK_RE_REPL_THREAD_ID_CAP, &cunt);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cunt > (uint64_t)CROSSTALK_RE_REPL_THREAD_ID_CAP) { cunt = (uint64_t)CROSSTALK_RE_REPL_THREAD_ID_CAP; }

    thread->id_cunt = (uint32_t)cunt;

    if (thread->id_cunt == 0) { return LDG_ERR_EMPTY; }

    if (thread->sel >= thread->id_cunt) { thread->sel = 0; }

    thread->sel_id = thread->id[thread->sel];

    return LDG_ERR_AOK;
}

static uint32_t thread_regs_ld(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_thread_t *thread)
{
    ldg_thread_ref_t ref = LDG_STRUCT_ZERO_INIT;
    ldg_thread_regs_t regs = LDG_STRUCT_ZERO_INIT;
    uint32_t at = 0;
    uint32_t ret = 0;

    ret = ldg_thread_ref_open(&ctx->proc, thread->sel_id, &ref);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_thread_ref_suspend(&ref);
    if (ret == LDG_ERR_AOK)
    {
        ret = ldg_thread_ref_regs_get(&ref, &regs);

        if (ldg_thread_ref_resume(&ref) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_BUSY; }
    }

    if (ldg_thread_ref_close(&ref) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = 0; at < (uint32_t)LDG_THREAD_REG_CUNT; at = at + (uint32_t)THREAD_STEP) { thread->reg[at] = regs.gpr[at]; }

    thread->reg[CROSSTALK_RE_REPL_THREAD_REG_LOC] = regs.rip;
    thread->reg[CROSSTALK_RE_REPL_THREAD_REG_FLAGS] = regs.rflags;

    return LDG_ERR_AOK;
}

static uint8_t thread_tag_get(crosstalk_re_repl_ctx_t *ctx, const crosstalk_re_repl_thread_t *thread, uint64_t val)
{
    ldg_mem_region_t region = LDG_STRUCT_ZERO_INIT;
    uint64_t sp = thread->reg[CROSSTALK_RE_REPL_THREAD_REG_SP];

    if (val == 0) { return (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_NONE; }

    if (sp != 0 && val >= sp && val - sp < (uint64_t)THREAD_STACK_SPAN) { return (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_STACK; }

    if (ldg_mem_region_get(&ctx->proc, val, &region) != LDG_ERR_AOK) { return (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_NONE; }

    if (region.committed != LDG_TRUTH_TRUE || region.readable != LDG_TRUTH_TRUE || region.guard == LDG_TRUTH_TRUE) { return (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_NONE; }

    return region.executable == LDG_TRUTH_TRUE ? (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_CODE : (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_PTR;
}

static void thread_stack_mod_ld(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_thread_t *thread, uint32_t at)
{
    ldg_sys_mod_t mod = LDG_STRUCT_ZERO_INIT;

    thread->stack_mod[at][0] = LDG_STR_TERM;
    thread->stack_mod_off[at] = 0;

    if (ldg_sys_mod_addr_find(&ctx->proc, thread->stack[at], &mod) != LDG_ERR_AOK || mod.base > thread->stack[at]) { return; }

    if (crosstalk_re_repl_name_get(mod.name, thread->stack_mod[at], (uint32_t)CROSSTALK_RE_REPL_THREAD_MOD_NAME_CAP) != LDG_ERR_AOK) { thread->stack_mod[at][0] = LDG_STR_TERM; }

    thread->stack_mod_off[at] = thread->stack[at] - mod.base;
}

static void thread_stack_ld(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_thread_t *thread)
{
    uint64_t got = 0;
    uint32_t lookups = 0;
    uint32_t at = 0;

    thread->stack_addr = thread->reg[CROSSTALK_RE_REPL_THREAD_REG_SP];
    thread->stack_cunt = 0;

    if (ldg_mem_vmem_rd(&ctx->proc, thread->stack_addr, thread->stack, (uint64_t)sizeof(thread->stack), &got) != LDG_ERR_AOK && got > (uint64_t)sizeof(thread->stack)) { got = 0; }

    if (got > (uint64_t)sizeof(thread->stack)) { got = (uint64_t)sizeof(thread->stack); }

    thread->stack_cunt = (uint32_t)(got / (uint64_t)CROSSTALK_RE_REPL_THREAD_STACK_WORD_BYTES);

    for (at = 0; at < thread->stack_cunt; at = at + (uint32_t)THREAD_STEP)
    {
        thread->stack_tag[at] = thread_tag_get(ctx, thread, thread->stack[at]);
        thread->stack_mod[at][0] = LDG_STR_TERM;
        thread->stack_mod_off[at] = 0;

        if (thread->stack_tag[at] != (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_CODE || lookups >= (uint32_t)THREAD_MOD_LOOKUP_CAP) { continue; }

        thread_stack_mod_ld(ctx, thread, at);
        lookups = lookups + (uint32_t)THREAD_STEP;
    }
}

static void thread_code_ld(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_thread_t *thread)
{
    uint64_t got = 0;

    thread->code_addr = thread->reg[CROSSTALK_RE_REPL_THREAD_REG_LOC];
    thread->code_cunt = 0;

    if (ldg_mem_vmem_rd(&ctx->proc, thread->code_addr, thread->code, (uint64_t)sizeof(thread->code), &got) != LDG_ERR_AOK && got > (uint64_t)sizeof(thread->code)) { got = 0; }

    if (got > (uint64_t)sizeof(thread->code)) { got = (uint64_t)sizeof(thread->code); }

    thread->code_cunt = (uint32_t)got;
}

uint32_t crosstalk_re_repl_thread_tick(crosstalk_re_repl_ctx_t *ctx)
{
    crosstalk_re_repl_thread_t *thread = 0x0;
    uint64_t now = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->thread == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    thread = (crosstalk_re_repl_thread_t *)ctx->thread;
    thread->live = ctx->attached;

    if (ctx->attached != LDG_TRUTH_TRUE || thread->watch != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_time_monotonic_ns_get(&now);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (now < thread->next_ns) { return LDG_ERR_AOK; }

    thread->next_ns = now + (uint64_t)THREAD_REFRESH_NS;
    thread->regs_ok = LDG_TRUTH_FALSE;

    ret = thread_ids_ld(ctx, thread);
    if (ret == LDG_ERR_AOK) { ret = thread_regs_ld(ctx, thread); }

    thread->last_err = ret;

    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    thread->regs_ok = LDG_TRUTH_TRUE;

    for (at = 0; at < (uint32_t)CROSSTALK_RE_REPL_THREAD_REG_FLAGS; at = at + (uint32_t)THREAD_STEP) { thread->reg_tag[at] = thread_tag_get(ctx, thread, thread->reg[at]); }

    thread->reg_tag[CROSSTALK_RE_REPL_THREAD_REG_FLAGS] = (uint8_t)CROSSTALK_RE_REPL_THREAD_TAG_NONE;

    thread_stack_ld(ctx, thread);
    thread_code_ld(ctx, thread);

    return LDG_ERR_AOK;
}

static uint32_t thread_list(crosstalk_re_repl_ctx_t *ctx, crosstalk_re_repl_thread_t *thread, uint32_t start)
{
    uint32_t at = 0;
    uint32_t rows = 0;
    uint32_t ret = 0;

    ret = thread_ids_ld(ctx, thread);
    if (ret != LDG_ERR_AOK) { return ret; }

    for (at = start; at < thread->id_cunt && rows < (uint32_t)THREAD_PAGE_CAP; at = at + (uint32_t)THREAD_STEP)
    {
        ret = crosstalk_re_repl_emit(ctx, "thread: %u32; id: %u64%.1s", at, thread->id[at], at == thread->sel ? THREAD_MARK_SEL : THREAD_MARK_NONE);
        if (ret != LDG_ERR_AOK) { return ret; }

        rows = rows + (uint32_t)THREAD_STEP;
    }

    return crosstalk_re_repl_emit(ctx, "threads: %u32; next start: %u32; more: %u32; selected: %u32", rows, at, at < thread->id_cunt ? (uint32_t)LDG_TRUTH_TRUE : (uint32_t)LDG_TRUTH_FALSE, thread->sel);
}

uint32_t crosstalk_re_repl_thread_cmd(crosstalk_re_repl_ctx_t *ctx, uint32_t argc, char **args)
{
    crosstalk_re_repl_thread_t *thread = 0x0;
    uint64_t val = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(ctx == 0x0 || ctx->thread == 0x0 || args == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(argc == 0 || argc > (uint32_t)THREAD_ARGC_X)) { return LDG_ERR_FUNC_ARG_INVALID; }

    thread = (crosstalk_re_repl_thread_t *)ctx->thread;

    if (argc == (uint32_t)THREAD_ARGC_X) { ret = crosstalk_re_repl_num_get(args[THREAD_ARG_X], &val); }

    if (ret != LDG_ERR_AOK || val > (uint64_t)UINT32_MAX) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (crosstalk_re_repl_cmd_eq_is(args[THREAD_ARG_VERB], THREAD_WORD_LIST) == LDG_TRUTH_TRUE) { return thread_list(ctx, thread, (uint32_t)val); }

    if (argc == (uint32_t)THREAD_ARGC_VERB) { return crosstalk_re_repl_emit(ctx, "thread selected: %u32; id: %u64; regs: %u32; err: %u32", thread->sel, thread->sel_id, (uint32_t)thread->regs_ok, thread->last_err); }

    ret = thread_ids_ld(ctx, thread);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (val >= (uint64_t)thread->id_cunt) { return LDG_ERR_RANGE; }

    thread->sel = (uint32_t)val;
    thread->sel_id = thread->id[thread->sel];
    thread->next_ns = 0;

    return crosstalk_re_repl_emit(ctx, "thread selected: %u32; id: %u64", thread->sel, thread->sel_id);
}
