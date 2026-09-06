#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include "bootloader_api.h"

#define BOOTLOADER_DEBUG_MODE 0		 // DEBUG: 使能直接进入 APP, 设为1时APP间跳转无效
#define FLASH_PROG_BUFFER 512		 // Byte	

void Bootloader_Run(const char *path);

#endif
