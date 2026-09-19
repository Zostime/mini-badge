#include <kernel/vfs.h>
#include <kernel/tty.h>
#include "ff.h"
#include <string.h>
#include <stdarg.h>

// fd table
typedef struct {
    FIL     fil;
    uint8_t used;
} vfs_fd_t;

static vfs_fd_t fd_table[VFS_MAX_FD];

// fd -> FatFs 权限映射
static BYTE fatfs_mode(int flags)
{
    BYTE mode = 0;

    // 读写权限
    if ((flags & O_ACCMODE) != O_WRONLY) mode |= FA_READ;
    if ((flags & O_ACCMODE) != O_RDONLY) mode |= FA_WRITE;

    // 创建/截断
    if (flags & O_CREAT) {
        if (flags & O_EXCL) {
            mode |= FA_CREATE_NEW;       // 存在则失败
        } else if (flags & O_TRUNC) {
            mode |= FA_CREATE_ALWAYS;    // 存在则清空
        } else {
            mode |= FA_OPEN_ALWAYS;      // 存在则打开，否则创建
        }
    } else {
        mode |= FA_OPEN_EXISTING;        // 不存在则失败
    }

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
    int fd = alloc_fd();
    if (fd < 0) return -1;

    BYTE mode = fatfs_mode(flags);
    if (f_open(&fd_table[fd].fil, pathname, mode) != FR_OK) {
        fd_table[fd].used = 0;
        return -1;
    }

    // O_APPEND：定位到文件末尾
    if (flags & O_APPEND) {
        f_lseek(&fd_table[fd].fil, f_size(&fd_table[fd].fil));
    }

    return fd;
}

int close(int fd)
{
    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;

    FRESULT res = f_close(&p->fil);
    p->used = 0;                  
    return (res == FR_OK) ? 0 : -1;
}

ssize_t read(int fd, void *buf, size_t count)
{
    if (fd == 0) {	// STDIN_FILENO
        return tty_read((char *)buf, (int)count);
    }
	
    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;
    if (count > 0xFFFF) count = 0xFFFF; 

    UINT br = 0;
    if (f_read(&p->fil, buf, count, &br) != FR_OK) return -1;
    return (ssize_t)br;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;

    UINT bw = 0;
    if (f_write(&p->fil, buf, count, &bw) != FR_OK) return -1;
    return (ssize_t)bw;
}

off_t lseek(int fd, off_t offset, int whence)
{
    vfs_fd_t *p = get_fd(fd);
    if (!p) return -1;

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
