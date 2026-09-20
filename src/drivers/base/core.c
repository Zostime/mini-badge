#include <kernel/device.h>
#include <string.h>

#define DEV_MAX  8

static const device_t *dev_table[DEV_MAX];
static int dev_count = 0;

int device_register(const device_t *dev)
{
    if (dev == NULL || dev->name == NULL) return -1;
    if (dev_count >= DEV_MAX) return -1;

    dev_table[dev_count++] = dev;
    return 0;
}

const device_t *device_find(const char *name)
{
    for (int i = 0; i < dev_count; i++) {
        if (strcmp(dev_table[i]->name, name) == 0)
            return dev_table[i];
    }
    return NULL;
}
