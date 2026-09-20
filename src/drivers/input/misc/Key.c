#include "main.h"
#include "tim.h"
#include "Key.h"
#include <kernel/input.h>

uint8_t Key_Flag[KEY_COUNT];

static const uint16_t key_code[KEY_COUNT] = {
    KEY_A, KEY_B, KEY_C
};

static void key_emit(uint8_t n, uint8_t flag)
{
    uint16_t code = key_code[n];
    if      (flag & KFLAG_DOWN)   input_event_push(EV_KEY, code, 1);
    else if (flag & KFLAG_UP)     input_event_push(EV_KEY, code, 0);
    else if (flag & KFLAG_LONG)   input_event_push(EV_KEY, code, 2);
    else if (flag & KFLAG_REPEAT) input_event_push(EV_KEY, code, 2);
    else if (flag & KFLAG_SINGLE) input_event_push(EV_KEY, code, 1);
}

void Key_Init(void) {
	
	HAL_TIM_Base_Start_IT(&htim2);
} 

uint8_t Key_GetState(uint8_t n) {
    if (n == KEY_1)      return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET) ? KEY_PRESSED : KEY_UNPRESSED;
    else if (n == KEY_2) return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_SET) ? KEY_PRESSED : KEY_UNPRESSED;
    else if (n == KEY_3) return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET) ? KEY_PRESSED : KEY_UNPRESSED;
    return KEY_UNPRESSED;
}

uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
	if (Key_Flag[n] & Flag)
	{
		if (Flag != KFLAG_HOLD)
		{
			Key_Flag[n] &= ~Flag;
		}
		return 1;
	}
	return 0;
}

void Key_Tick(void)
{
	static uint8_t Count, i;
	static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];
	static uint8_t S[KEY_COUNT];
	static uint16_t Time[KEY_COUNT];
	
	for (i = 0; i < KEY_COUNT; i ++)
	{
		if (Time[i] > 0)
		{
			Time[i] --;
		}
	}
	
	Count ++;
	if (Count >= 20)
	{
		Count = 0;
		
		for (i = 0; i < KEY_COUNT; i ++)
		{
			PrevState[i] = CurrState[i];
			CurrState[i] = Key_GetState(i);
			
			if (CurrState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KFLAG_HOLD;
			}
			else
			{
				Key_Flag[i] &= ~KFLAG_HOLD;
			}
			
			if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
			{
				Key_Flag[i] |= KFLAG_DOWN;
				key_emit(i, KFLAG_DOWN);
			}
			
			if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KFLAG_UP;
				key_emit(i, KFLAG_UP);
			}
			
			if (S[i] == 0)
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Time[i] = KEY_TIME_LONG;
					S[i] = 1;
				}
			}
			else if (S[i] == 1)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					Time[i] = KEY_TIME_DOUBLE;
					S[i] = 2;
				}
				else if (Time[i] == 0)
				{
					Time[i] = KEY_TIME_REPEAT;
					Key_Flag[i] |= KFLAG_LONG;
					key_emit(i, KFLAG_LONG);
					S[i] = 4;
				}
			}
			else if (S[i] == 2)
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Key_Flag[i] |= KFLAG_DOUBLE;
					S[i] = 3;
				}
				else if (Time[i] == 0)
				{
					Key_Flag[i] |= KFLAG_SINGLE;
					key_emit(i, KFLAG_SINGLE);
					S[i] = 0;
				}
			}
			else if (S[i] == 3)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					S[i] = 0;
				}
			}
			else if (S[i] == 4)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					S[i] = 0;
				}
				else if (Time[i] == 0)
				{
					Time[i] = KEY_TIME_REPEAT;
					Key_Flag[i] |= KFLAG_REPEAT;
					key_emit(i, KFLAG_REPEAT);
					S[i] = 4;
				}
			}
		}
	}
}

uint8_t Key_GetStatus(void) {
    if (Key_Check(KEY_1, KFLAG_SINGLE)) return KEY1_SINGLE;
    if (Key_Check(KEY_1, KFLAG_LONG))   return KEY1_LONG;
    if (Key_Check(KEY_1, KFLAG_REPEAT)) return KEY1_REPEAT;
    if (Key_Check(KEY_2, KFLAG_SINGLE)) return KEY2_SINGLE;
    if (Key_Check(KEY_2, KFLAG_LONG))   return KEY2_LONG;
    if (Key_Check(KEY_2, KFLAG_REPEAT)) return KEY2_REPEAT;
    if (Key_Check(KEY_3, KFLAG_SINGLE)) return KEY3_SINGLE;
    if (Key_Check(KEY_3, KFLAG_LONG))   return KEY3_LONG;
    if (Key_Check(KEY_3, KFLAG_REPEAT)) return KEY3_REPEAT;
    return KEY_NONE;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        Key_Tick(); 
    }
}
