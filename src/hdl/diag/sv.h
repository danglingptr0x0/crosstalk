#ifndef CROSSTALK_HDL_DIAG_SV_H
#define CROSSTALK_HDL_DIAG_SV_H

#include <stdint.h>

#include "hdl/diag/diag.h"
#include "hdl/parse.h"

uint32_t crosstalk_hdl_diag_sv_entry_find(const crosstalk_hdl_parse_ctx_t *parse, uint32_t err, crosstalk_hdl_diag_t *out);

#endif
