#include "system.h"
#include "ff.h"
#include "ST7789V.h"
#include "GUI.h"
#include <stdarg.h>
#include <stdio.h>

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

void SYS_Printf(uint16_t x, uint16_t y, uint16_t color, uint16_t background_color, const char *fmt, ...) {
    char buf[SYS_PRINT_BUFFER_SIZE];                  
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    FIL font_file;
    if (f_open(&font_file, "0:/sys/fonts/ASC_6x8", FA_READ) != FR_OK) return;

    uint16_t cur_x = x, cur_y = y;
    const char *p = buf;
    uint8_t col_data[6];          
    uint16_t row_buf[6];                    

    while (*p) {
        if (*p == '\n') {
            cur_x = x;
            cur_y += 8;
            p++;
            continue;
        }

        if (cur_x + 6 > LCD_H) {
            cur_x = x;
            cur_y += 8;
        }
        if (cur_y + 8 > LCD_W) break;

        char ch = *p++;
        if (ch < 32 || ch > 126) ch = 32;

        f_lseek(&font_file, (DWORD)(ch - 32) * 6);
        UINT br;
        f_read(&font_file, col_data, 6, &br);

        for (uint8_t row = 0; row < 8; row++) {
            for (uint8_t col = 0; col < 6; col++) {
                if (col_data[col] & (0x01 << row))
                    row_buf[col] = color;
                else
                    row_buf[col] = background_color;
            }

            LCD_SetWindows(cur_x, cur_y + row, cur_x + 5, cur_y + row);
            LCD_CS_CLR;
            LCD_RS_SET;
            SPI_SET_16BIT;
            for (uint8_t col = 0; col < 6; col++) {
                SPI_TRANSMIT_16BIT(row_buf[col]);
            }
            SPI_WAIT();
            SPI_SET_8BIT;
            LCD_CS_SET;
        }
        cur_x += 6;
    }
    f_close(&font_file);
}
