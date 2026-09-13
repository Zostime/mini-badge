#include "power.h"
#include "adc.h"  

Charging_Status Power_GetChargeStatus(void)
{
    GPIO_PinState chrg  = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);
    GPIO_PinState stdby = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);

    if (chrg == GPIO_PIN_RESET && stdby == GPIO_PIN_SET)
        return CHARGING_CHARGING;
    else if (chrg == GPIO_PIN_SET && stdby == GPIO_PIN_RESET)
        return CHARGING_FULL;
    else if (chrg == GPIO_PIN_SET && stdby == GPIO_PIN_SET)
        return CHARGING_NO_INPUT;
    else
        return CHARGING_ERROR;
}

static uint16_t ADC_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    HAL_ADC_Stop(&hadc1);
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);

    uint16_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

static uint16_t ADC_ReadAverage(uint32_t channel, uint8_t times)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < times; i++)
        sum += ADC_ReadChannel(channel);
    return (uint16_t)(sum / times);
}

static float ADC_ToVoltage(uint16_t adc)
{
    return adc * 3.3f / 4095.0f;
}

float Power_GetBatteryVoltage(void)
{
    uint16_t adc = ADC_ReadAverage(ADC_CH_BATTERY, 16);
    return ADC_ToVoltage(adc) * VDIV_RATIO;
}

float Power_GetVrefVoltage(void)
{
    uint16_t adc = ADC_ReadAverage(ADC_CH_VREF, 16);
    return ADC_ToVoltage(adc);
}
