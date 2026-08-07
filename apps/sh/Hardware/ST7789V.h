#ifndef __ST7789V_H__
#define __ST7789V_H__

/* Includes */
#include <stdint.h>
#include "spi.h" 

// 端口定义
#define LCD_GPIO_TYPE  GPIOB
#define LCD_CS_PIN     12    // 片选
#define LCD_RS_PIN     11    // 数据/命令
#define LCD_RST_PIN    14    // 复位

#define LCD_CS_SET     (LCD_GPIO_TYPE->BSRR = 1 << LCD_CS_PIN)
#define LCD_CS_CLR     (LCD_GPIO_TYPE->BRR  = 1 << LCD_CS_PIN)

#define LCD_RS_SET     (LCD_GPIO_TYPE->BSRR = 1 << LCD_RS_PIN)
#define LCD_RS_CLR     (LCD_GPIO_TYPE->BRR  = 1 << LCD_RS_PIN)

#define LCD_RST_SET    (LCD_GPIO_TYPE->BSRR = 1 << LCD_RST_PIN)
#define LCD_RST_CLR    (LCD_GPIO_TYPE->BRR  = 1 << LCD_RST_PIN)

#define LCD_W 135
#define LCD_H 240


// LCD重要参数集
typedef struct {										    
	uint16_t width;	      //LCD 宽度
	uint16_t height;	  //LCD 高度
	uint16_t id;	      //LCD ID
	uint8_t  dir;	      //横屏还是竖屏控制：0，竖屏；1，横屏。	
	uint16_t wramcmd; 	  //开始写gram指令
	uint16_t setxcmd;	  //设置x坐标指令
	uint16_t setycmd;	  //设置y坐标指令	
  uint8_t    xoffset;    
  uint8_t	 yoffset;
}_lcd_dev;

extern _lcd_dev lcddev;

typedef struct {
    uint8_t cmd;          // 命令
    uint8_t data[16];     // 参数
    uint8_t len;          // 参数长度
    uint16_t delay;       // 命令后延时
} LCD_InitCmd_t;


void LCD_Init(void);
void LCD_DeInit(void);
void LCD_SetBrightness(uint16_t brightness);
void LCD_Clear(uint16_t Color);
void LCD_direction(uint8_t direction);
void LCD_SetWindows(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);

/** 
  * @defgroup  SPI_TX 数据发送
  * @brief  SPI发送8|16位宽数据, 发送前需要使用SPI_SET_xBIT来设置位宽,
			发送一批数据后需调用SPI_WAIT()
  * @{
  */
#define SPI_SET_8BIT 			(SPI2->CR1 &= ~SPI_CR1_DFF)
#define SPI_SET_16BIT 			(SPI2->CR1 |=  SPI_CR1_DFF)	

#define SPI_TRANSMIT_8BIT(data)	 do { \
								   while (!(SPI2->SR & SPI_SR_TXE)); \
							       *(volatile uint8_t *)&SPI2->DR = (data); \
								   __nop(); \
								   __nop(); \
	                               __nop(); \
							     } while(0U)

#define SPI_TRANSMIT_16BIT(data)   do { \
									 while (!(SPI2->SR & SPI_SR_TXE)); \
									 SPI2->DR = (data); \
								   } while(0U)

#define SPI_WAIT()   do { \
					   while (SPI2->SR & SPI_SR_BSY); \
					   (void)SPI2->DR; \
					   (void)SPI2->SR; \
					 } while(0U)

/**
					 
  * @}
  */						   				   						   								  

#endif
