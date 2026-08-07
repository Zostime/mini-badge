#include "system.h"
#include "ff.h"
#include "ST7789V.h"
#include "GUI.h"
#include <stdlib.h>

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
	
	#if SYS_USE_BOOTLOGO
	SYS_DisplayBMP(0, 0, SYS_BOOTLOGO_PATH);
	for(uint16_t i=0;i<1000;i++)
	{
		LCD_SetBrightness(i);
		HAL_Delay(2);
	}
	#else
	LCD_Clear(BLACK);
	LCD_SetBrightness(1000);
	#endif
}

#pragma pack(push, 1)
typedef struct BITMAPFILEHEADER {
    uint16_t bfType;// 文件的类型，该值必需是0x4D42，也就是字符'BM'。
    uint32_t bfSize;// 位图文件的大小，用字节为单位
    uint16_t bfReserved1;// 保留，必须设置为0
    uint16_t bfReserved2;// 保留，必须设置为0
    uint32_t bfOffBits;// 位图数据距离文件开头偏移量，用字节为单位
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER {
    uint32_t biSize;//位图信息数据头
    int32_t  biWidth;//图像宽度
    int32_t  biHeight;//图像高度
    uint16_t biPlanes;//色彩平面数量，必须为1
    uint16_t biBitCount;//每个像素存储的位数
    uint32_t biCompression;//压缩方式，0表示不压缩
    uint32_t biSizeImage;//原始位图数据的大小
    int32_t  biXPelsPerMeter;//横向分辨率
    int32_t  biYPelsPerMeter;//纵向分辨率
    uint32_t biClrUsed;//调色板颜色数
    uint32_t biClrImportant;//重要颜色数
} BITMAPINFOHEADER;
#pragma pack(pop)

FRESULT SYS_DisplayBMP(uint16_t x, uint16_t y, const char *path)
{   	
    UINT bytes_read;
    FIL file;
    FRESULT res = f_open(&file, path, FA_READ);
    if (res != FR_OK) return res;

    BITMAPFILEHEADER bmpFileHeader;
    res = f_read(&file, &bmpFileHeader, sizeof(BITMAPFILEHEADER), &bytes_read);
    if (res != FR_OK || bytes_read != sizeof(BITMAPFILEHEADER) || bmpFileHeader.bfType != 0x4D42) {
        f_close(&file);
        return FR_INT_ERR;
    }

    BITMAPINFOHEADER bmpInfoHeader;
    res = f_read(&file, &bmpInfoHeader, sizeof(BITMAPINFOHEADER), &bytes_read);
    if (res != FR_OK || bytes_read != sizeof(BITMAPINFOHEADER)) {
        f_close(&file);
        return FR_INT_ERR;
    }

    int32_t width  = bmpInfoHeader.biWidth;
    int32_t height = bmpInfoHeader.biHeight;
    uint16_t bpp   = bmpInfoHeader.biBitCount;

    uint32_t rowSize;
    switch (bpp) {
        case 16: rowSize = ((width * 2) + 3) & ~3; break;
        case 24: rowSize = ((width * 3) + 3) & ~3; break;
        case 32: rowSize = ((width * 4) + 3) & ~3; break;
        default: f_close(&file); return FR_INT_ERR;
    }

    uint8_t rowBuffer[240 * 4 + 4];  
    if (rowSize > sizeof(rowBuffer)) {
        f_close(&file);
        return FR_INT_ERR;           
    }

    int32_t row, row_start, row_end, row_step;
    if (height > 0) {      
        row_start = height - 1;
        row_end = -1;
        row_step = -1;
    } else {       
        height = -height;
        row_start = 0;
        row_end = height;
        row_step = 1;
    }

    res = f_lseek(&file, bmpFileHeader.bfOffBits);
    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    LCD_SetWindows(x, y, x + width - 1, y + height - 1);
    LCD_CS_CLR;
    LCD_RS_SET;
    SPI_SET_16BIT;

    uint8_t bytesPerPixel = bpp / 8;

    for (row = row_start; row != row_end; row += row_step) {
        res = f_lseek(&file, bmpFileHeader.bfOffBits + row * rowSize);
        if (res != FR_OK) break;

        res = f_read(&file, rowBuffer, rowSize, &bytes_read);
        if (res != FR_OK || bytes_read != rowSize) break;

        for (int32_t col = 0; col < width; col++) {
            uint8_t *p = rowBuffer + col * bytesPerPixel;
            uint16_t pixel;
            if (bytesPerPixel == 2) {
                pixel = p[0] | (p[1] << 8);        
            } else {
                uint8_t b = p[0], g = p[1], r = p[2];
                pixel = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            }
            SPI_TRANSMIT_16BIT(pixel);
        }
    }

    SPI_WAIT();
    SPI_SET_8BIT;
    LCD_CS_SET;

    f_close(&file);
    return res;
}
