#ifndef CROSSTALK_EDA_SCRIPT_PARSE_H
#define CROSSTALK_EDA_SCRIPT_PARSE_H

#include <stdint.h>

#include "eda/script/ast.h"
#include "eda/script/lex.h"

#define CROSSTALK_EDA_SCRIPT_DIAG_MSG_CAP 160
#define CROSSTALK_EDA_SCRIPT_DIAG_SUGGESTION_CAP 160

typedef struct crosstalk_eda_script_diag
{
    char msg[CROSSTALK_EDA_SCRIPT_DIAG_MSG_CAP];
    char suggestion[CROSSTALK_EDA_SCRIPT_DIAG_SUGGESTION_CAP];
    uint32_t line;
    uint32_t col;
} crosstalk_eda_script_diag_t;

uint32_t crosstalk_eda_script_parse_program(crosstalk_eda_script_lex_t *lex, crosstalk_eda_script_ast_t *ast, crosstalk_eda_script_node_t **out, crosstalk_eda_script_diag_t *diag);

#endif
