#ifndef __BOOTLOADER_API_H
#define __BOOTLOADER_API_H

#include <stdint.h>
#include <string.h>

#define DEFAULT_APP_PATH    	"0:/bin/init"
#define MAX_APP_PATH            84			// 最大值84, 必须为偶数
#define BKP_PATH_START_DR       1           // min: 1, max: 42
#define BKP_MAX_DR				42
#define APP_ADDR    	0x08004000   // APP 存放的 Flash 起始地址
#define APP_MAX_SIZE	0x08020000 - APP_ADDR 

#define BKP_DR(n)	(*(volatile uint16_t *)(BKP_BASE + 0x04 + ((n) - 1) * 4))

static __inline void BKP_EnableWrite(void) {
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;
}

static inline void BKP_WritePath(const char *path) {
    uint32_t len = strlen(path);
    if (len > MAX_APP_PATH) len = MAX_APP_PATH;

    uint32_t words = (len + 1) / 2;    
    for (uint32_t i = 0; i < words; i++) {
        uint16_t data = 0;
        if (i * 2 < len)       data |= (uint8_t)path[i * 2];
        if (i * 2 + 1 < len)   data |= ((uint8_t)path[i * 2 + 1]) << 8;
        BKP_DR(BKP_PATH_START_DR + i) = data;
    }
    if (words < (BKP_MAX_DR - BKP_PATH_START_DR + 1)) {
        BKP_DR(BKP_PATH_START_DR + words) = 0;
    }
}

static inline void BKP_ReadPath(char *buf) {
    uint32_t i;
    for (i = 0; i < MAX_APP_PATH / 2; i++) {
        uint16_t data = BKP_DR(BKP_PATH_START_DR + i);
        if (i * 2 < MAX_APP_PATH) buf[i * 2] = (uint8_t)(data & 0xFF);
        if (i * 2 + 1 < MAX_APP_PATH) buf[i * 2 + 1] = (uint8_t)(data >> 8);
        if ((data & 0xFF) == 0 || (data >> 8) == 0) break;
    }
    if (MAX_APP_PATH > 0) buf[MAX_APP_PATH - 1] = '\0';
}


#define BOOTLOADER_READ_REQUEST_PATH(Buf) do { \
											BKP_ReadPath(Buf); \
										  } while(0U) 

#define BOOTLOADER_REQUEST_APP(Path) do { \
										  BKP_EnableWrite(); \
										  BKP_WritePath(Path); \
                                          SCB->AIRCR = (0x5FA << 16) | (1 << 2); \
										} while(0U)

#endif
