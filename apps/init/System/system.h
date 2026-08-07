#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdint.h>
#include "sys_config.h"
#include "ff.h"

void SYS_Init(void);
FRESULT SYS_DisplayBMP(uint16_t x, uint16_t y, const char *path);

#endif
