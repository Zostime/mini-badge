#include "ST7789V.h"
#include "string.h"
#include "gui.h"
#include <stdarg.h>
#include <stdio.h>
#include "spi.h"

void GUI_DrawPoint(uint16_t x, uint16_t y, uint16_t Color)
{
	LCD_SetWindows(x,y,x,y);	
	LCD_CS_CLR;
    LCD_RS_SET;
	SPI_TRANSMIT_8BIT(Color >> 8);
	SPI_TRANSMIT_8BIT(Color);
	SPI_WAIT();
	LCD_CS_SET;	// DEBUG: 若有噪点可能需要注释
}

void GUI_Fill(uint16_t xStart,uint16_t yStart,uint16_t xEnd,uint16_t yEnd,uint16_t Color)
{  			
	LCD_SetWindows(xStart,yStart,xEnd,yEnd);	// 设置显示窗口
	
	LCD_CS_CLR;
    LCD_RS_SET;
	SPI_SET_16BIT;
	for(uint16_t i=0; i<(xEnd - xStart + 1) * (yEnd - yStart + 1); i++) {
		SPI_TRANSMIT_16BIT(Color); 	 
	}
	SPI_WAIT();
	SPI_SET_8BIT;  
    LCD_CS_SET;     
}

void GUI_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t Color)
{
	uint16_t t; 
	int xerr=0, yerr=0, delta_x, delta_y, distance; 
	int incx, incy, uRow, uCol; 

	delta_x = x2-x1;	// 计算坐标增量 
	delta_y = y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x > 0) incx=1;	// 设置单步方向 
	else if(delta_x == 0) incx=0;	// 垂直线 
	else {incx = -1; delta_x = -delta_x;} 
	if(delta_y > 0) incy=1; 
	else if(delta_y == 0) incy=0;	// 水平线 
	else {incy = -1; delta_y = -delta_y;} 
	if( delta_x > delta_y) distance = delta_x;	// 选取基本增量坐标轴 
	else distance = delta_y; 
	for(t = 0; t <= distance+1; t++ )	// 画线输出 
	{  
		LCD_SetWindows(uRow, uCol, uRow, uCol);
		LCD_CS_CLR;
		LCD_RS_SET;
		SPI_TRANSMIT_8BIT(Color >> 8);
		SPI_TRANSMIT_8BIT(Color);
		
		xerr += delta_x; 
		yerr += delta_y; 
		if(xerr > distance) 
		{ 
			xerr -= distance; 
			uRow += incx; 
		} 
		if(yerr > distance) 
		{ 
			yerr -= distance; 
			uCol += incy; 
		} 
	}  
	SPI_WAIT();
	LCD_CS_SET;
} 

void GUI_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t Color) {
    if (x1 > x2) { uint16_t t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }

    LCD_SetWindows(x1, y1, x2, y1);
    LCD_CS_CLR; LCD_RS_SET;
    for (uint16_t i = x1; i <= x2; i++) {
        SPI_TRANSMIT_8BIT(Color >> 8);
        SPI_TRANSMIT_8BIT(Color);
    }
    SPI_WAIT();  

    LCD_SetWindows(x1, y2, x2, y2);
    LCD_CS_CLR; LCD_RS_SET;
    for (uint16_t i = x1; i <= x2; i++) {
        SPI_TRANSMIT_8BIT(Color >> 8);
        SPI_TRANSMIT_8BIT(Color);
    }
    SPI_WAIT();

    if(y2 > y1 + 1) {
        LCD_SetWindows(x1, y1+1, x1, y2-1);
        LCD_CS_CLR; LCD_RS_SET;
        for(uint16_t j = y1+1; j <= y2-1; j++) {
            SPI_TRANSMIT_8BIT(Color >> 8);
            SPI_TRANSMIT_8BIT(Color);
        }
        SPI_WAIT();
    }

    if(y2 > y1 + 1) {
        LCD_SetWindows(x2, y1+1, x2, y2-1);
        LCD_CS_CLR; LCD_RS_SET;
        for(uint16_t j = y1+1; j <= y2-1; j++) {
            SPI_TRANSMIT_8BIT(Color >> 8);
            SPI_TRANSMIT_8BIT(Color);
        }
        SPI_WAIT();
    }
}

