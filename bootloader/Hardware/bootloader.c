#include "main.h"
#include "ff.h"
#include "bootloader.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_flash_ex.h"

void Bootloader_JumpAddr(uint32_t address) {
    uint32_t sp = *(volatile uint32_t*) address;
    uint32_t pc = *(volatile uint32_t*)(address + 4);

    if ((sp < 0x20000000) || (sp > 0x20005000) || ((pc & 0x1) == 0)) return;

	__disable_irq();
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
	
    SysTick->CTRL = 0;
    SCB->VTOR = address;
    __DSB(); __ISB();
    __set_MSP(sp);
    __DSB(); __ISB();
	__enable_irq();
    ((void(*)(void))pc)();
}

void Bootloader_Run(const char *path) {
	#if !BOOTLOADER_DEBUG_MODE           	
    FIL file;
    FRESULT res;
    UINT bytes_read;
    uint8_t buffer[FLASH_PROG_BUFFER];
	uint32_t app_addr = APP_ADDR;
    uint32_t app_size;

    // 打开文件
    if (f_open(&file, path, FA_READ) != FR_OK) {
        // 失败
        return;
    }

    app_size = f_size(&file);
    if (app_size == 0 || app_size > APP_MAX_SIZE) {
        f_close(&file);
        return;
    }

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = APP_ADDR;           // 起始地址
    EraseInitStruct.NbPages     = (app_size + FLASH_PAGE_SIZE-1) / FLASH_PAGE_SIZE;  // 页数

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        // 擦除失败
        HAL_FLASH_Lock();
        f_close(&file);
        return;
    }

    // 编程
    FLASH->CR |= FLASH_CR_PG;
    while (1) {
        res = f_read(&file, buffer, sizeof(buffer), &bytes_read);
        if (res != FR_OK || bytes_read == 0) break;
        for (UINT i = 0; i < bytes_read; i += 2) {
            uint16_t data = (i + 1 < bytes_read) ? (buffer[i] | (buffer[i+1] << 8)) : buffer[i];
            *(volatile uint16_t*)app_addr = data;
            while (FLASH->SR & FLASH_SR_BSY);
            app_addr += 2;
        }
    }
    FLASH->CR &= ~FLASH_CR_PG;

    f_close(&file);
    HAL_FLASH_Lock();
	f_mount(NULL, "0:", 1);
	SPI1->CR1 = 0;
	__HAL_RCC_SPI1_FORCE_RESET();
	__HAL_RCC_SPI1_RELEASE_RESET();
	#endif
	
	// 跳转到 APP
    Bootloader_JumpAddr(APP_ADDR);
}
