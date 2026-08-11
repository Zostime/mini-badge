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

void SYS_Printf(uint16_t x, uint16_t y, uint16_t color, uint16_t background_color, const char *fmt, ...)
{
    char buf[SYS_PRINT_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    FIL file_asc, file_gb;
    uint8_t gb_opened = 0;

    if (f_open(&file_asc, "0:/sys/fonts/ASC_DEFAULT_6x8", FA_READ) != FR_OK)
        return;
    gb_opened = (f_open(&file_gb, "0:/sys/fonts/GB2312_DEFAULT_8x8", FA_READ) == FR_OK);

    uint16_t cur_x = x, cur_y = y;
    const char *p = buf;

    while (*p)
    {
        if (*p == '\n') {
            cur_x = x;
            cur_y += 8;	// 行高统一 8
            p++;
            continue;
        }

        FIL *fp;
        uint8_t w, h;
        uint8_t col_data[16];             
        uint32_t offset;

        // ---- ASCII (< 0x80) ----
        if ((uint8_t)*p < 0x80) {
            char ch = *p;
            if (ch < 32 || ch > 126) ch = 32;
            offset = (uint32_t)(ch - 32) * 6;
            fp = &file_asc;
            w = 6; h = 8;

            f_lseek(fp, offset);
            UINT br;
            f_read(fp, col_data, 6, &br);
            p++;
        }
        // GB2312 汉字 (>= 0xA1)
        else if (gb_opened && (uint8_t)*p >= 0xA1) {
            uint8_t hi = (uint8_t)*p;
            uint8_t lo = (uint8_t)*(p + 1);
            // 区码 = hi - 0xA0, 位码 = lo - 0xA0
            // 索引 = (区码-1)*94 + (位码-1) = (hi - 0xA1)*94 + (lo - 0xA1)
            offset = (uint32_t)((hi - 0xA1) * 94 + (lo - 0xA1)) * 8;
            fp = &file_gb;
            w = 8; h = 8;

            f_lseek(fp, offset);
            UINT br;
            f_read(fp, col_data, 8, &br);
            p += 2;
        }
        else {
            p++;
            continue;
        }

        if (cur_x + w > LCD_H) {
            cur_x = x;
            cur_y += h;
        }
        if (cur_y + h > LCD_W) break;

        for (uint8_t row = 0; row < h; row++) {
            uint16_t row_buf[16];	// 行缓冲区，最大 16 列
            for (uint8_t col = 0; col < w; col++) {
                // 纵向取模, 高位在下
                if (col_data[col] & (0x01 << row))
                    row_buf[col] = color;
                else
                    row_buf[col] = background_color;
            }

            LCD_SetWindows(cur_x, cur_y + row, cur_x + w - 1, cur_y + row);
            LCD_CS_CLR;
            LCD_RS_SET;
            SPI_SET_16BIT;
            for (uint8_t col = 0; col < w; col++)
                SPI_TRANSMIT_16BIT(row_buf[col]);
            SPI_WAIT();
            SPI_SET_8BIT;
            LCD_CS_SET;
        }
        cur_x += w;
    }

    f_close(&file_asc);
    if (gb_opened) f_close(&file_gb);
}
