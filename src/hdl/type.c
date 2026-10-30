#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/io/file.h>

#include "core/log.h"
#include "hdl/type.h"

#define TYPE_FILE_MODE 0644
#define TYPE_IDENT_LEN 16
#define TYPE_MAG0 0x7F
#define TYPE_MAG1 'E'
#define TYPE_MAG2 'L'
#define TYPE_MAG3 'F'
#define TYPE_CLASS_AT 4
#define TYPE_DATA_AT 5
#define TYPE_CLASS32 1
#define TYPE_CLASS64 2
#define TYPE_DATA_LSB 1
#define TYPE_HDR32_LEN 52
#define TYPE_HDR64_LEN 64
#define TYPE_SHOFF32_AT 32
#define TYPE_SHENTSIZE32_AT 46
#define TYPE_SHNUM32_AT 48
#define TYPE_SHSTRNDX32_AT 50
#define TYPE_SHOFF64_AT 40
#define TYPE_SHENTSIZE64_AT 58
#define TYPE_SHNUM64_AT 60
#define TYPE_SHSTRNDX64_AT 62
#define TYPE_SHENT32_LEN 40
#define TYPE_SHENT64_LEN 64
#define TYPE_SH_NAME_AT 0
#define TYPE_SH_OFF32_AT 16
#define TYPE_SH_SIZE32_AT 20
#define TYPE_SH_OFF64_AT 24
#define TYPE_SH_SIZE64_AT 32
#define TYPE_SHNUM_MAX 65535
#define TYPE_SECT_BYTES_MAX (256 * LDG_MIB)
#define TYPE_POOL_SLACK 64
#define TYPE_POOL_ITEM_VAR 0
#define TYPE_HDR_AT 0
#define TYPE_HEAP_ROOT_AT 0

#define TYPE_SECT_INFO ".debug_info"
#define TYPE_SECT_ABBREV ".debug_abbrev"
#define TYPE_SECT_STR ".debug_str"
#define TYPE_SECT_LINE_STR ".debug_line_str"
#define TYPE_SECT_STR_OFF ".debug_str_offsets"
#define TYPE_SECT_ADDR ".debug_addr"

#define TYPE_TAG_ARR 0x01
#define TYPE_TAG_ENUM 0x04
#define TYPE_TAG_MEMBER 0x0D
#define TYPE_TAG_PTR 0x0F
#define TYPE_TAG_CU 0x11
#define TYPE_TAG_STRUCT 0x13
#define TYPE_TAG_SUBROUTINE 0x15
#define TYPE_TAG_TYPEDEF 0x16
#define TYPE_TAG_UNION 0x17
#define TYPE_TAG_SUBRANGE 0x21
#define TYPE_TAG_BASE 0x24
#define TYPE_TAG_CONST 0x26
#define TYPE_TAG_SUBPROGRAM 0x2E
#define TYPE_TAG_VAR 0x34
#define TYPE_TAG_VOLATILE 0x35
#define TYPE_TAG_RESTRICT 0x37

#define TYPE_AT_LOCATION 0x02
#define TYPE_AT_NAME 0x03
#define TYPE_AT_BYTE_SIZE 0x0B
#define TYPE_AT_UPPER_BOUND 0x2F
#define TYPE_AT_COUNT 0x37
#define TYPE_AT_MEMBER_LOC 0x38
#define TYPE_AT_DECLARATION 0x3C
#define TYPE_AT_ENCODING 0x3E
#define TYPE_AT_TYPE 0x49
#define TYPE_AT_STR_OFF_BASE 0x72
#define TYPE_AT_ADDR_BASE 0x73

#define TYPE_FORM_ADDR 0x01
#define TYPE_FORM_BLOCK2 0x03
#define TYPE_FORM_BLOCK4 0x04
#define TYPE_FORM_DATA2 0x05
#define TYPE_FORM_DATA4 0x06
#define TYPE_FORM_DATA8 0x07
#define TYPE_FORM_STRING 0x08
#define TYPE_FORM_BLOCK 0x09
#define TYPE_FORM_BLOCK1 0x0A
#define TYPE_FORM_DATA1 0x0B
#define TYPE_FORM_FLAG 0x0C
#define TYPE_FORM_SDATA 0x0D
#define TYPE_FORM_STRP 0x0E
#define TYPE_FORM_UDATA 0x0F
#define TYPE_FORM_REF_ADDR 0x10
#define TYPE_FORM_REF1 0x11
#define TYPE_FORM_REF2 0x12
#define TYPE_FORM_REF4 0x13
#define TYPE_FORM_REF8 0x14
#define TYPE_FORM_REF_UDATA 0x15
#define TYPE_FORM_INDIRECT 0x16
#define TYPE_FORM_SEC_OFF 0x17
#define TYPE_FORM_EXPRLOC 0x18
#define TYPE_FORM_FLAG_PRESENT 0x19
#define TYPE_FORM_STRX 0x1A
#define TYPE_FORM_ADDRX 0x1B
#define TYPE_FORM_REF_SUP4 0x1C
#define TYPE_FORM_STRP_SUP 0x1D
#define TYPE_FORM_DATA16 0x1E
#define TYPE_FORM_LINE_STRP 0x1F
#define TYPE_FORM_REF_SIG8 0x20
#define TYPE_FORM_IMPLICIT_CONST 0x21
#define TYPE_FORM_LOCLISTX 0x22
#define TYPE_FORM_RNGLISTX 0x23
#define TYPE_FORM_REF_SUP8 0x24
#define TYPE_FORM_STRX1 0x25
#define TYPE_FORM_STRX2 0x26
#define TYPE_FORM_STRX3 0x27
#define TYPE_FORM_STRX4 0x28
#define TYPE_FORM_ADDRX1 0x29
#define TYPE_FORM_ADDRX2 0x2A
#define TYPE_FORM_ADDRX3 0x2B
#define TYPE_FORM_ADDRX4 0x2C

#define TYPE_ATE_BOOL 0x02
#define TYPE_ATE_SIGNED 0x05
#define TYPE_ATE_SIGNED_CHAR 0x06
#define TYPE_ATE_UNSIGNED 0x07
#define TYPE_ATE_UNSIGNED_CHAR 0x08

#define TYPE_OP_ADDR 0x03
#define TYPE_OP_ADDRX 0xA1

#define TYPE_VER_MIN 2
#define TYPE_VER_MAX 5
#define TYPE_VER_HDR_SPLIT 5
#define TYPE_UNIT_HDR_LEN_LEN 4
#define TYPE_STR_OFF_HDR_LEN 8
#define TYPE_ADDR_HDR_LEN 8
#define TYPE_DIE_MAX 1048576
#define TYPE_STACK_MAX 128
#define TYPE_RESOLVE_PASS_MAX 8
#define TYPE_ULEB_SHIFT 7
#define TYPE_ULEB_MASK 0x7F
#define TYPE_ULEB_MORE 0x80
#define TYPE_LEB_MAX_BYTES 10
#define TYPE_IDX_TEXT_CAP 24
#define TYPE_IDX_FMT "%c%" PRIu64 "%c"
#define TYPE_PATH_SEP_LEN 1
#define TYPE_DIE_REF_ABSENT 0
#define TYPE_SIZE_PENDING 0

typedef struct type_sect
{
    const uint8_t *at;
    uint64_t len;
} type_sect_t;

typedef struct type_parse
{
    type_sect_t info;
    type_sect_t abbrev;
    type_sect_t str;
    type_sect_t line_str;
    type_sect_t str_off;
    type_sect_t addr;
    uint64_t cu_off;
    uint64_t str_off_base;
    uint64_t addr_base;
    uint32_t ver;
    uint8_t addr_size;
    uint8_t pudding[3];
} type_parse_t;

typedef struct type_val
{
    uint64_t num;
    const uint8_t *blk;
    const char *str;
    uint32_t form;
    uint8_t str_idx_is;
    uint8_t addr_idx_is;
    uint8_t pudding[2];
} type_val_t;

typedef struct type_build
{
    crosstalk_hdl_type_rec_t *type;
    crosstalk_hdl_type_member_t *member;
    crosstalk_hdl_type_var_t *var;
    uint64_t *die_off;
    uint32_t *die_idx;
    uint64_t *cv_off;
    uint64_t *cv_ref;
    uint32_t type_cunt;
    uint32_t member_cunt;
    uint32_t var_cunt;
    uint32_t die_cunt;
    uint32_t cv_cunt;
} type_build_t;

static uint16_t type_u16_get(const uint8_t *at)
{
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[0x1] << LDG_BYTE_BITS));
}

static uint32_t type_u32_get(const uint8_t *at)
{
    return (uint32_t)at[0] | ((uint32_t)at[0x1] << 8) | ((uint32_t)at[0x2] << 16) | ((uint32_t)at[0x3] << 24);
}

static uint64_t type_u64_get(const uint8_t *at)
{
    return (uint64_t)type_u32_get(at) | ((uint64_t)type_u32_get(at + 0x4) << 32);
}

