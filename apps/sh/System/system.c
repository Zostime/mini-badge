#include "system.h"
#include "ff.h"
#include "ST7789V.h"
#include "GUI.h"
#include "bootloader_api.h"
#include <stdarg.h>
#include <stdio.h>    

FATFS sSDCARD_FatFs;
void SYS_Init(void) {
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

static int utf8_decode(const uint8_t *p, const uint8_t *end, uint32_t *cp) {
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
		if (*p == '\r') {
			p++;
			continue;
		}
        if (*p == '\n') {
            cur_x = x;
            cur_y += 8;
            p++;
            continue;
        }

        // 解析 UTF-8 码点, 失败则替换为 U+FFFD
        uint32_t cp;
        int consumed = utf8_decode(p, end, &cp);
        if (consumed <= 0) {
            cp = 0xFFFD;     
            consumed = 1;     // 跳过当前非法字节
        }

        // 从字库读取9字节[宽度][点阵]
        f_lseek(&file_uni, (DWORD)cp * 9);
        UINT br;
        uint8_t char_data[9];
        f_read(&file_uni, char_data, 9, &br);

        uint8_t char_width = char_data[0];
        uint8_t *col_data = char_data + 1;

        // 如果缺少字形, 替换为U+25A1
        if (char_width == 0) {
            cp = 0x25A1;
            f_lseek(&file_uni, (DWORD)cp * 9);
            f_read(&file_uni, char_data, 9, &br);
            char_width = char_data[0];
            col_data = char_data + 1;

			// 无 U+25A1
            if (char_width == 0) {
                char_width = 8;
            }
        }

        // 自动换行判断
        if (cur_x + char_width > LCD_H) {
            cur_x = x;
            cur_y += 8;
        }
        if (cur_y + 8 > LCD_W) break;

        // 逐行批量发送像素 (纵向取模高位在下)
        for (uint8_t row = 0; row < 8; row++) {
            uint16_t row_buf[8];
            for (uint8_t col = 0; col < char_width; col++) {
                if (col_data[col] & (0x01 << row))
                    row_buf[col] = color;
                else
                    row_buf[col] = background_color;
            }

            LCD_SetWindows(cur_x, cur_y + row, cur_x + char_width - 1, cur_y + row);
            LCD_CS_CLR;
            LCD_RS_SET;
            SPI_SET_16BIT;
            for (uint8_t col = 0; col < char_width; col++)
                SPI_TRANSMIT_16BIT(row_buf[col]);
            SPI_WAIT();
            SPI_SET_8BIT;
            LCD_CS_SET;
        }

        cur_x += char_width;
        p += consumed;
    }

    f_close(&file_uni);
}
