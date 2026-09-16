#ifndef __VFS_H
#define __VFS_H

#include "kernel/types.h"
#include "stdio.h"

/* open flags */
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0003
#define O_ACCMODE   0x0003
#define O_CREAT     0x0100
#define O_EXCL      0x0200
#define O_TRUNC     0x0400
#define O_APPEND    0x0800

#define VFS_MAX_FD  8           /* 最大同时打开文件数 */
#define VFS_FD_BASE 3           /* 0/1/2 留给 stdin/stdout/stderr */

int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);

#endif
