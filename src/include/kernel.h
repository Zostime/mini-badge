#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "errno.h"
#include <stdint.h>

#define EXEC_MAX_ARGS   16
#define EXEC_MAX_ENVS   16
#define EXEC_ARG_MAX    128
#define EXEC_ENV_MAX    128

typedef struct {
    uint8_t xres;        /* 水平分辨率 (px) */
    uint8_t yres;        /* 垂直分辨率 (px) */
} screeninfo_t;

extern screeninfo_t screen_info;

err_t execve(const char *pathname, char *const argv[], char *const envp[]);
err_t execve_load(int *argc, char *argv[], char *envp[]);

#endif
