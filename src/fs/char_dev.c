#include <kernel/device.h>
#include <string.h>

// "/dev/*" 的设备节点处理

/**
 * @brief  打开一个字符设备
 * @param  name : 去掉 "/dev/" 前缀的设备名
 * @return ops / ctx 通过指针返回，成功返回 0
 */
int chrdev_open(const char *name, const file_ops_t **ops, void **ctx)
{
    const device_t *dev = device_find(name);
    if (dev == NULL) return -1;

    *ops = dev->ops;
    *ctx = dev->ctx;
    return 0;
}
