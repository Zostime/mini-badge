#include "stm32f10x.h"                  // Device header

#include "Delay.h" 
#include "MyRTC.h"
#include "bootloader_api.h"

#include "ff.h"
#include "gui.h"

#include "lcd.h"
#include "Key.h"
#include "Buzzer.h"
#include "spi_sdcard.h" 
#include "Power.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int main(void)
{	
	 
	
	LCD_Init();
	Key_Init();
	Buzzer_Init();
	Power_Init();
	MyRTC_Init();
	while(1)
	{   
	}
}
