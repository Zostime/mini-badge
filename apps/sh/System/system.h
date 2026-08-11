#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdint.h>

#define SYS_PRINT_BUFFER_SIZE 1024

void SYS_Init(void);
void SYS_Printf(uint16_t x, uint16_t y, uint16_t color, uint16_t background_color, const char *fmt, ...);

#endif
