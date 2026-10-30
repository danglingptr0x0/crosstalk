#ifndef CROSSTALK_RE_SPAWN_H
#define CROSSTALK_RE_SPAWN_H

#include <stdint.h>

#define CROSSTALK_RE_SPAWN_ERR_SPAWN 813
#define CROSSTALK_RE_SPAWN_ERR_EXIT 814
#define CROSSTALK_RE_SPAWN_ERR_ARG 815

#define CROSSTALK_RE_SPAWN_LINE_CAP 2048
#define CROSSTALK_RE_SPAWN_ARG_MAX 64
#define CROSSTALK_RE_SPAWN_EXIT_OK 0
#define CROSSTALK_RE_SPAWN_WAIT_FLAGS 0

uint32_t crosstalk_re_spawn_run(const char *line, uint32_t *out_exit);

#endif
