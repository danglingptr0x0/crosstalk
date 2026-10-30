#ifndef CROSSTALK_HDL_CC_H
#define CROSSTALK_HDL_CC_H

#include <stdint.h>

#include <dangling/core/types.h>
#include <dangling/io/path.h>

#define CROSSTALK_HDL_ERR_CC_SPAWN 767
#define CROSSTALK_HDL_ERR_CC_EXIT 768
#define CROSSTALK_HDL_ERR_CC_MISSING 769
#define CROSSTALK_HDL_ERR_CC_ARG 770

#define CROSSTALK_HDL_CC_EXE "clang"
#define CROSSTALK_HDL_CC_EXE_WIN "clang.exe"
#define CROSSTALK_HDL_CC_TGT_WIN "--tar" "get=x86_64-pc-windows-msvc"
#define CROSSTALK_HDL_CC_TGT_UNIX "--tar" "get=x86_64-pc-linux-gnu"
#define CROSSTALK_HDL_CC_OPT "-O3"
#define CROSSTALK_HDL_CC_ARCH "-march=x86-64-v2"
#define CROSSTALK_HDL_CC_FREE "-ffreestanding"
#define CROSSTALK_HDL_CC_NOBUILTIN "-fno-builtin"
#define CROSSTALK_HDL_CC_NOLIB "-nostdlib"
#define CROSSTALK_HDL_CC_SHARED "-shared"
#define CROSSTALK_HDL_CC_LD "-fuse-ld=lld"
#define CROSSTALK_HDL_CC_NOENTRY "-Wl,-noentry"
#define CROSSTALK_HDL_CC_NOPROBE "-mno-stack-arg-probe"
#define CROSSTALK_HDL_CC_PIC "-fPIC"
#define CROSSTALK_HDL_CC_OUT_FLAG "-o"

#define CROSSTALK_HDL_CC_ARG_MAX 16
#define CROSSTALK_HDL_CC_LINE_MUL 4
#define CROSSTALK_HDL_CC_WAIT_FLAGS 0
#define CROSSTALK_HDL_CC_REV "24"
#define CROSSTALK_HDL_CC_STEP 1
#define CROSSTALK_HDL_CC_EXIT_OK 0

typedef struct crosstalk_hdl_cc_ctx
{
    char exe[LDG_IO_PATH_MAX];
    uint64_t runs;
    uint64_t fails;
    uint32_t last_exit;
    uint8_t ok;
    uint8_t pudding[3];
} crosstalk_hdl_cc_ctx_t;

uint32_t crosstalk_hdl_cc_init(crosstalk_hdl_cc_ctx_t *ctx, const char *dir);
uint32_t crosstalk_hdl_cc_shutdown(crosstalk_hdl_cc_ctx_t *ctx);
uint32_t crosstalk_hdl_cc_run(crosstalk_hdl_cc_ctx_t *ctx, const char *src_path, const char *out_path);
uint8_t crosstalk_hdl_cc_ready_is(const crosstalk_hdl_cc_ctx_t *ctx);

#endif
