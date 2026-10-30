#ifndef DANGEM_CORE_H
#define DANGEM_CORE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/io/path.h>
#include <dangling/term/input.h>
#include <dangling/term/sesh.h>

#include "compile/compile.h"
#include "ui/menu.h"

#define DANGEM_CORE_LOG_PATH "dangem.log"
#define DANGEM_CORE_FRAME_NS (16ULL * LDG_NS_PER_MS)
#define DANGEM_CORE_COL_MAX 512
#define DANGEM_CORE_ROW_MAX 256
#define DANGEM_CORE_SAMPLE_CUNT 1
#define DANGEM_CORE_EXIT_MASK 0xFFU
#define DANGEM_CORE_LDG_DIR "ldg/lib"
#define DANGEM_CORE_TITLE "dangem"

typedef enum dangem_core_screen
{
    DANGEM_CORE_SCREEN_MENU = 0,
    DANGEM_CORE_SCREEN_SUMMARY
} dangem_core_screen_t;

typedef struct dangem_core
{
    dangem_compile_t build;
    dangem_ui_menu_view_t view;
    char root[LDG_IO_PATH_MAX];
    ldg_term_sesh_t *sesh;
    ldg_term_input_t *input;
    uint64_t launch_confirm_ms;
    uint32_t screen;
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint8_t quit;
    uint8_t launch;
    uint8_t launch_after;
    uint8_t deps_ok;
    uint8_t launch_confirm;
    uint8_t pudding[7];
} dangem_core_t;

uint32_t dangem_core_create(dangem_core_t **out);
uint32_t dangem_core_run(dangem_core_t *core);
uint32_t dangem_core_launch(dangem_core_t *core, uint32_t *out_code);
uint32_t dangem_core_destroy(dangem_core_t *core);

#endif
