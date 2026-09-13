#include "buzzer.h"
#include "tim.h"

static uint8_t buzzer_volume = 100; 

void Buzzer_Init(void) {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
}

void Buzzer_SetFreq(uint32_t freq) {
    if (freq < 20) freq = 20;
    if (freq > 20000) freq = 20000;

    uint32_t psc, arr;

    // 根据频率选择合适的分频
    if (freq >= 1000) {
        psc = 0;                // 72 MHz
    } else if (freq >= 100) {
        psc = 3;                // 18 MHz (72M/(3+1)=18M)
    } else {
        psc = 19;               // 3.6 MHz (72M/(19+1)=3.6M)
    }

    uint32_t timer_clk = 72000000 / (psc + 1);
    arr = timer_clk / freq - 1;
    if (arr > 65535) arr = 65535;
    uint32_t ccr = (arr * buzzer_volume) / 100; 

    TIM3->CR1 &= ~TIM_CR1_CEN;          
    TIM3->PSC = psc;
    TIM3->ARR = arr;
    TIM3->CCR4 = ccr;
    TIM3->EGR = TIM_EGR_UG;           
    TIM3->CR1 |= TIM_CR1_CEN;        

    if (!(TIM3->CCER & TIM_CCER_CC4E)) {
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    }
}

void Buzzer_SetVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    buzzer_volume = volume;
    if (TIM3->CR1 & TIM_CR1_CEN) {
        uint32_t arr = TIM3->ARR;
        uint32_t ccr = (arr * volume) / 100;
        TIM3->CCR4 = ccr;
    }
}

uint8_t Buzzer_GetVolume(void) {
	return buzzer_volume;
}

void Buzzer_Off(void) {
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
}
