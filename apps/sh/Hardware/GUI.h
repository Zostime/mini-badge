#ifndef __GUI_H__
#define __GUI_H__

/* 透明色设置 */
#define TRANSPARENT_ENABLE 0 // 0:关闭透明色 1:开启透明色
#define TRANSPARENT_COLOR 0xF81F

/* RGB565 Colors */
#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define BLUE    0x001F
#define MAGENTA 0xF81F
#define CYAN    0x07FF
#define WHITE   0xFFFF

#define BRIGHT_BLACK   0x8410   
#define BRIGHT_RED     0xFC10   
#define BRIGHT_GREEN   0x87E0   
#define BRIGHT_YELLOW  0xFFF0   
#define BRIGHT_BLUE    0x041F   
#define BRIGHT_MAGENTA 0xF81F  
#define BRIGHT_CYAN    0x07FF   
#define BRIGHT_WHITE   0xFFFF 


void GUI_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
void GUI_Fill(uint16_t xStart,uint16_t yStart,uint16_t xEnd,uint16_t yEnd,uint16_t Color);
void GUI_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t Color);
void GUI_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t Color);
void GUI_FillCircle(uint16_t xc, uint16_t yc, int r, uint16_t color);
void GUI_DrawCircle(uint16_t xc, uint16_t yc, int r, uint16_t color);
void GUI_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t Color);
void GUI_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

#endif