static uint32_t type_rd_at(ldg_io_file_t *file, uint64_t off, void *dst, uint64_t len)
{
    uint64_t bytes_rd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(file == 0x0 || dst == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    ret = ldg_io_file_seek(file, off, LDG_IO_SEEK_START);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_rd(file, dst, len, &bytes_rd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(bytes_rd != len)) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

static void type_name_cpy(char *dst, const char *src, uint32_t cap)
{
    uint32_t at = 0;

    if (src == 0x0) { dst[0] = LDG_STR_TERM; return; }

    while (at + LDG_STR_TERM_SIZE < cap && src[at] != LDG_STR_TERM)
    {
        dst[at] = src[at];
        at = at + 0x1;
    }

    dst[at] = LDG_STR_TERM;
}

static uint32_t type_uleb_rd(const uint8_t *buff, uint64_t len, uint64_t *at, uint64_t *out)
{
    uint64_t val = 0;
    uint32_t shift = 0;
    uint32_t steps = 0;

    if (LDG_UNLIKELY(buff == 0x0 || at == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    while (*at < len && steps < (uint32_t)TYPE_LEB_MAX_BYTES)
    {
        uint8_t byte = buff[*at];

        *at = *at + 0x1;
        steps = steps + 0x1;

        if (shift < (uint32_t)(LDG_BYTE_BITS * 8)) { val = val | ((uint64_t)(byte & (uint8_t)TYPE_ULEB_MASK) << shift); }

        shift = shift + (uint32_t)TYPE_ULEB_SHIFT;

        if ((byte & (uint8_t)TYPE_ULEB_MORE) == 0) { *out = val; return LDG_ERR_AOK; }
    }

    return LDG_ERR_IO_FMT;
}

static uint32_t type_sleb_rd(const uint8_t *buff, uint64_t len, uint64_t *at, uint64_t *out)
{
    uint64_t val = 0;
    uint32_t shift = 0;
    uint32_t steps = 0;

    if (LDG_UNLIKELY(buff == 0x0 || at == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    while (*at < len && steps < (uint32_t)TYPE_LEB_MAX_BYTES)
    {
        uint8_t byte = buff[*at];

        *at = *at + 0x1;
        steps = steps + 0x1;

        if (shift < (uint32_t)(LDG_BYTE_BITS * 8)) { val = val | ((uint64_t)(byte & (uint8_t)TYPE_ULEB_MASK) << shift); }

        shift = shift + (uint32_t)TYPE_ULEB_SHIFT;

        if ((byte & (uint8_t)TYPE_ULEB_MORE) == 0)
        {
            if (shift < (uint32_t)(LDG_BYTE_BITS * 8) && (byte & 0x40) != 0) { val = val | (UINT64_MAX << shift); }

            *out = val;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_IO_FMT;
}

static uint32_t type_sect_find(const uint8_t *sect, uint32_t shnum, uint64_t ent_len, uint8_t class_is64, const char *shstr, uint64_t shstr_len, const char *name, uint64_t *out_off, uint64_t *out_len)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(sect == 0x0 || shstr == 0x0 || name == 0x0 || out_off == 0x0 || out_len == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_off = 0;
    *out_len = 0;

    for (at = 0; at < shnum; at = at + 0x1)
    {
        const uint8_t *ent = sect + (uint64_t)at * ent_len;
        uint32_t name_off = type_u32_get(ent + TYPE_SH_NAME_AT);

        if ((uint64_t)name_off >= shstr_len) { continue; }

        if (strncmp(shstr + name_off, name, shstr_len - (uint64_t)name_off) != 0) { continue; }

        if (class_is64 == LDG_TRUTH_TRUE)
        {
            *out_off = type_u64_get(ent + TYPE_SH_OFF64_AT);
            *out_len = type_u64_get(ent + TYPE_SH_SIZE64_AT);
        }
        else
        {
            *out_off = (uint64_t)type_u32_get(ent + TYPE_SH_OFF32_AT);
            *out_len = (uint64_t)type_u32_get(ent + TYPE_SH_SIZE32_AT);
        }

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t type_str_at_get(const type_sect_t *sect, uint64_t off, const char **out)
{
    if (LDG_UNLIKELY(sect == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (sect->at == 0x0 || off >= sect->len) { return LDG_ERR_BOUNDS; }

    *out = (const char *)sect->at + off;

    return LDG_ERR_AOK;
}

static uint32_t type_strx_get(const type_parse_t *parse, uint64_t idx, const char **out)
{
    uint64_t slot = 0;
    uint64_t off = 0;

    if (LDG_UNLIKELY(parse == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (parse->str_off.at == 0x0) { return LDG_ERR_NOT_FOUND; }

    slot = parse->str_off_base + idx * (uint64_t)TYPE_UNIT_HDR_LEN_LEN;

    if (slot + (uint64_t)TYPE_UNIT_HDR_LEN_LEN > parse->str_off.len) { return LDG_ERR_BOUNDS; }

    off = (uint64_t)type_u32_get(parse->str_off.at + slot);

    return type_str_at_get(&parse->str, off, out);
}

static uint32_t type_addrx_get(const type_parse_t *parse, uint64_t idx, uint64_t *out)
{
    uint64_t slot = 0;

    if (LDG_UNLIKELY(parse == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (parse->addr.at == 0x0) { return LDG_ERR_NOT_FOUND; }

    slot = parse->addr_base + idx * (uint64_t)parse->addr_size;

    if (slot + (uint64_t)parse->addr_size > parse->addr.len) { return LDG_ERR_BOUNDS; }

    if (parse->addr_size == sizeof(uint64_t)) { *out = type_u64_get(parse->addr.at + slot); }
    else { *out = (uint64_t)type_u32_get(parse->addr.at + slot); }

    return LDG_ERR_AOK;
}

static uint32_t type_form_rd(const type_parse_t *parse, uint64_t *at, uint32_t form, uint64_t implicit, type_val_t *out)
{
    const uint8_t *info = 0x0;
    uint64_t len = 0;
    uint64_t blk_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(parse == 0x0 || at == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));
    out->form = form;

    info = parse->info.at;
    len = parse->info.len;

    if (form == (uint32_t)TYPE_FORM_INDIRECT)
    {
        uint64_t inner = 0;

        ret = type_uleb_rd(info, len, at, &inner);
        if (ret != LDG_ERR_AOK) { return ret; }

        return type_form_rd(parse, at, (uint32_t)inner, implicit, out);
    }

    if (form == (uint32_t)TYPE_FORM_IMPLICIT_CONST) { out->num = implicit; return LDG_ERR_AOK; }

    if (form == (uint32_t)TYPE_FORM_FLAG_PRESENT) { out->num = 0x1; return LDG_ERR_AOK; }

    if (form == (uint32_t)TYPE_FORM_ADDR)
    {
        if (*at + (uint64_t)parse->addr_size > len) { return LDG_ERR_BOUNDS; }

        if (parse->addr_size == sizeof(uint64_t)) { out->num = type_u64_get(info + *at); }
        else { out->num = (uint64_t)type_u32_get(info + *at); }

        *at = *at + (uint64_t)parse->addr_size;

        return LDG_ERR_AOK;
    }

    if (form == (uint32_t)TYPE_FORM_DATA1 || form == (uint32_t)TYPE_FORM_REF1 || form == (uint32_t)TYPE_FORM_FLAG || form == (uint32_t)TYPE_FORM_STRX1 || form == (uint32_t)TYPE_FORM_ADDRX1)
    {
        if (*at + 0x1 > len) { return LDG_ERR_BOUNDS; }

        out->num = (uint64_t)info[*at];
        *at = *at + 0x1;
    }
    else if (form == (uint32_t)TYPE_FORM_DATA2 || form == (uint32_t)TYPE_FORM_REF2 || form == (uint32_t)TYPE_FORM_STRX2 || form == (uint32_t)TYPE_FORM_ADDRX2)
    {
        if (*at + 0x2 > len) { return LDG_ERR_BOUNDS; }

        out->num = (uint64_t)type_u16_get(info + *at);
        *at = *at + 0x2;
    }
    else if (form == (uint32_t)TYPE_FORM_STRX3 || form == (uint32_t)TYPE_FORM_ADDRX3)
    {
        if (*at + 0x3 > len) { return LDG_ERR_BOUNDS; }

        out->num = (uint64_t)info[*at] | ((uint64_t)info[*at + 0x1] << 8) | ((uint64_t)info[*at + 0x2] << 16);
        *at = *at + 0x3;
    }
    else if (form == (uint32_t)TYPE_FORM_DATA4 || form == (uint32_t)TYPE_FORM_REF4 || form == (uint32_t)TYPE_FORM_REF_SUP4 || form == (uint32_t)TYPE_FORM_STRX4 || form == (uint32_t)TYPE_FORM_ADDRX4)
    {
        if (*at + 0x4 > len) { return LDG_ERR_BOUNDS; }

        out->num = (uint64_t)type_u32_get(info + *at);
        *at = *at + 0x4;
    }
    else if (form == (uint32_t)TYPE_FORM_DATA8 || form == (uint32_t)TYPE_FORM_REF8 || form == (uint32_t)TYPE_FORM_REF_SIG8 || form == (uint32_t)TYPE_FORM_REF_SUP8)
    {
        if (*at + 0x8 > len) { return LDG_ERR_BOUNDS; }

        out->num = type_u64_get(info + *at);
        *at = *at + 0x8;
    }
    else if (form == (uint32_t)TYPE_FORM_DATA16)
    {
        if (*at + 0x10 > len) { return LDG_ERR_BOUNDS; }

        *at = *at + 0x10;
    }
    else if (form == (uint32_t)TYPE_FORM_SEC_OFF || form == (uint32_t)TYPE_FORM_STRP || form == (uint32_t)TYPE_FORM_LINE_STRP || form == (uint32_t)TYPE_FORM_STRP_SUP || form == (uint32_t)TYPE_FORM_REF_ADDR)
    {
        if (*at + 0x4 > len) { return LDG_ERR_BOUNDS; }

        out->num = (uint64_t)type_u32_get(info + *at);
        *at = *at + 0x4;
    }
    else if (form == (uint32_t)TYPE_FORM_UDATA || form == (uint32_t)TYPE_FORM_REF_UDATA || form == (uint32_t)TYPE_FORM_STRX || form == (uint32_t)TYPE_FORM_ADDRX || form == (uint32_t)TYPE_FORM_LOCLISTX || form == (uint32_t)TYPE_FORM_RNGLISTX)
    {
        ret = type_uleb_rd(info, len, at, &out->num);
        if (ret != LDG_ERR_AOK) { return ret; }
    }
    else if (form == (uint32_t)TYPE_FORM_SDATA)
    {
        ret = type_sleb_rd(info, len, at, &out->num);
        if (ret != LDG_ERR_AOK) { return ret; }
    }
    else if (form == (uint32_t)TYPE_FORM_STRING)
    {
        if (*at >= len) { return LDG_ERR_BOUNDS; }

        out->str = (const char *)info + *at;

        while (*at < len && info[*at] != LDG_STR_TERM) { *at = *at + 0x1; }

        if (*at >= len) { return LDG_ERR_BOUNDS; }

        *at = *at + 0x1;

        return LDG_ERR_AOK;
    }
    else if (form == (uint32_t)TYPE_FORM_BLOCK1 || form == (uint32_t)TYPE_FORM_BLOCK2 || form == (uint32_t)TYPE_FORM_BLOCK4 || form == (uint32_t)TYPE_FORM_BLOCK || form == (uint32_t)TYPE_FORM_EXPRLOC)
    {
        if (form == (uint32_t)TYPE_FORM_BLOCK1)
        {
            if (*at + 0x1 > len) { return LDG_ERR_BOUNDS; }

            blk_len = (uint64_t)info[*at];
            *at = *at + 0x1;
        }
        else if (form == (uint32_t)TYPE_FORM_BLOCK2)
        {
            if (*at + 0x2 > len) { return LDG_ERR_BOUNDS; }

            blk_len = (uint64_t)type_u16_get(info + *at);
            *at = *at + 0x2;
        }
        else if (form == (uint32_t)TYPE_FORM_BLOCK4)
        {
            if (*at + 0x4 > len) { return LDG_ERR_BOUNDS; }

            blk_len = (uint64_t)type_u32_get(info + *at);
            *at = *at + 0x4;
        }
        else
        {
            ret = type_uleb_rd(info, len, at, &blk_len);
            if (ret != LDG_ERR_AOK) { return ret; }
        }

        if (*at + blk_len > len) { return LDG_ERR_BOUNDS; }

        out->blk = info + *at;
        out->num = blk_len;
        *at = *at + blk_len;

        return LDG_ERR_AOK;
    }
    else { return LDG_ERR_UNSUPPORTED; }

    if (form == (uint32_t)TYPE_FORM_STRP || form == (uint32_t)TYPE_FORM_STRP_SUP)
    {
        ret = type_str_at_get(&parse->str, out->num, &out->str);
        if (ret != LDG_ERR_AOK) { out->str = 0x0; }
    }
    else if (form == (uint32_t)TYPE_FORM_LINE_STRP)
    {
        ret = type_str_at_get(&parse->line_str, out->num, &out->str);
        if (ret != LDG_ERR_AOK) { out->str = 0x0; }
    }
    else if (form == (uint32_t)TYPE_FORM_STRX || form == (uint32_t)TYPE_FORM_STRX1 || form == (uint32_t)TYPE_FORM_STRX2 || form == (uint32_t)TYPE_FORM_STRX3 || form == (uint32_t)TYPE_FORM_STRX4) { out->str_idx_is = LDG_TRUTH_TRUE; }
    else if (form == (uint32_t)TYPE_FORM_ADDRX || form == (uint32_t)TYPE_FORM_ADDRX1 || form == (uint32_t)TYPE_FORM_ADDRX2 || form == (uint32_t)TYPE_FORM_ADDRX3 || form == (uint32_t)TYPE_FORM_ADDRX4) { out->addr_idx_is = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

static uint32_t type_ref_abs_get(const type_parse_t *parse, const type_val_t *val, uint64_t *out)
{
    if (LDG_UNLIKELY(parse == 0x0 || val == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (val->form == (uint32_t)TYPE_FORM_REF_ADDR) { *out = val->num; return LDG_ERR_AOK; }

    *out = parse->cu_off + val->num;

    return LDG_ERR_AOK;
}

static uint32_t type_abbrev_find(const type_sect_t *abbrev, uint64_t base, uint64_t code, uint64_t *out_at, uint64_t *out_tag, uint8_t *out_kids)
{
    uint64_t at = base;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(abbrev == 0x0 || out_at == 0x0 || out_tag == 0x0 || out_kids == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_at = 0;
    *out_tag = 0;
    *out_kids = LDG_TRUTH_FALSE;

    if (abbrev->at == 0x0 || base >= abbrev->len) { return LDG_ERR_BOUNDS; }

    while (at < abbrev->len)
    {
        uint64_t ent_code = 0;
        uint64_t tag = 0;
        uint8_t kids = 0;

        ret = type_uleb_rd(abbrev->at, abbrev->len, &at, &ent_code);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (ent_code == 0) { return LDG_ERR_NOT_FOUND; }

        ret = type_uleb_rd(abbrev->at, abbrev->len, &at, &tag);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (at >= abbrev->len) { return LDG_ERR_BOUNDS; }

        kids = abbrev->at[at];
        at = at + 0x1;

        if (ent_code == code)
        {
            *out_at = at;
            *out_tag = tag;
            *out_kids = kids != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

            return LDG_ERR_AOK;
        }

        while (at < abbrev->len)
        {
            uint64_t attr = 0;
            uint64_t form = 0;
            uint64_t skip = 0;

            ret = type_uleb_rd(abbrev->at, abbrev->len, &at, &attr);
            if (ret != LDG_ERR_AOK) { return ret; }

            ret = type_uleb_rd(abbrev->at, abbrev->len, &at, &form);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (form == (uint64_t)TYPE_FORM_IMPLICIT_CONST)
            {
                ret = type_sleb_rd(abbrev->at, abbrev->len, &at, &skip);
                if (ret != LDG_ERR_AOK) { return ret; }
            }

            if (attr == 0 && form == 0) { break; }
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t type_die_map_add(type_build_t *build, uint64_t die_off, uint32_t idx)
{
    if (LDG_UNLIKELY(build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (build->die_cunt >= (uint32_t)TYPE_DIE_MAX) { return LDG_ERR_FULL; }

    build->die_off[build->die_cunt] = die_off;
    build->die_idx[build->die_cunt] = idx;
    build->die_cunt = build->die_cunt + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t type_die_map_find(const type_build_t *build, uint64_t die_off, uint32_t *out)
{
    uint32_t lo = 0;
    uint32_t hi = 0;

    if (LDG_UNLIKELY(build == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    hi = build->die_cunt;

    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 0x2;

        if (build->die_off[mid] == die_off)
        {
            *out = build->die_idx[mid];

            return LDG_ERR_AOK;
        }

        if (build->die_off[mid] < die_off) { lo = mid + 0x1; }
        else { hi = mid; }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t type_cv_add(type_build_t *build, uint64_t die_off, uint64_t ref)
{
    if (LDG_UNLIKELY(build == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (build->cv_cunt >= (uint32_t)TYPE_DIE_MAX) { return LDG_ERR_FULL; }

    build->cv_off[build->cv_cunt] = die_off;
    build->cv_ref[build->cv_cunt] = ref;
    build->cv_cunt = build->cv_cunt + 0x1;

    return LDG_ERR_AOK;
}

static uint64_t type_cv_chase(const type_build_t *build, uint64_t die_off)
{
    uint64_t at = die_off;
    uint32_t depth = 0;

    while (depth < (uint32_t)CROSSTALK_HDL_TYPE_DEPTH_MAX)
    {
        uint32_t lo = 0;
        uint32_t hi = build->cv_cunt;
        uint8_t hit = LDG_TRUTH_FALSE;

        while (lo < hi)
        {
            uint32_t mid = lo + (hi - lo) / 0x2;

            if (build->cv_off[mid] == at)
            {
                at = build->cv_ref[mid];
                hit = LDG_TRUTH_TRUE;

                break;
            }

            if (build->cv_off[mid] < at) { lo = mid + 0x1; }
            else { hi = mid; }
        }

        if (hit != LDG_TRUTH_TRUE) { return at; }

        depth = depth + 0x1;
    }

    return at;
}

static uint32_t type_named_find(const type_build_t *build, const char *name, const char *alias, uint64_t size, uint32_t kind, uint32_t *out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(build == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    for (at = 0; at < build->type_cunt; at = at + 0x1)
    {
        if (build->type[at].kind != kind) { continue; }

        if (kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE && build->type[at].size != size && alias[0] == LDG_STR_TERM) { continue; }

        if (strncmp(build->type[at].name, name, (uint64_t)CROSSTALK_HDL_TYPE_NAME_CAP) != 0) { continue; }

        if (strncmp(build->type[at].alias, alias, (uint64_t)CROSSTALK_HDL_TYPE_NAME_CAP) != 0) { continue; }

        *out = at;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t type_rec_add(type_build_t *build, const char *name, const char *alias, uint64_t size, uint32_t kind, uint32_t *out)
{
    crosstalk_hdl_type_rec_t *rec = 0x0;

    if (LDG_UNLIKELY(build == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (build->type_cunt >= (uint32_t)CROSSTALK_HDL_TYPE_MAX) { return LDG_ERR_FULL; }

    rec = &build->type[build->type_cunt];

    memset(rec, 0x0, sizeof(*rec));

    type_name_cpy(rec->name, name, (uint32_t)CROSSTALK_HDL_TYPE_NAME_CAP);
    type_name_cpy(rec->alias, alias, (uint32_t)CROSSTALK_HDL_TYPE_NAME_CAP);

    rec->size = size;
    rec->kind = kind;
    rec->ref = (uint32_t)UINT32_MAX;
    rec->elem_cunt = 0;
    rec->member_at = build->member_cunt;
    rec->member_cunt = 0;

    *out = build->type_cunt;
    build->type_cunt = build->type_cunt + 0x1;

    return LDG_ERR_AOK;
}

static uint32_t type_kind_from_encoding(uint64_t encoding, uint64_t size)
{
    if (encoding == (uint64_t)TYPE_ATE_BOOL) { return (uint32_t)CROSSTALK_HDL_TYPE_KIND_BOOL; }

    if (encoding == (uint64_t)TYPE_ATE_SIGNED_CHAR || encoding == (uint64_t)TYPE_ATE_UNSIGNED_CHAR) { return (uint32_t)CROSSTALK_HDL_TYPE_KIND_CHAR; }

    if (encoding == (uint64_t)TYPE_ATE_SIGNED) { return (uint32_t)CROSSTALK_HDL_TYPE_KIND_INT; }

    if (encoding == (uint64_t)TYPE_ATE_UNSIGNED) { return (uint32_t)CROSSTALK_HDL_TYPE_KIND_UINT; }

    if (size == 0) { return (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE; }

    return (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE;
}

static uint8_t type_member_ahead_is(const crosstalk_hdl_type_member_t *lhs, const crosstalk_hdl_type_member_t *rhs)
{
    if (lhs->owner != rhs->owner) { return lhs->owner > rhs->owner ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return lhs->off > rhs->off ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void type_member_regroup(type_build_t *build)
{
    uint32_t at = 0;

    for (at = 0x1; at < build->member_cunt; at = at + 0x1)
    {
        crosstalk_hdl_type_member_t tmp = LDG_STRUCT_ZERO_INIT;
        uint32_t back = at;

        memcpy(&tmp, &build->member[at], sizeof(tmp));

        while (back > 0 && type_member_ahead_is(&build->member[back - 0x1], &tmp) == LDG_TRUTH_TRUE)
        {
            memcpy(&build->member[back], &build->member[back - 0x1], sizeof(tmp));
            back = back - 0x1;
        }

        memcpy(&build->member[back], &tmp, sizeof(tmp));
    }

    for (at = 0; at < build->type_cunt; at = at + 0x1)
    {
        build->type[at].member_at = 0;
        build->type[at].member_cunt = 0;
    }

    for (at = 0; at < build->member_cunt; at = at + 0x1)
    {
        uint32_t owner = build->member[at].owner;

        if (owner >= build->type_cunt) { continue; }

        if (build->type[owner].member_cunt == 0) { build->type[owner].member_at = at; }

        build->type[owner].member_cunt = build->type[owner].member_cunt + 0x1;
    }
}

static void type_var_swap(crosstalk_hdl_type_var_t *lhs, crosstalk_hdl_type_var_t *rhs)
{
    crosstalk_hdl_type_var_t tmp = LDG_STRUCT_ZERO_INIT;

    memcpy(&tmp, lhs, sizeof(tmp));
    memcpy(lhs, rhs, sizeof(tmp));
    memcpy(rhs, &tmp, sizeof(tmp));
}

static void type_var_sift(crosstalk_hdl_type_var_t *arr, uint32_t root, uint32_t cunt)
{
    uint32_t top = root;
    uint32_t kid = 0;

    while (top * 0x2 + 0x1 < cunt)
    {
        kid = top * 0x2 + 0x1;

        if (kid + 0x1 < cunt && arr[kid + 0x1].addr > arr[kid].addr) { kid = kid + 0x1; }

        if (arr[top].addr >= arr[kid].addr) { return; }

        type_var_swap(&arr[top], &arr[kid]);
        top = kid;
    }
}

static void type_var_sort(crosstalk_hdl_type_var_t *arr, uint32_t cunt)
{
    uint32_t at = 0;

    if (cunt < 0x2) { return; }

    at = cunt / 0x2;
    while (at > 0)
    {
        at = at - 0x1;
        type_var_sift(arr, at, cunt);
    }

    at = cunt;
    while (at > 0x1)
    {
        at = at - 0x1;
        type_var_swap(&arr[0], &arr[at]);
        type_var_sift(arr, (uint32_t)TYPE_HEAP_ROOT_AT, at);
    }
}

static uint32_t type_cu_walk(type_parse_t *parse, type_build_t *build, uint64_t cu_end, uint64_t abbrev_base, uint64_t *at)
{
    uint32_t owner[TYPE_STACK_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t depth = 0;
    uint32_t ret = 0;

    for (depth = 0; depth < (uint32_t)TYPE_STACK_MAX; depth = depth + 0x1) { owner[depth] = (uint32_t)UINT32_MAX; }

    depth = 0;

    while (*at < cu_end)
    {
        uint64_t die_off = *at;
        uint64_t code = 0;
        uint64_t spec_at = 0;
        uint64_t tag = 0;
        uint64_t name_idx = 0;
        uint64_t byte_size = 0;
        uint64_t encoding = 0;
        uint64_t member_loc = 0;
        uint64_t upper_bound = 0;
        uint64_t elem_cunt = 0;
        uint64_t type_ref = 0;
        uint64_t var_addr = 0;
        const char *name = 0x0;
        uint32_t idx = (uint32_t)UINT32_MAX;
        uint32_t kind = (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE;
        uint8_t kids = LDG_TRUTH_FALSE;
        uint8_t name_pending = LDG_TRUTH_FALSE;
        uint8_t type_has = LDG_TRUTH_FALSE;
        uint8_t loc_has = LDG_TRUTH_FALSE;
        uint8_t size_has = LDG_TRUTH_FALSE;
        uint8_t decl_only = LDG_TRUTH_FALSE;

        ret = type_uleb_rd(parse->info.at, parse->info.len, at, &code);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (code == 0)
        {
            if (depth == 0) { return LDG_ERR_AOK; }

            depth = depth - 0x1;

            continue;
        }

        ret = type_abbrev_find(&parse->abbrev, abbrev_base, code, &spec_at, &tag, &kids);
        if (ret != LDG_ERR_AOK) { return ret; }

        while (spec_at < parse->abbrev.len)
        {
            uint64_t attr = 0;
            uint64_t form = 0;
            uint64_t implicit = 0;
            type_val_t val = LDG_STRUCT_ZERO_INIT;

            ret = type_uleb_rd(parse->abbrev.at, parse->abbrev.len, &spec_at, &attr);
            if (ret != LDG_ERR_AOK) { return ret; }

            ret = type_uleb_rd(parse->abbrev.at, parse->abbrev.len, &spec_at, &form);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (form == (uint64_t)TYPE_FORM_IMPLICIT_CONST)
            {
                ret = type_sleb_rd(parse->abbrev.at, parse->abbrev.len, &spec_at, &implicit);
                if (ret != LDG_ERR_AOK) { return ret; }
            }

            if (attr == 0 && form == 0) { break; }

            ret = type_form_rd(parse, at, (uint32_t)form, implicit, &val);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (attr == (uint64_t)TYPE_AT_NAME) { if (val.str_idx_is == LDG_TRUTH_TRUE) { name_idx = val.num; name_pending = LDG_TRUTH_TRUE; }
                else { name = val.str; } }
            else if (attr == (uint64_t)TYPE_AT_BYTE_SIZE) { byte_size = val.num; size_has = LDG_TRUTH_TRUE; }
            else if (attr == (uint64_t)TYPE_AT_ENCODING) { encoding = val.num; }
            else if (attr == (uint64_t)TYPE_AT_MEMBER_LOC) { member_loc = val.num; }
            else if (attr == (uint64_t)TYPE_AT_UPPER_BOUND) { upper_bound = val.num + 0x1; }
            else if (attr == (uint64_t)TYPE_AT_COUNT) { elem_cunt = val.num; }
            else if (attr == (uint64_t)TYPE_AT_DECLARATION) { decl_only = val.num != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
            else if (attr == (uint64_t)TYPE_AT_STR_OFF_BASE) { parse->str_off_base = val.num; }
            else if (attr == (uint64_t)TYPE_AT_ADDR_BASE) { parse->addr_base = val.num; }
            else if (attr == (uint64_t)TYPE_AT_TYPE)
            {
                ret = type_ref_abs_get(parse, &val, &type_ref);
                if (ret != LDG_ERR_AOK) { return ret; }

                type_has = LDG_TRUTH_TRUE;
            }
            else if (attr == (uint64_t)TYPE_AT_LOCATION && val.blk != 0x0 && val.num > 0)
            {
                if (val.blk[0] == (uint8_t)TYPE_OP_ADDR && val.num >= (uint64_t)parse->addr_size + 0x1)
                {
                    if (parse->addr_size == sizeof(uint64_t)) { var_addr = type_u64_get(val.blk + 0x1); }
                    else { var_addr = (uint64_t)type_u32_get(val.blk + 0x1); }

                    loc_has = LDG_TRUTH_TRUE;
                }
                else if (val.blk[0] == (uint8_t)TYPE_OP_ADDRX && val.num >= 0x2)
                {
                    uint64_t blk_at = 0x1;
                    uint64_t addr_idx = 0;

                    if (type_uleb_rd(val.blk, val.num, &blk_at, &addr_idx) == LDG_ERR_AOK && type_addrx_get(parse, addr_idx, &var_addr) == LDG_ERR_AOK) { loc_has = LDG_TRUTH_TRUE; }
                }
            }
        }

        if (name_pending == LDG_TRUTH_TRUE && type_strx_get(parse, name_idx, &name) != LDG_ERR_AOK) { name = 0x0; }

        if (elem_cunt == 0) { elem_cunt = upper_bound; }

        if (tag == (uint64_t)TYPE_TAG_CONST || tag == (uint64_t)TYPE_TAG_VOLATILE || tag == (uint64_t)TYPE_TAG_RESTRICT)
        {
            ret = type_cv_add(build, die_off, type_has == LDG_TRUTH_TRUE ? type_ref : (uint64_t)TYPE_DIE_REF_ABSENT);
            if (ret != LDG_ERR_AOK) { return ret; }
        }
        else if (tag == (uint64_t)TYPE_TAG_BASE)
        {
            kind = type_kind_from_encoding(encoding, byte_size);

            ret = type_named_find(build, name != 0x0 ? name : "", "", byte_size, kind, &idx);
            if (ret != LDG_ERR_AOK)
            {
                ret = type_rec_add(build, name != 0x0 ? name : "", "", byte_size, kind, &idx);
                if (ret != LDG_ERR_AOK) { return ret; }
            }

            ret = type_die_map_add(build, die_off, idx);
            if (ret != LDG_ERR_AOK) { return ret; }
        }
        else if (tag == (uint64_t)TYPE_TAG_TYPEDEF)
        {
            ret = type_named_find(build, "", name != 0x0 ? name : "", (uint64_t)TYPE_SIZE_PENDING, (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE, &idx);
            if (ret != LDG_ERR_AOK)
            {
                ret = type_rec_add(build, "", name != 0x0 ? name : "", (uint64_t)TYPE_SIZE_PENDING, (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE, &idx);
                if (ret != LDG_ERR_AOK) { return ret; }

                build->type[idx].ref = (uint32_t)UINT32_MAX;
            }

            ret = type_die_map_add(build, die_off, idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (type_has == LDG_TRUTH_TRUE && build->type[idx].ref == (uint32_t)UINT32_MAX)
            {
                ret = type_cv_add(build, die_off + 0x1, type_ref);
                if (ret != LDG_ERR_AOK) { return ret; }

                build->type[idx].ref = (uint32_t)(die_off + 0x1);
            }
        }
        else if (tag == (uint64_t)TYPE_TAG_STRUCT || tag == (uint64_t)TYPE_TAG_UNION || tag == (uint64_t)TYPE_TAG_ENUM)
        {
            kind = tag == (uint64_t)TYPE_TAG_STRUCT ? (uint32_t)CROSSTALK_HDL_TYPE_KIND_STRUCT : (tag == (uint64_t)TYPE_TAG_UNION ? (uint32_t)CROSSTALK_HDL_TYPE_KIND_UNION : (uint32_t)CROSSTALK_HDL_TYPE_KIND_ENUM);

            if (decl_only == LDG_TRUTH_TRUE || name == 0x0 || name[0] == LDG_STR_TERM)
            {
                ret = type_rec_add(build, name != 0x0 ? name : "", "", byte_size, kind, &idx);
                if (ret != LDG_ERR_AOK) { return ret; }
            }
            else if (type_named_find(build, name, "", byte_size, kind, &idx) != LDG_ERR_AOK)
            {
                ret = type_rec_add(build, name, "", byte_size, kind, &idx);
                if (ret != LDG_ERR_AOK) { return ret; }
            }

            ret = type_die_map_add(build, die_off, idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (size_has == LDG_TRUTH_TRUE && build->type[idx].size == 0) { build->type[idx].size = byte_size; }

            if (kids == LDG_TRUTH_TRUE && depth + 0x1 < (uint32_t)TYPE_STACK_MAX) { owner[depth + 0x1] = build->type[idx].member_cunt == 0 ? idx : (uint32_t)UINT32_MAX; }
        }
        else if (tag == (uint64_t)TYPE_TAG_PTR)
        {
            ret = type_rec_add(build, "", "", (uint64_t)parse->addr_size, (uint32_t)CROSSTALK_HDL_TYPE_KIND_PTR, &idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            build->type[idx].ref = (uint32_t)UINT32_MAX;

            ret = type_die_map_add(build, die_off, idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (type_has == LDG_TRUTH_TRUE)
            {
                ret = type_cv_add(build, die_off + 0x1, type_ref);
                if (ret != LDG_ERR_AOK) { return ret; }

                build->type[idx].ref = (uint32_t)(die_off + 0x1);
            }
        }
        else if (tag == (uint64_t)TYPE_TAG_ARR)
        {
            ret = type_rec_add(build, "", "", byte_size, (uint32_t)CROSSTALK_HDL_TYPE_KIND_ARR, &idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            ret = type_die_map_add(build, die_off, idx);
            if (ret != LDG_ERR_AOK) { return ret; }

            if (type_has == LDG_TRUTH_TRUE)
            {
                ret = type_cv_add(build, die_off + 0x1, type_ref);
                if (ret != LDG_ERR_AOK) { return ret; }

                build->type[idx].ref = (uint32_t)(die_off + 0x1);
            }

            if (kids == LDG_TRUTH_TRUE && depth + 0x1 < (uint32_t)TYPE_STACK_MAX) { owner[depth + 0x1] = idx; }
        }
        else if (tag == (uint64_t)TYPE_TAG_SUBROUTINE || tag == (uint64_t)TYPE_TAG_SUBPROGRAM)
        {
            if (tag == (uint64_t)TYPE_TAG_SUBROUTINE)
            {
                ret = type_rec_add(build, name != 0x0 ? name : "", "", (uint64_t)TYPE_SIZE_PENDING, (uint32_t)CROSSTALK_HDL_TYPE_KIND_FUNC, &idx);
                if (ret != LDG_ERR_AOK) { return ret; }

                ret = type_die_map_add(build, die_off, idx);
                if (ret != LDG_ERR_AOK) { return ret; }
            }

            if (kids == LDG_TRUTH_TRUE && depth + 0x1 < (uint32_t)TYPE_STACK_MAX) { owner[depth + 0x1] = (uint32_t)UINT32_MAX; }
        }
        else if (tag == (uint64_t)TYPE_TAG_SUBRANGE) { if (depth > 0 && owner[depth] != (uint32_t)UINT32_MAX && build->type[owner[depth]].kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_ARR) { build->type[owner[depth]].elem_cunt = (uint32_t)elem_cunt; } }
        else if (tag == (uint64_t)TYPE_TAG_MEMBER)
        {
            if (depth > 0 && owner[depth] != (uint32_t)UINT32_MAX && build->member_cunt < (uint32_t)CROSSTALK_HDL_TYPE_MEMBER_MAX)
            {
                crosstalk_hdl_type_member_t *member = &build->member[build->member_cunt];

                memset(member, 0x0, sizeof(*member));

                type_name_cpy(member->name, name != 0x0 ? name : "", (uint32_t)CROSSTALK_HDL_TYPE_NAME_CAP);

                member->off = member_loc;
                member->size = 0;
                member->owner = owner[depth];
                member->type_idx = (uint32_t)UINT32_MAX;

                if (type_has == LDG_TRUTH_TRUE)
                {
                    ret = type_cv_add(build, die_off + 0x1, type_ref);
                    if (ret != LDG_ERR_AOK) { return ret; }

                    member->type_idx = (uint32_t)(die_off + 0x1);
                }

                build->type[owner[depth]].member_cunt = build->type[owner[depth]].member_cunt + 0x1;
                build->member_cunt = build->member_cunt + 0x1;
            }
        }
        else if (tag == (uint64_t)TYPE_TAG_VAR)
        {
            if (loc_has == LDG_TRUTH_TRUE && var_addr != 0 && type_has == LDG_TRUTH_TRUE && build->var_cunt < (uint32_t)CROSSTALK_HDL_TYPE_VAR_MAX)
            {
                crosstalk_hdl_type_var_t *var = &build->var[build->var_cunt];

                memset(var, 0x0, sizeof(*var));

                type_name_cpy(var->name, name != 0x0 ? name : "", (uint32_t)CROSSTALK_HDL_TYPE_NAME_CAP);

                var->addr = var_addr;
                var->size = 0;

                ret = type_cv_add(build, die_off + 0x1, type_ref);
                if (ret != LDG_ERR_AOK) { return ret; }

                var->type_idx = (uint32_t)(die_off + 0x1);

                build->var_cunt = build->var_cunt + 0x1;
            }
        }

        if (kids == LDG_TRUTH_TRUE)
        {
            depth = depth + 0x1;

            if (depth >= (uint32_t)TYPE_STACK_MAX) { return LDG_ERR_BOUNDS; }

            if (tag != (uint64_t)TYPE_TAG_STRUCT && tag != (uint64_t)TYPE_TAG_UNION && tag != (uint64_t)TYPE_TAG_ENUM && tag != (uint64_t)TYPE_TAG_ARR && tag != (uint64_t)TYPE_TAG_SUBROUTINE && tag != (uint64_t)TYPE_TAG_SUBPROGRAM) { owner[depth] = (uint32_t)UINT32_MAX; }
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t type_ref_resolve(const type_build_t *build, uint32_t raw, uint32_t *out)
{
    uint64_t die_off = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(build == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (raw == (uint32_t)UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    die_off = type_cv_chase(build, (uint64_t)raw);

    ret = type_die_map_find(build, die_off, &idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = idx;

    return LDG_ERR_AOK;
}

static void type_cv_sort(type_build_t *build)
{
    uint32_t at = 0;

    for (at = 0x1; at < build->cv_cunt; at = at + 0x1)
    {
        uint64_t off = build->cv_off[at];
        uint64_t ref = build->cv_ref[at];
        uint32_t back = at;

        while (back > 0 && build->cv_off[back - 0x1] > off)
        {
            build->cv_off[back] = build->cv_off[back - 0x1];
            build->cv_ref[back] = build->cv_ref[back - 0x1];
            back = back - 0x1;
        }

        build->cv_off[back] = off;
        build->cv_ref[back] = ref;
    }
}

static void type_die_sort(type_build_t *build)
{
    uint32_t at = 0;

    for (at = 0x1; at < build->die_cunt; at = at + 0x1)
    {
        uint64_t off = build->die_off[at];
        uint32_t idx = build->die_idx[at];
        uint32_t back = at;

        while (back > 0 && build->die_off[back - 0x1] > off)
        {
            build->die_off[back] = build->die_off[back - 0x1];
            build->die_idx[back] = build->die_idx[back - 0x1];
            back = back - 0x1;
        }

        build->die_off[back] = off;
        build->die_idx[back] = idx;
    }
}

static void type_resolve(type_build_t *build)
{
    uint32_t at = 0;
    uint32_t pass = 0;

    type_cv_sort(build);
    type_die_sort(build);

    for (at = 0; at < build->type_cunt; at = at + 0x1)
    {
        if (build->type[at].kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_ENUM || build->type[at].size != 0) { continue; }

        build->type[at].size = (uint64_t)sizeof(uint32_t);
    }

    for (at = 0; at < build->type_cunt; at = at + 0x1)
    {
        uint32_t idx = 0;

        if (build->type[at].ref == (uint32_t)UINT32_MAX) { continue; }

        if (type_ref_resolve(build, build->type[at].ref, &idx) != LDG_ERR_AOK) { build->type[at].ref = (uint32_t)UINT32_MAX; continue; }

        build->type[at].ref = idx;
    }

    for (at = 0; at < build->member_cunt; at = at + 0x1)
    {
        uint32_t idx = 0;

        if (build->member[at].type_idx == (uint32_t)UINT32_MAX) { continue; }

        if (type_ref_resolve(build, build->member[at].type_idx, &idx) != LDG_ERR_AOK) { build->member[at].type_idx = (uint32_t)UINT32_MAX; continue; }

        build->member[at].type_idx = idx;
    }

    for (at = 0; at < build->var_cunt; at = at + 0x1)
    {
        uint32_t idx = 0;

        if (build->var[at].type_idx == (uint32_t)UINT32_MAX) { continue; }

        if (type_ref_resolve(build, build->var[at].type_idx, &idx) != LDG_ERR_AOK) { build->var[at].type_idx = (uint32_t)UINT32_MAX; continue; }

        build->var[at].type_idx = idx;
    }

    for (pass = 0; pass < (uint32_t)TYPE_RESOLVE_PASS_MAX; pass = pass + 0x1)
    {
        for (at = 0; at < build->type_cunt; at = at + 0x1)
        {
            crosstalk_hdl_type_rec_t *rec = &build->type[at];

            if (rec->ref == (uint32_t)UINT32_MAX) { continue; }

            if (rec->kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_ARR)
            {
                if (rec->size == 0 && build->type[rec->ref].size != 0) { rec->size = build->type[rec->ref].size * (uint64_t)rec->elem_cunt; }

                continue;
            }

            if (rec->kind != (uint32_t)CROSSTALK_HDL_TYPE_KIND_NONE) { continue; }

            if (rec->alias[0] == LDG_STR_TERM) { continue; }

            rec->kind = build->type[rec->ref].kind;
            rec->size = build->type[rec->ref].size;
            rec->elem_cunt = build->type[rec->ref].elem_cunt;

            if (rec->name[0] == LDG_STR_TERM) { type_name_cpy(rec->name, build->type[rec->ref].name, (uint32_t)CROSSTALK_HDL_TYPE_NAME_CAP); }
        }
    }

    for (at = 0; at < build->member_cunt; at = at + 0x1)
    {
        if (build->member[at].type_idx == (uint32_t)UINT32_MAX) { continue; }

        build->member[at].size = build->type[build->member[at].type_idx].size;
    }

    type_member_regroup(build);

    for (at = 0; at < build->var_cunt; at = at + 0x1)
    {
        if (build->var[at].type_idx == (uint32_t)UINT32_MAX) { continue; }

        build->var[at].size = build->type[build->var[at].type_idx].size;
    }

    type_var_sort(build->var, build->var_cunt);
}

uint32_t crosstalk_hdl_type_init(crosstalk_hdl_type_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(ctx, 0x0, sizeof(*ctx));

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_clear(crosstalk_hdl_type_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->pool != 0x0) { ret = ldg_mem_pool_destroy(&ctx->pool); }

    ctx->pool = 0x0;
    ctx->type = 0x0;
    ctx->member = 0x0;
    ctx->var = 0x0;
    ctx->type_cunt = 0;
    ctx->member_cunt = 0;
    ctx->var_cunt = 0;
    ctx->ok = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t crosstalk_hdl_type_shutdown(crosstalk_hdl_type_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return crosstalk_hdl_type_clear(ctx);
}

uint8_t crosstalk_hdl_type_ld_is(const crosstalk_hdl_type_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ctx->ok != 0 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t crosstalk_hdl_type_cunt_get(const crosstalk_hdl_type_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->type_cunt;
}

uint32_t crosstalk_hdl_type_var_cunt_get(const crosstalk_hdl_type_ctx_t *ctx)
{
    if (LDG_UNLIKELY(ctx == 0x0)) { return UINT32_MAX; }

    return ctx->var_cunt;
}

uint32_t crosstalk_hdl_type_ld(crosstalk_hdl_type_ctx_t *ctx, const char *path)
{
    uint8_t ident[TYPE_IDENT_LEN] = LDG_ARR_ZERO_INIT;
    uint8_t hdr[TYPE_HDR64_LEN] = LDG_ARR_ZERO_INIT;
    type_parse_t parse = LDG_STRUCT_ZERO_INIT;
    type_build_t build = LDG_STRUCT_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint8_t *sect = 0x0;
    char *shstr = 0x0;
    uint8_t *info = 0x0;
    uint8_t *abbrev = 0x0;
    uint8_t *str = 0x0;
    uint8_t *line_str = 0x0;
    uint8_t *str_off = 0x0;
    uint8_t *addr = 0x0;
    uint64_t shoff = 0;
    uint64_t sect_bytes = 0;
    uint64_t hdr_len = 0;
    uint64_t ent_len = 0;
    uint64_t shstr_off = 0;
    uint64_t shstr_len = 0;
    uint64_t off = 0;
    uint64_t len = 0;
    uint64_t at = 0;
    uint64_t need = 0;
    uint32_t shnum = 0;
    uint32_t shentsize = 0;
    uint32_t shstrndx = 0;
    uint32_t ret = 0;
    uint8_t class_is64 = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = crosstalk_hdl_type_clear(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, TYPE_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = type_rd_at(file, (uint64_t)TYPE_HDR_AT, ident, (uint64_t)TYPE_IDENT_LEN);

    if (ret == LDG_ERR_AOK && (ident[0] != TYPE_MAG0 || ident[0x1] != TYPE_MAG1 || ident[0x2] != TYPE_MAG2 || ident[0x3] != TYPE_MAG3)) { ret = LDG_ERR_IO_FMT; }

    if (ret == LDG_ERR_AOK && ident[TYPE_DATA_AT] != TYPE_DATA_LSB) { ret = LDG_ERR_UNSUPPORTED; }

    if (ret == LDG_ERR_AOK && ident[TYPE_CLASS_AT] != TYPE_CLASS32 && ident[TYPE_CLASS_AT] != TYPE_CLASS64) { ret = LDG_ERR_UNSUPPORTED; }

    if (ret == LDG_ERR_AOK)
    {
        class_is64 = ident[TYPE_CLASS_AT] == TYPE_CLASS64 ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        hdr_len = class_is64 == LDG_TRUTH_TRUE ? (uint64_t)TYPE_HDR64_LEN : (uint64_t)TYPE_HDR32_LEN;
        ent_len = class_is64 == LDG_TRUTH_TRUE ? (uint64_t)TYPE_SHENT64_LEN : (uint64_t)TYPE_SHENT32_LEN;
        parse.addr_size = class_is64 == LDG_TRUTH_TRUE ? (uint8_t)sizeof(uint64_t) : (uint8_t)sizeof(uint32_t);
        ret = type_rd_at(file, (uint64_t)TYPE_HDR_AT, hdr, hdr_len);
    }

    if (ret == LDG_ERR_AOK)
    {
        if (class_is64 == LDG_TRUTH_TRUE)
        {
            shoff = type_u64_get(hdr + TYPE_SHOFF64_AT);
            shentsize = (uint32_t)type_u16_get(hdr + TYPE_SHENTSIZE64_AT);
            shnum = (uint32_t)type_u16_get(hdr + TYPE_SHNUM64_AT);
            shstrndx = (uint32_t)type_u16_get(hdr + TYPE_SHSTRNDX64_AT);
        }
        else
        {
            shoff = (uint64_t)type_u32_get(hdr + TYPE_SHOFF32_AT);
            shentsize = (uint32_t)type_u16_get(hdr + TYPE_SHENTSIZE32_AT);
            shnum = (uint32_t)type_u16_get(hdr + TYPE_SHNUM32_AT);
            shstrndx = (uint32_t)type_u16_get(hdr + TYPE_SHSTRNDX32_AT);
        }

        if (shoff == 0 || shnum == 0 || shnum > (uint32_t)TYPE_SHNUM_MAX || shstrndx >= shnum) { ret = LDG_ERR_NOT_FOUND; }
        else if ((uint64_t)shentsize != ent_len) { ret = LDG_ERR_IO_FMT; }
    }

    if (ret == LDG_ERR_AOK)
    {
        sect_bytes = (uint64_t)shnum * ent_len;

        if (sect_bytes > (uint64_t)TYPE_SECT_BYTES_MAX) { ret = LDG_ERR_BOUNDS; }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(sect_bytes, (void **)&sect); }

    if (ret == LDG_ERR_AOK) { ret = type_rd_at(file, shoff, sect, sect_bytes); }

    if (ret == LDG_ERR_AOK)
    {
        const uint8_t *ent = sect + (uint64_t)shstrndx * ent_len;

        if (class_is64 == LDG_TRUTH_TRUE)
        {
            shstr_off = type_u64_get(ent + TYPE_SH_OFF64_AT);
            shstr_len = type_u64_get(ent + TYPE_SH_SIZE64_AT);
        }
        else
        {
            shstr_off = (uint64_t)type_u32_get(ent + TYPE_SH_OFF32_AT);
            shstr_len = (uint64_t)type_u32_get(ent + TYPE_SH_SIZE32_AT);
        }

        if (shstr_len == 0 || shstr_len > (uint64_t)TYPE_SECT_BYTES_MAX) { ret = LDG_ERR_BOUNDS; }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(shstr_len, (void **)&shstr); }

    if (ret == LDG_ERR_AOK) { ret = type_rd_at(file, shstr_off, shstr, shstr_len); }

    if (ret == LDG_ERR_AOK) { shstr[shstr_len - LDG_STR_TERM_SIZE] = LDG_STR_TERM; }

    if (ret == LDG_ERR_AOK) { ret = type_sect_find(sect, shnum, ent_len, class_is64, shstr, shstr_len, TYPE_SECT_INFO, &off, &len); }

    if (ret == LDG_ERR_AOK && (len == 0 || len > (uint64_t)TYPE_SECT_BYTES_MAX)) { ret = LDG_ERR_BOUNDS; }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(len, (void **)&info); }

    if (ret == LDG_ERR_AOK) { ret = type_rd_at(file, off, info, len); }

    if (ret == LDG_ERR_AOK)
    {
        parse.info.at = info;
        parse.info.len = len;

        ret = type_sect_find(sect, shnum, ent_len, class_is64, shstr, shstr_len, TYPE_SECT_ABBREV, &off, &len);
    }

    if (ret == LDG_ERR_AOK && (len == 0 || len > (uint64_t)TYPE_SECT_BYTES_MAX)) { ret = LDG_ERR_BOUNDS; }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc(len, (void **)&abbrev); }

    if (ret == LDG_ERR_AOK) { ret = type_rd_at(file, off, abbrev, len); }

    if (ret == LDG_ERR_AOK)
    {
        parse.abbrev.at = abbrev;
        parse.abbrev.len = len;
    }

    if (ret == LDG_ERR_AOK && type_sect_find(sect, shnum, ent_len, class_is64, shstr, shstr_len, TYPE_SECT_STR, &off, &len) == LDG_ERR_AOK && len > 0 && len <= (uint64_t)TYPE_SECT_BYTES_MAX) { if (ldg_mem_alloc(len, (void **)&str) == LDG_ERR_AOK && type_rd_at(file, off, str, len) == LDG_ERR_AOK)
        {
            str[len - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
            parse.str.at = str;
            parse.str.len = len;
        }
    }

    if (ret == LDG_ERR_AOK && type_sect_find(sect, shnum, ent_len, class_is64, shstr, shstr_len, TYPE_SECT_LINE_STR, &off, &len) == LDG_ERR_AOK && len > 0 && len <= (uint64_t)TYPE_SECT_BYTES_MAX) { if (ldg_mem_alloc(len, (void **)&line_str) == LDG_ERR_AOK && type_rd_at(file, off, line_str, len) == LDG_ERR_AOK)
        {
            line_str[len - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
            parse.line_str.at = line_str;
            parse.line_str.len = len;
        }
    }

    if (ret == LDG_ERR_AOK && type_sect_find(sect, shnum, ent_len, class_is64, shstr, shstr_len, TYPE_SECT_STR_OFF, &off, &len) == LDG_ERR_AOK && len > 0 && len <= (uint64_t)TYPE_SECT_BYTES_MAX) { if (ldg_mem_alloc(len, (void **)&str_off) == LDG_ERR_AOK && type_rd_at(file, off, str_off, len) == LDG_ERR_AOK)
        {
            parse.str_off.at = str_off;
            parse.str_off.len = len;
        }
    }

    if (ret == LDG_ERR_AOK && type_sect_find(sect, shnum, ent_len, class_is64, shstr, shstr_len, TYPE_SECT_ADDR, &off, &len) == LDG_ERR_AOK && len > 0 && len <= (uint64_t)TYPE_SECT_BYTES_MAX) { if (ldg_mem_alloc(len, (void **)&addr) == LDG_ERR_AOK && type_rd_at(file, off, addr, len) == LDG_ERR_AOK)
        {
            parse.addr.at = addr;
            parse.addr.len = len;
        }
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)CROSSTALK_HDL_TYPE_MAX * (uint64_t)sizeof(crosstalk_hdl_type_rec_t), (void **)&build.type); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)CROSSTALK_HDL_TYPE_MEMBER_MAX * (uint64_t)sizeof(crosstalk_hdl_type_member_t), (void **)&build.member); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)CROSSTALK_HDL_TYPE_VAR_MAX * (uint64_t)sizeof(crosstalk_hdl_type_var_t), (void **)&build.var); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)TYPE_DIE_MAX * (uint64_t)sizeof(uint64_t), (void **)&build.die_off); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)TYPE_DIE_MAX * (uint64_t)sizeof(uint32_t), (void **)&build.die_idx); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)TYPE_DIE_MAX * (uint64_t)sizeof(uint64_t), (void **)&build.cv_off); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_alloc((uint64_t)TYPE_DIE_MAX * (uint64_t)sizeof(uint64_t), (void **)&build.cv_ref); }

    if (ret == LDG_ERR_AOK)
    {
        at = 0;

        while (at + (uint64_t)TYPE_UNIT_HDR_LEN_LEN <= parse.info.len)
        {
            uint64_t unit_len = (uint64_t)type_u32_get(parse.info.at + at);
            uint64_t cu_off = at;
            uint64_t cu_end = 0;
            uint64_t abbrev_base = 0;
            uint32_t ver = 0;

            at = at + (uint64_t)TYPE_UNIT_HDR_LEN_LEN;

            if (unit_len == 0 || unit_len == UINT32_MAX) { break; }

            cu_end = at + unit_len;

            if (cu_end > parse.info.len) { break; }

            if (at + 0x2 > parse.info.len) { break; }

            ver = (uint32_t)type_u16_get(parse.info.at + at);
            at = at + 0x2;

            if (ver < (uint32_t)TYPE_VER_MIN || ver > (uint32_t)TYPE_VER_MAX) { at = cu_end; continue; }

            if (ver >= (uint32_t)TYPE_VER_HDR_SPLIT)
            {
                if (at + 0x2 > parse.info.len) { break; }

                at = at + 0x1;
                parse.addr_size = parse.info.at[at];
                at = at + 0x1;

                if (at + 0x4 > parse.info.len) { break; }

                abbrev_base = (uint64_t)type_u32_get(parse.info.at + at);
                at = at + 0x4;
            }
            else
            {
                if (at + 0x5 > parse.info.len) { break; }

                abbrev_base = (uint64_t)type_u32_get(parse.info.at + at);
                at = at + 0x4;
                parse.addr_size = parse.info.at[at];
                at = at + 0x1;
            }

            if (parse.addr_size != sizeof(uint32_t) && parse.addr_size != sizeof(uint64_t)) { at = cu_end; continue; }

            parse.cu_off = cu_off;
            parse.ver = ver;
            parse.str_off_base = (uint64_t)TYPE_STR_OFF_HDR_LEN;
            parse.addr_base = (uint64_t)TYPE_ADDR_HDR_LEN;

            if (type_cu_walk(&parse, &build, cu_end, abbrev_base, &at) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "type cu walk failed"); }

            at = cu_end;
        }

        type_resolve(&build);

        if (build.type_cunt == 0) { ret = LDG_ERR_EMPTY; }
    }

    if (ret == LDG_ERR_AOK)
    {
        need = (uint64_t)build.type_cunt * (uint64_t)sizeof(crosstalk_hdl_type_rec_t) + (uint64_t)build.member_cunt * (uint64_t)sizeof(crosstalk_hdl_type_member_t) + (uint64_t)build.var_cunt * (uint64_t)sizeof(crosstalk_hdl_type_var_t);

        ret = ldg_mem_pool_create((uint64_t)TYPE_POOL_ITEM_VAR, need + (uint64_t)TYPE_POOL_SLACK, &ctx->pool);
    }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)build.type_cunt * (uint64_t)sizeof(crosstalk_hdl_type_rec_t), (void **)&ctx->type); }

    if (ret == LDG_ERR_AOK && build.member_cunt > 0) { ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)build.member_cunt * (uint64_t)sizeof(crosstalk_hdl_type_member_t), (void **)&ctx->member); }

    if (ret == LDG_ERR_AOK && build.var_cunt > 0) { ret = ldg_mem_pool_alloc(ctx->pool, (uint64_t)build.var_cunt * (uint64_t)sizeof(crosstalk_hdl_type_var_t), (void **)&ctx->var); }

    if (ret == LDG_ERR_AOK)
    {
        memcpy(ctx->type, build.type, (uint64_t)build.type_cunt * (uint64_t)sizeof(crosstalk_hdl_type_rec_t));

        if (build.member_cunt > 0) { memcpy(ctx->member, build.member, (uint64_t)build.member_cunt * (uint64_t)sizeof(crosstalk_hdl_type_member_t)); }

        if (build.var_cunt > 0) { memcpy(ctx->var, build.var, (uint64_t)build.var_cunt * (uint64_t)sizeof(crosstalk_hdl_type_var_t)); }

        ctx->type_cunt = build.type_cunt;
        ctx->member_cunt = build.member_cunt;
        ctx->var_cunt = build.var_cunt;
        ctx->gen = ctx->gen + 0x1;
        ctx->ok = LDG_TRUTH_TRUE;
    }

    if (build.cv_ref != 0x0) { ldg_mem_dealloc(build.cv_ref); }

    if (build.cv_off != 0x0) { ldg_mem_dealloc(build.cv_off); }

    if (build.die_idx != 0x0) { ldg_mem_dealloc(build.die_idx); }

    if (build.die_off != 0x0) { ldg_mem_dealloc(build.die_off); }

    if (build.var != 0x0) { ldg_mem_dealloc(build.var); }

    if (build.member != 0x0) { ldg_mem_dealloc(build.member); }

    if (build.type != 0x0) { ldg_mem_dealloc(build.type); }

    if (addr != 0x0) { ldg_mem_dealloc(addr); }

    if (str_off != 0x0) { ldg_mem_dealloc(str_off); }

    if (line_str != 0x0) { ldg_mem_dealloc(line_str); }

    if (str != 0x0) { ldg_mem_dealloc(str); }

    if (abbrev != 0x0) { ldg_mem_dealloc(abbrev); }

    if (info != 0x0) { ldg_mem_dealloc(info); }

    if (shstr != 0x0) { ldg_mem_dealloc(shstr); }

    if (sect != 0x0) { ldg_mem_dealloc(sect); }

    if (ldg_io_file_close(file) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "type close failed"); }

    if (ret != LDG_ERR_AOK)
    {
        if (crosstalk_hdl_type_clear(ctx) != LDG_ERR_AOK) { ldg_logging_line_wr(CROSSTALK_CORE_LOG_PATH, LDG_LOGGING_LVL_WARN, "type clear failed"); }

        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_get(const crosstalk_hdl_type_ctx_t *ctx, uint32_t idx, const crosstalk_hdl_type_rec_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->type == 0x0) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->type_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->type[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_member_get(const crosstalk_hdl_type_ctx_t *ctx, uint32_t idx, const crosstalk_hdl_type_member_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->member == 0x0) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->member_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->member[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_find(const crosstalk_hdl_type_ctx_t *ctx, const char *name, uint32_t *out)
{
    uint32_t at = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || name == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint32_t)UINT32_MAX;

    if (name[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->type == 0x0) { return LDG_ERR_NOT_INIT; }

    for (at = 0; at < ctx->type_cunt; at = at + 0x1)
    {
        if (strncmp(ctx->type[at].alias, name, (uint64_t)CROSSTALK_HDL_TYPE_NAME_CAP) == 0 && ctx->type[at].alias[0] != LDG_STR_TERM)
        {
            *out = at;

            return LDG_ERR_AOK;
        }

        if (strncmp(ctx->type[at].name, name, (uint64_t)CROSSTALK_HDL_TYPE_NAME_CAP) == 0 && ctx->type[at].name[0] != LDG_STR_TERM)
        {
            *out = at;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_hdl_type_label_get(const crosstalk_hdl_type_rec_t *rec, const char **out)
{
    if (LDG_UNLIKELY(rec == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (rec->alias[0] != LDG_STR_TERM) { *out = rec->alias; return LDG_ERR_AOK; }

    if (rec->name[0] != LDG_STR_TERM) { *out = rec->name; return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

uint32_t crosstalk_hdl_type_var_at(const crosstalk_hdl_type_ctx_t *ctx, uint64_t addr, const crosstalk_hdl_type_var_t **out, uint64_t *out_off)
{
    uint32_t lo = 0;
    uint32_t hi = 0;
    uint32_t best = UINT32_MAX;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0 || out_off == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_off = 0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->var == 0x0 || ctx->var_cunt == 0) { return LDG_ERR_NOT_INIT; }

    hi = ctx->var_cunt;

    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 0x2;

        if (ctx->var[mid].addr <= addr)
        {
            best = mid;
            lo = mid + 0x1;
        }
        else { hi = mid; }
    }

    if (best == UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

    if (ctx->var[best].size == 0 || addr >= ctx->var[best].addr + ctx->var[best].size) { return LDG_ERR_NOT_FOUND; }

    *out = &ctx->var[best];
    *out_off = addr - ctx->var[best].addr;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_var_get(const crosstalk_hdl_type_ctx_t *ctx, uint32_t idx, const crosstalk_hdl_type_var_t **out)
{
    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->var == 0x0) { return LDG_ERR_NOT_INIT; }

    if (idx >= ctx->var_cunt) { return LDG_ERR_BOUNDS; }

    *out = &ctx->var[idx];

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_field_find(const crosstalk_hdl_type_ctx_t *ctx, uint32_t type_idx, const char *path, crosstalk_hdl_type_field_t *out)
{
    uint64_t off = 0;
    uint32_t cur = type_idx;
    uint32_t at = 0;
    uint32_t depth = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || path == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    out->type_idx = (uint32_t)UINT32_MAX;
    out->ref = (uint32_t)UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->type == 0x0) { return LDG_ERR_NOT_INIT; }

    if (type_idx >= ctx->type_cunt || path[0] == LDG_STR_TERM) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (path[at] != LDG_STR_TERM && depth < (uint32_t)CROSSTALK_HDL_TYPE_DEPTH_MAX)
    {
        char name[CROSSTALK_HDL_TYPE_NAME_CAP] = LDG_ARR_ZERO_INIT;
        const crosstalk_hdl_type_rec_t *rec = 0x0;
        uint32_t pos = 0;
        uint32_t hit = (uint32_t)UINT32_MAX;
        uint32_t member_at = 0;

        while (path[at] != LDG_STR_TERM && path[at] != (char)CROSSTALK_HDL_TYPE_PATH_SEP && pos + LDG_STR_TERM_SIZE < (uint32_t)sizeof(name))
        {
            name[pos] = path[at];
            pos = pos + 0x1;
            at = at + 0x1;
        }

        name[pos] = LDG_STR_TERM;

        if (path[at] == (char)CROSSTALK_HDL_TYPE_PATH_SEP) { at = at + 0x1; }

        rec = &ctx->type[cur];

        if (rec->member_cunt == 0 && rec->ref != (uint32_t)UINT32_MAX && rec->ref < ctx->type_cunt)
        {
            cur = rec->ref;
            rec = &ctx->type[cur];
        }

        for (member_at = 0; member_at < rec->member_cunt; member_at = member_at + 0x1)
        {
            if (strncmp(ctx->member[rec->member_at + member_at].name, name, (uint64_t)CROSSTALK_HDL_TYPE_NAME_CAP) != 0) { continue; }

            hit = rec->member_at + member_at;

            break;
        }

        if (hit == (uint32_t)UINT32_MAX) { return LDG_ERR_NOT_FOUND; }

        off = off + ctx->member[hit].off;

        if (ctx->member[hit].type_idx == (uint32_t)UINT32_MAX || ctx->member[hit].type_idx >= ctx->type_cunt) { return LDG_ERR_NOT_FOUND; }

        cur = ctx->member[hit].type_idx;
        depth = depth + 0x1;
    }

    type_name_cpy(out->path, path, (uint32_t)CROSSTALK_HDL_TYPE_PATH_CAP);

    out->off = off;
    out->size = ctx->type[cur].size;
    out->kind = ctx->type[cur].kind;
    out->type_idx = cur;
    out->ref = ctx->type[cur].ref;

    return LDG_ERR_AOK;
}

uint32_t crosstalk_hdl_type_field_at(const crosstalk_hdl_type_ctx_t *ctx, uint32_t type_idx, uint64_t off, crosstalk_hdl_type_field_t *out)
{
    uint64_t rest = off;
    uint32_t cur = type_idx;
    uint32_t depth = 0;
    uint32_t path_len = 0;

    if (LDG_UNLIKELY(ctx == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0x0, sizeof(*out));

    out->type_idx = (uint32_t)UINT32_MAX;
    out->ref = (uint32_t)UINT32_MAX;

    if (ctx->ok != LDG_TRUTH_TRUE || ctx->type == 0x0) { return LDG_ERR_NOT_INIT; }

    if (type_idx >= ctx->type_cunt) { return LDG_ERR_BOUNDS; }

    while (depth < (uint32_t)CROSSTALK_HDL_TYPE_DEPTH_MAX)
    {
        const crosstalk_hdl_type_rec_t *rec = &ctx->type[cur];

        if (rec->kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_STRUCT || rec->kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_UNION)
        {
            uint32_t at = 0;
            uint32_t hit = (uint32_t)UINT32_MAX;

            if (rec->member_cunt == 0 && rec->ref != (uint32_t)UINT32_MAX && rec->ref < ctx->type_cunt)
            {
                cur = rec->ref;
                depth = depth + 0x1;

                continue;
            }

            for (at = 0; at < rec->member_cunt; at = at + 0x1)
            {
                const crosstalk_hdl_type_member_t *member = &ctx->member[rec->member_at + at];
                uint64_t span = member->size != 0 ? member->size : 0x1;

                if (rest < member->off || rest >= member->off + span) { continue; }

                hit = rec->member_at + at;

                break;
            }

            if (hit == (uint32_t)UINT32_MAX) { break; }

            if (path_len != 0 && path_len + (uint32_t)TYPE_PATH_SEP_LEN < (uint32_t)CROSSTALK_HDL_TYPE_PATH_CAP)
            {
                out->path[path_len] = (char)CROSSTALK_HDL_TYPE_PATH_SEP;
                path_len = path_len + (uint32_t)TYPE_PATH_SEP_LEN;
                out->path[path_len] = LDG_STR_TERM;
            }

            type_name_cpy(out->path + path_len, ctx->member[hit].name, (uint32_t)CROSSTALK_HDL_TYPE_PATH_CAP - path_len);
            path_len = (uint32_t)strlen(out->path);

            rest = rest - ctx->member[hit].off;

            if (ctx->member[hit].type_idx == (uint32_t)UINT32_MAX || ctx->member[hit].type_idx >= ctx->type_cunt) { break; }

            cur = ctx->member[hit].type_idx;
            depth = depth + 0x1;

            continue;
        }

        if (rec->kind == (uint32_t)CROSSTALK_HDL_TYPE_KIND_ARR)
        {
            char idx_text[TYPE_IDX_TEXT_CAP] = LDG_ARR_ZERO_INIT;
            const crosstalk_hdl_type_rec_t *elem = 0x0;
            uint64_t slot = 0;

            if (rec->ref == (uint32_t)UINT32_MAX || rec->ref >= ctx->type_cunt) { break; }

            elem = &ctx->type[rec->ref];

            if (elem->size == 0) { break; }

            slot = rest / elem->size;

            if (snprintf(idx_text, sizeof(idx_text), TYPE_IDX_FMT, (char)CROSSTALK_HDL_TYPE_IDX_OPEN, slot, (char)CROSSTALK_HDL_TYPE_IDX_CLOSE) < 0) { break; }

            type_name_cpy(out->path + path_len, idx_text, (uint32_t)CROSSTALK_HDL_TYPE_PATH_CAP - path_len);
            path_len = (uint32_t)strlen(out->path);

            rest = rest - slot * elem->size;
            cur = rec->ref;
            depth = depth + 0x1;

            continue;
        }

        break;
    }

    out->off = off - rest;
    out->size = ctx->type[cur].size;
    out->kind = ctx->type[cur].kind;
    out->type_idx = cur;
    out->ref = ctx->type[cur].ref;

    if (out->path[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}
