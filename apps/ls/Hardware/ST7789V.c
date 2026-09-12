#include "ST7789V.h"
#include "main.h"
#include "tim.h"

_lcd_dev lcddev;  

/**
  * @brief  发送1字节命令  
  * @param	cmd - 命令数据
  * @retval 无
  */
void LCD_WriteCmd(uint8_t cmd) {
    LCD_CS_CLR;
    LCD_RS_CLR;
    SPI_TRANSMIT_8BIT(cmd);
	SPI_WAIT();
    LCD_CS_SET;
}

/**
  * @brief    发送数据  
  * @param     data - 数据
  * @param     len - 数据长度
  * @retval 无
  */
void LCD_WriteData(uint8_t *data, uint16_t len) {
    LCD_CS_CLR;
    LCD_RS_SET;                       
    while (len--) SPI_TRANSMIT_8BIT(*data++);
	SPI_WAIT();
    LCD_CS_SET;
}

/**
  * @brief     给寄存器写入值
  * @param     LCD_Reg - 寄存器
  * @param     LCD_RegValue - 寄存器值
  * @param     LCD_RegValueLen - 寄存器值的长度
  * @retval
  */
void LCD_WriteReg(uint8_t LCD_Reg, uint8_t *LCD_RegValue, uint16_t LCD_RegValueLen) {    
    LCD_WriteCmd(LCD_Reg);  
    LCD_WriteData(LCD_RegValue, LCD_RegValueLen);                 
}     

/**
  * @brief     重置LCD屏幕 
  * @param     无
  * @retval    无
  */
void LCD_Reset(void) {
    LCD_RST_CLR;
    HAL_Delay(20);    
    LCD_RST_SET;
    HAL_Delay(20);
}

void LCD_direction(uint8_t direction)
{ 
	lcddev.setxcmd=0x2A;
	lcddev.setycmd=0x2B;
	lcddev.wramcmd=0x2C;
    switch(direction){          
        case 0:                                      
            lcddev.width=LCD_W;
            lcddev.height=LCD_H;    
            lcddev.xoffset=52;
            lcddev.yoffset=40;
            LCD_WriteReg(0x36,(uint8_t[]){0},1);            //BGR==1,MY==0,MX==0,MV==0
        break;
        case 1:
            lcddev.width=LCD_H;
            lcddev.height=LCD_W;
            lcddev.xoffset=40;
            lcddev.yoffset=53;
            LCD_WriteReg(0x36,(uint8_t[]){(1<<6)|(1<<5)},1);//BGR==1,MY==1,MX==0,MV==1
        break;
        case 2:                                      
            lcddev.width=LCD_W;
            lcddev.height=LCD_H;
              lcddev.xoffset=53;
            lcddev.yoffset=40;            
            LCD_WriteReg(0x36,(uint8_t[]){(1<<6)|(1<<7)},1);//BGR==1,MY==0,MX==0,MV==0
        break;
        case 3:
            lcddev.width=LCD_H;
            lcddev.height=LCD_W;
            lcddev.xoffset=40;
            lcddev.yoffset=52;
            LCD_WriteReg(0x36,(uint8_t[]){(1<<7)|(1<<5)},1);//BGR==1,MY==1,MX==0,MV==1
        break;    
        default:break;
    }        
}     

/**
  * @brief    调节LCD的亮度  
  * @param     brightness - duty范围: 0-1000 (对应0%-100%)
  * @retval 无
  */
void LCD_SetBrightness(uint16_t brightness) {
    if(brightness > 1000) brightness = 1000;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1000 - brightness);
}

const LCD_InitCmd_t ST7789V_InitSeq[] = {
    {0x11, {0}, 0, 120},    // Sleep out
    {0x36, {0x00}, 1, 0},   // MADCTL
    {0x3A, {0x05}, 1, 10},  // 16-bit/pixel 
    {0x21, {0}, 0, 0},      // Display Inversion
    {0xB2, {0x05, 0x05, 0x00, 0x33, 0x33}, 5, 10},
    {0xB7, {0x23}, 1, 10},
    {0xBB, {0x22}, 1, 10},
    {0xC0, {0x2C}, 1, 10},
    {0xC2, {0x01}, 1, 10},
    {0xC3, {0x13}, 1, 10},
    {0xC4, {0x20}, 1, 10},
    {0xC6, {0x0F}, 1, 10},
    {0xD0, {0xA4, 0xA1}, 2, 10},
    {0xD6, {0xA1}, 1, 10},
    {0xE0, {0x70,0x06,0x0C,0x08,0x09,0x27,0x2E,0x34,0x46,0x37,0x13,0x13,0x25,0x2A}, 14, 0},
    {0xE1, {0x70,0x04,0x08,0x09,0x07,0x03,0x2C,0x42,0x42,0x38,0x14,0x14,0x27,0x2C}, 14, 0},
    {0x29, {0}, 0, 0},      // Display ON
    {0x00, {0}, 0, 0}       // 结束标记
};

/**
  * @brief     初始化LCD 
  * @param     无
  * @retval    无
  */
void LCD_Init(void) {  
	__HAL_SPI_ENABLE(&hspi2);	// 使能SPI2	
    // 背光初始化
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	__HAL_TIM_MOE_ENABLE(&htim1);
	LCD_SetBrightness(0);

    LCD_Reset();	// 硬件复位
    
    const LCD_InitCmd_t *cmd = ST7789V_InitSeq;
    while (cmd->cmd != 0x00) {
        LCD_WriteCmd(cmd->cmd);
        for (int i = 0; i < cmd->len; i++) {
            uint8_t param = cmd->data[i];
            LCD_WriteData(&param, 1);
        }
        if (cmd->delay > 0) {
            HAL_Delay(cmd->delay);
        }
        cmd++;
    }

	LCD_direction(1);
}

/**
  * @brief     去初始化LCD 
  * @param    无
  * @retval 无
  */
void LCD_DeInit() {
    __HAL_SPI_DISABLE(&hspi2);    // 失能SPI2	
	LCD_SetBrightness(0);
}

void LCD_SetWindows(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
    LCD_WriteCmd(lcddev.setxcmd);
    LCD_WriteData((uint8_t[]) {
        (xStart + lcddev.xoffset) >> 8,
         xStart + lcddev.xoffset,
        (xEnd  + lcddev.xoffset) >> 8,
         xEnd  + lcddev.xoffset
    }, 4);	// 列地址设置

    LCD_WriteCmd(lcddev.setycmd);
    LCD_WriteData((uint8_t[]) {
        (yStart + lcddev.yoffset) >> 8,
         yStart + lcddev.yoffset,
        (yEnd  + lcddev.yoffset) >> 8,
         yEnd  + lcddev.yoffset
    }, 4);	// 行地址设置
	
    LCD_WriteCmd(lcddev.wramcmd);	// 写入内存
}

void LCD_Clear(uint16_t Color) {
    LCD_SetWindows(0, 0, lcddev.width-1, lcddev.height-1);
    LCD_CS_CLR;
    LCD_RS_SET;
	SPI_SET_16BIT;  
    for (uint16_t i = 0; i < LCD_W * LCD_H; i++) {
        SPI_TRANSMIT_16BIT(Color);
    }
    SPI_WAIT();
	SPI_SET_8BIT;  
    LCD_CS_SET;
}