void GUI_FillCircle(uint16_t xc, uint16_t yc, int r, uint16_t color) {
    int x = 0, y = r, d = 3-2*r;

    while(x <= y) {
        GUI_Fill(xc - x, yc - y, xc + x, yc - y, color);
        GUI_Fill(xc - y, yc - x, xc + y, yc - x, color);
        GUI_Fill(xc - x, yc + y, xc + x, yc + y, color);
        GUI_Fill(xc - y, yc + x, xc + y, yc + x, color);

        if(d < 0) {
            d += 4*x+6;
        } 
		else {
            d += 4*(x-y)+10;
            y--;
        }
        x++;
    }
    if(r == 0) GUI_DrawPoint(xc, yc, color);
}

void GUI_DrawCircle(uint16_t xc, uint16_t yc, int r, uint16_t color) {
    int x = 0, y = r, d = 3-2*r;

    while (x <= y) {
        GUI_DrawPoint(xc + x, yc + y, color);
        GUI_DrawPoint(xc - x, yc + y, color);
        GUI_DrawPoint(xc + x, yc - y, color);
        GUI_DrawPoint(xc - x, yc - y, color);
        GUI_DrawPoint(xc + y, yc + x, color);
        GUI_DrawPoint(xc - y, yc + x, color);
        GUI_DrawPoint(xc + y, yc - x, color);
        GUI_DrawPoint(xc - y, yc - x, color);

        if(d < 0) {
            d += 4*x+6;
        } 
		else {
            d += 4*(x-y)+10;
            y--;
        }
        x++;
    }
}       

void GUI_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2, uint16_t Color)
{
    GUI_DrawLine(x0, y0, x1, y1, Color);
    GUI_DrawLine(x1, y1, x2, y2, Color);
    GUI_DrawLine(x2, y2, x0, y0, Color);
}

void GUI_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2, uint16_t color)
{
    if(y0 > y1) {uint16_t t; t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t;}
    if(y0 > y2) {uint16_t t; t = x0; x0 = x2; x2 = t; t = y0; y0 = y2; y2 = t;}
    if(y1 > y2) {uint16_t t; t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t;}

    int32_t dx1 = (y1 - y0) ? (int32_t)(x1 - x0) * 1000 / (y1 - y0) : 0;
    int32_t dx2 = (y2 - y0) ? (int32_t)(x2 - x0) * 1000 / (y2 - y0) : 0;
    int32_t dx3 = (y2 - y1) ? (int32_t)(x2 - x1) * 1000 / (y2 - y1) : 0;

    int32_t sx = x0 * 1000, ex = x0 * 1000;

    if(y0 < y1) {
		for(uint16_t y = y0; y <= y1; y++) {
			uint16_t xs = (sx + 500) / 1000;
			uint16_t xe = (ex + 500) / 1000;
			if(xs <= xe) GUI_Fill(xs, y, xe, y, color);
			else         GUI_Fill(xe, y, xs, y, color);
			sx += dx1; ex += dx2;
        }
    }

    sx = x1 * 1000;
	if(y1 < y2) {
        for(uint16_t y = y1; y <= y2; y++) {
            uint16_t xs = (sx + 500) / 1000;
            uint16_t xe = (ex + 500) / 1000;
            if(xs <= xe) GUI_Fill(xs, y, xe, y, color);
            else         GUI_Fill(xe, y, xs, y, color);
            sx += dx3; ex += dx2;
        }
    }
}

/* TEMP */

#include "FONT.H"

#define GUI_PRINT_BUFFER_SIZE 128

void GUI_Printf(uint16_t x, uint16_t y, uint16_t color,uint16_t background_color, const char *fmt, ...) {
    char buf[GUI_PRINT_BUFFER_SIZE];                  
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args); 
    va_end(args);

    uint16_t cur_x = x, cur_y = y;
    const char *p = buf;

    while (*p) {
        if (*p == '\n') {          
            cur_x = x;
            cur_y += 12;
            p++;
            continue;
        }

        if (cur_x + 6 > LCD_H) {
            cur_x = x;
            cur_y += 12;
        }
        if (cur_y + 12 > LCD_W) break;  

		char ch = *p++;
		if (ch < 32 || ch > 126) ch = 32; // 空格
		uint8_t index = ch - 32;
		const uint8_t *pData = asc2_1206[index]; // 12字节
		for (int row = 0; row < 12; row++) {
			uint8_t line = pData[row];
			for (int col = 0; col < 6; col++) {
				if (line & (0x01 << col)) { 
					GUI_DrawPoint(cur_x + col, cur_y + row, color);
				}
				else {
					GUI_DrawPoint(cur_x + col, cur_y + row, background_color);
				}
			}
		}
        cur_x += 6;
    }
}
