#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdint.h>

#define SYS_PRINT_BUFFER_SIZE (30*16)*3 + 1 // (30*16)*3 为全屏汉字所需字节数量, +1为 '\0'
#define SYS_CMD_SIZE 64

void SYS_Init(void);
void SYS_Printf(uint16_t x, uint16_t y, uint16_t color, uint16_t background_color, const char *fmt, ...);
void SYS_ShellExecute(char *cmd);

#endif
