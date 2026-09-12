#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>

void Buzzer_Init(void);
void Buzzer_SetFreq(uint32_t freq_hz);
void Buzzer_SetVolume(uint8_t volume);
uint8_t Buzzer_GetVolume(void);
void Buzzer_Off(void);

#endif
