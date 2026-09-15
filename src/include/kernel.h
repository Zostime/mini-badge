#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "errno.h"
#include <stdint.h>
#include <stdio.h>

#define EXEC_MAX_ARGS   16
#define EXEC_MAX_ENVS   16
#define EXEC_ARG_MAX    128
#define EXEC_ENV_MAX    128

#define STDIN_FILENO 	0
#define STDOUT_FILENO	1
#define STDERR_FILENO 	2

typedef signed int ssize_t;

typedef struct {
    uint8_t xres;        /* 水平分辨率 (px) */
    uint8_t yres;        /* 垂直分辨率 (px) */
} screeninfo_t;

extern screeninfo_t screen_info;

err_t execve(const char *pathname, char *const argv[], char *const envp[]);
err_t execve_load(int *argc, char *argv[], char *envp[]);

int open(const char *pathname, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

#endif
