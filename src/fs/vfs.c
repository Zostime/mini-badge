#include <kernel/vfs.h>
#include <kernel/tty.h>
#include <kernel/device.h>
#include "ff.h"
#include <string.h>
#include <stdarg.h>

extern int chrdev_open(const char *name, const file_ops_t **ops, void **ctx);

// fd table
typedef struct {
    const file_ops_t *ops;
    void             *ctx;
    FIL               fil;
    uint8_t           used;
} vfs_fd_t;

static vfs_fd_t fd_table[VFS_MAX_FD];

// fd -> FatFs 权限映射
static BYTE fatfs_mode(int flags)
{
    BYTE mode = 0;

    if ((flags & O_ACCMODE) != O_WRONLY) mode |= FA_READ;
    if ((flags & O_ACCMODE) != O_RDONLY) mode |= FA_WRITE;

    if (flags & O_APPEND)
        mode |= FA_OPEN_APPEND;            
    else if (flags & O_CREAT) {
        if (flags & O_EXCL)       mode |= FA_CREATE_NEW;
        else if (flags & O_TRUNC) mode |= FA_CREATE_ALWAYS;
        else                      mode |= FA_OPEN_ALWAYS;
    } else
        mode |= FA_OPEN_EXISTING;

    return mode;
}

// 分配 fd
static int alloc_fd(void)
{
    for (int i = VFS_FD_BASE; i < VFS_MAX_FD; i++) {
        if (!fd_table[i].used) {
            fd_table[i].used = 1;
            return i;
        }
    }
    return -1;
}

// 检查 fd 是否有效
static vfs_fd_t *get_fd(int fd)
{
    if (fd < VFS_FD_BASE || fd >= VFS_MAX_FD) return NULL;
    if (!fd_table[fd].used) return NULL;
    return &fd_table[fd];
}

int open(const char *pathname, int flags, ...)
{
    (void)0;
    if (pathname == NULL) return -1;

    int fd = alloc_fd();
    if (fd < 0) return -1;

    // 字符设备
    if (strncmp(pathname, "/dev/", 5) == 0) {
        const file_ops_t *ops;
        void *ctx;
        if (chrdev_open(pathname + 5, &ops, &ctx) != 0) {
            fd_table[fd].used = 0;
            return -1;
        }
        fd_table[fd].ops = ops;
        fd_table[fd].ctx = ctx;
        return fd;
    }

    // 文件系统
    if (f_open(&fd_table[fd].fil, pathname, fatfs_mode(flags)) != FR_OK) {
        fd_table[fd].used = 0;
        return -1;
    }
    fd_table[fd].ops = NULL;
    fd_table[fd].ctx = &fd_table[fd].fil;
    return fd;
}


ssize_t read(int fd, void *buf, size_t count)
{
    if (fd == 0) {	// STDIN_FILENO
        return tty_read((char *)buf, (int)count);
    }

    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;

    // 字符设备
    if (p->ops != NULL) {
        if (p->ops->read == NULL) return -1;
        return p->ops->read(p->ctx, buf, count);
    }

    // 文件
    if (count > 0xFFFF) count = 0xFFFF;
    UINT br = 0;
    if (f_read(&p->fil, buf, count, &br) != FR_OK) return -1;
    return (ssize_t)br;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    if (fd == 1 || fd == 2) {	// STDOUT / STDERR
        return tty_write((char *)buf, (int)count);
    }

    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;

    // 字符设备
    if (p->ops != NULL) {
        if (p->ops->write == NULL) return -1;
        return p->ops->write(p->ctx, buf, count);
    }

    // 文件
    UINT bw = 0;
    if (f_write(&p->fil, buf, count, &bw) != FR_OK) return -1;
    return (ssize_t)bw;
}

int close(int fd)
{
    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;

    if (p->ops != NULL) {
        // 字符设备
        int r = 0;
        if (p->ops->close != NULL)
            r = p->ops->close(p->ctx);
        p->used = 0;
        p->ops  = NULL;
        p->ctx  = NULL;
        return r;
    }

    // 文件
    FRESULT res = f_close(&p->fil);
    p->used = 0;
    return (res == FR_OK) ? 0 : -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;
	if (p->ops != NULL) return -1;      // 字符设备

    FSIZE_t pos;

    switch (whence) {
        case SEEK_SET: pos = offset; break;
        case SEEK_CUR: pos = f_tell(&p->fil) + offset; break;
        case SEEK_END: pos = f_size(&p->fil) + offset; break;
        default: return -1;
    }

    if (f_lseek(&p->fil, pos) != FR_OK) return -1;
    return (off_t)pos;
}
