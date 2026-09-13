#ifndef __KEY_H
#define __KEY_H

#define KEY_PRESSED				1
#define KEY_UNPRESSED			0

#define KEY_TIME_DOUBLE			0
#define KEY_TIME_LONG			500
#define KEY_TIME_REPEAT			100

#define KEY_COUNT				3

#define KEY_1					0
#define KEY_2					1
#define KEY_3					2

#define KEY_HOLD				0x01
#define KEY_DOWN				0x02
#define KEY_UP					0x04
#define KEY_SINGLE				0x08
#define KEY_DOUBLE				0x10
#define KEY_LONG				0x20
#define KEY_REPEAT				0x40

#define KEY_NONE 	  0
#define KEY1_SINGLE   1
#define KEY2_SINGLE   2
#define KEY3_SINGLE   3
#define KEY1_LONG     4
#define KEY2_LONG     5
#define KEY3_LONG     6
#define KEY1_REPEAT   7
#define KEY2_REPEAT   8
#define KEY3_REPEAT   9

void Key_Init(void);
uint8_t Key_Check(uint8_t n, uint8_t Flag);
void Key_Tick(void);
uint8_t Key_GetStatus(void);

#endif
