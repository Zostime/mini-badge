#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

// 事件类型
#define EV_KEY   0x01

// 按键码
#define KEY_A       30
#define KEY_B       48
#define KEY_C       60

// 事件结构
struct input_event {
    uint16_t type;    /* EV_KEY */
    uint16_t code;    /* KEY_A */
    int32_t  value;   /* 1=按下, 0=抬起, 2=重复 */
};

void input_init(void);
void input_event_push(uint16_t type, uint16_t code, int32_t value);

#endif
