#ifndef __DEVICE_H
#define __DEVICE_H

#include <kernel/types.h>
#include <stdio.h> 

// 设备操作表
typedef struct file_ops {
    ssize_t (*read)(void *ctx, void *buf, size_t count);
    ssize_t (*write)(void *ctx, const void *buf, size_t count);
    int     (*close)(void *ctx);
} file_ops_t;

// 设备描述
typedef struct device {
    const char       *name;   
    const file_ops_t *ops;
    void             *ctx;
} device_t;

int device_register(const device_t *dev);
const device_t *device_find(const char *name);

#endif
