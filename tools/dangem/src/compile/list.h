#ifndef DANGEM_COMPILE_LIST_H
#define DANGEM_COMPILE_LIST_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define DANGEM_COMPILE_LIST_TEXT_CAP (512 * LDG_KIB)
#define DANGEM_COMPILE_LIST_ITEM_MAX 4096
#define DANGEM_COMPILE_LIST_OPEN_MODE 0
#define DANGEM_COMPILE_LIST_STEP 1
#define DANGEM_COMPILE_LIST_BUILD_IDX 0
#define DANGEM_COMPILE_LIST_DIR_LDG_INC "ldg/include"
#define DANGEM_COMPILE_LIST_DIR_LDG_LIB "ldg/lib"
#define DANGEM_COMPILE_LIST_DIR_STEAM "sdk/steamworks"

typedef enum dangem_compile_list_key
{
    DANGEM_COMPILE_LIST_KEY_ROOT = 0,
    DANGEM_COMPILE_LIST_KEY_BIN,
    DANGEM_COMPILE_LIST_KEY_TGT,
    DANGEM_COMPILE_LIST_KEY_STEAM,
    DANGEM_COMPILE_LIST_KEY_LDGINC,
    DANGEM_COMPILE_LIST_KEY_LDGLIB,
    DANGEM_COMPILE_LIST_KEY_OUT,
    DANGEM_COMPILE_LIST_KEY_STD,
    DANGEM_COMPILE_LIST_KEY_CFLAGSET,
    DANGEM_COMPILE_LIST_KEY_LDFLAGSET,
    DANGEM_COMPILE_LIST_KEY_LIBSET,
    DANGEM_COMPILE_LIST_KEY_CUNT
} dangem_compile_list_key_t;

typedef enum dangem_compile_list_kind
{
    DANGEM_COMPILE_LIST_KIND_SRC = 0,
    DANGEM_COMPILE_LIST_KIND_DEF,
    DANGEM_COMPILE_LIST_KIND_INC,
    DANGEM_COMPILE_LIST_KIND_CFLAG,
    DANGEM_COMPILE_LIST_KIND_LIB,
    DANGEM_COMPILE_LIST_KIND_LDFLAG,
    DANGEM_COMPILE_LIST_KIND_CUNT
} dangem_compile_list_kind_t;

typedef struct dangem_compile_list
{
    char text[DANGEM_COMPILE_LIST_TEXT_CAP];
    uint64_t text_len;
    uint32_t item_off[DANGEM_COMPILE_LIST_ITEM_MAX];
    uint8_t item_kind[DANGEM_COMPILE_LIST_ITEM_MAX];
    uint32_t key_off[DANGEM_COMPILE_LIST_KEY_CUNT];
    uint32_t item_cunt;
    uint32_t src_cunt;
    uint8_t pudding[4];
} dangem_compile_list_t;

uint32_t dangem_compile_list_ld(dangem_compile_list_t *list, const char *path);
uint32_t dangem_compile_list_key_get(const dangem_compile_list_t *list, uint32_t key, const char **out);
uint32_t dangem_compile_list_map(const dangem_compile_list_t *list, const char *root, const char *src, char *dst, uint64_t cap);

#endif
