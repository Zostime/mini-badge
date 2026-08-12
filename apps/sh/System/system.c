#include "system.h"
#include "ff.h"
#include "ST7789V.h"
#include "GUI.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

FATFS sSDCARD_FatFs;
void SYS_Init(void)
{
    FRESULT SD_res;
    SD_res = f_mount(&sSDCARD_FatFs, "0:", 0);
    if (SD_res != FR_OK) {
        BYTE work[512];
        SD_res = f_mkfs("0:", 0, work, sizeof(work));
        if (SD_res == FR_OK) {
            SD_res = f_mount(&sSDCARD_FatFs, "0:", 1);
        }
    }    
    
    LCD_Clear(BLACK);
    LCD_SetBrightness(1000);
}

static int utf8_decode(const uint8_t *p, const uint8_t *end, uint32_t *cp)
{
    if (p >= end) return 0;
    uint8_t c = *p;
    int len;
    uint32_t uc;

    if ((c & 0x80) == 0) {
        *cp = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        len = 2; uc = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        len = 3; uc = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        len = 4; uc = c & 0x07;
    } else {
        return 0;
    }
    if (p + len > end) return 0;
    for (int i = 1; i < len; i++) {
        if ((p[i] & 0xC0) != 0x80) return 0;
        uc = (uc << 6) | (p[i] & 0x3F);
    }
    *cp = uc;
    return len;
}

void SYS_Printf(uint16_t x, uint16_t y, uint16_t color, uint16_t background_color, const char *fmt, ...)
{
    char buf[SYS_PRINT_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    FIL file_uni;
    if (f_open(&file_uni, "0:/sys/fonts/UNICODE_DEFAULT_8x8", FA_READ) != FR_OK)
        return;

    uint16_t cur_x = x, cur_y = y;
    const uint8_t *p = (const uint8_t *)buf;
    const uint8_t *end = (const uint8_t *)(buf + sizeof(buf));

    while (p < end && *p)
    {
        if (*p == '\n') {
            cur_x = x;
            cur_y += 8;
            p++;
            continue;
        }

        uint32_t cp;
        int consumed = utf8_decode(p, end, &cp);
        if (consumed <= 0) {
            p++;           
            continue;
        }

        // 从字库读取该码点的8字节点阵
        uint8_t col_data[8];
        f_lseek(&file_uni, (DWORD)cp * 8);
        UINT br;
        f_read(&file_uni, col_data, 8, &br);

		// 全半角字符
		uint8_t x_offset = (
			cp<=0x7F ||				    // ascii
			(0xFF65<=cp&&cp<=0xFF9F) || // 日语半角片假名与标点
			(0x400<=cp&&cp<=0x4FF) ||   // 西里尔字母
			(0x370<=cp&&cp<=0x3FF)		// 希腊字母
		) ? 6:8;                               
		
        if (cur_x + x_offset > LCD_H) {
            cur_x = x;
            cur_y += 8;
        }
        if (cur_y + 8 > LCD_W) break;

        // 逐行批量发送
        for (uint8_t row = 0; row < 8; row++) {
            uint16_t row_buf[8];
            for (uint8_t col = 0; col < x_offset; col++) {
                if (col_data[col] & (0x01 << row))
                    row_buf[col] = color;
                else
                    row_buf[col] = background_color;
            }

            LCD_SetWindows(cur_x, cur_y + row, cur_x + (x_offset-1), cur_y + row);
            LCD_CS_CLR;
            LCD_RS_SET;
            SPI_SET_16BIT;
            for (uint8_t col = 0; col < x_offset; col++)
                SPI_TRANSMIT_16BIT(row_buf[col]);
            SPI_WAIT();
            SPI_SET_8BIT;
            LCD_CS_SET;
        }

        cur_x += x_offset;
        p += consumed;
    }

    f_close(&file_uni);
}
