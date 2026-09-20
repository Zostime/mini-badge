#include <kernel/device.h>
#include <kernel/input.h>
#include <string.h>
#include "main.h"

#define EVQ_SIZE 16

static struct input_event evq[EVQ_SIZE];
static volatile int evq_head = 0, evq_tail = 0;

void input_event_push(uint16_t type, uint16_t code, int32_t value)
{
    int next = (evq_head + 1) % EVQ_SIZE;
    if (next == evq_tail) return;
    evq[evq_head].type  = type;
    evq[evq_head].code  = code;
    evq[evq_head].value = value;
    evq_head = next;
}

static ssize_t event0_read(void *ctx, void *buf, size_t count)
{
    (void)ctx;
    if (count < sizeof(struct input_event)) return -1;

    while (evq_head == evq_tail) HAL_Delay(1);

    struct input_event *ev = (struct input_event *)buf;
    *ev = evq[evq_tail];
    evq_tail = (evq_tail + 1) % EVQ_SIZE;
    return sizeof(struct input_event);
}

static const file_ops_t event0_ops = {
    .read  = event0_read,
    .write = NULL,
    .close = NULL,
};

static const device_t event0_dev = {
    .name = "input/event0",
    .ops  = &event0_ops,
    .ctx  = NULL,
};

void input_init(void)
{
    device_register(&event0_dev);
}
