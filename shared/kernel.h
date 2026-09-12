#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "errno.h"

#define KERNEL_LINE_MAX   128

err_t execve(const char *pathname, char *const argv[], char *const envp[]);

#endif
