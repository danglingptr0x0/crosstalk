#ifndef DANGEM_RUN_H
#define DANGEM_RUN_H

#include <stdint.h>

#define DANGEM_RUN_CMDLINE_CAP 4096
#define DANGEM_RUN_ARG_MAX 16
#define DANGEM_RUN_STEP 1

uint32_t dangem_run_exec(const char *path, const char *const *argv, const char *cwd, uint32_t *out_code);
uint32_t dangem_run_pform_get(const char **out_name, const char **out_exe_ext);
uint8_t dangem_run_ldg_bundled_is(void);
uint8_t dangem_run_shlib_local_is(void);
uint32_t dangem_run_build_dir_get(const char **out);
uint32_t dangem_run_tty_ensure(void);

#endif
