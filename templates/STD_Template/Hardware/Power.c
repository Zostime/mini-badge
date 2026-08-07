#include "stm32f10x.h"
#include "Power.h"

// 初始化充电检测
void Power_Charging_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PA2 (CHRG) 输入模式，上拉（因为CHRG是开漏输出，低电平有效）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA3 (STDBY) 输入模式，上拉（因为STDBY是开漏输出，低电平有效）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 获取充电状态
Charging_Status Power_Charging_GetStatus(void)
{
    uint8_t chrg_state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);  // CHRG引脚
    uint8_t stdby_state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3); // STDBY引脚
    
    if (chrg_state == 0 && stdby_state == 1) {
        return CHARGING_CHARGING;    // 充电中
    } else if (chrg_state == 1 && stdby_state == 0) {
        return CHARGING_FULL;        // 充电完成
    } else if (chrg_state == 1 && stdby_state == 1) {
        return CHARGING_NO_INPUT;  	 //未充电或无电池
    } else {
        return CHARGING_ERROR;       // 异常状态
    }
}

const char* Power_Charging_GetStatusString(Charging_Status status)
{
    switch(status) {
        case CHARGING_CHARGING:    return "Charging";
        case CHARGING_FULL:        return "Full";
        case CHARGING_NO_INPUT:    return "No Input";
        case CHARGING_ERROR:       return "Error";
        default:                   return "Unknown";
    }
}

void Power_ADC_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开启GPIOA和ADC1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
    // 配置ADC时钟，PCLK2 72MHz 6分频 = 12MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    // 配置PA0和PA1为模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // ADC复位
    ADC_DeInit(ADC1);
    
    // ADC初始化配置
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 使能ADC
    ADC_Cmd(ADC1, ENABLE);
    
    // ADC校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
  * @brief  读取指定通道的ADC值（单次转换）
  * @param  channel: ADC通道
  * @retval ADC转换值
  */
uint16_t Power_ADC_ReadSingleChannel(uint8_t channel)
{
    // 配置规则组通道
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_239Cycles5);
    
    // 启动转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    // 等待转换结束
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    return ADC_GetConversionValue(ADC1);
}

/**
  * @brief  读取指定通道的ADC平均值
  * @param  channel: ADC通道
  * @param  sample_times: 采样次数
  * @retval ADC平均值
  */
uint16_t Power_ADC_ReadChannelAverage(uint8_t channel, uint8_t sample_times)
{
    uint32_t value_sum = 0;
    
    for(uint8_t i = 0; i < sample_times; i++)
    {
        value_sum += Power_ADC_ReadSingleChannel(channel);
    }
    
    return (uint16_t)(value_sum / sample_times);
}

/**
  * @brief  获取原始电压值
  * @param  channel: ADC通道
  * @retval 电压值(V)
  */
float Power_ADC_GetRawVoltage(uint8_t channel)
{
    uint16_t adc_value = Power_ADC_ReadChannelAverage(channel, 16);
    return (adc_value * 3.3f / 4095.0f);
}

/**
  * @brief  获取电池电压（经过分压计算）
  * @param  None
  * @retval 电池电压(V)
  */
float Power_ADC_GetBatteryVoltage(void)
{
    float raw_voltage = Power_ADC_GetRawVoltage(ADC_CH_BATTERY);
    return raw_voltage * VDIV_RATIO;
}

/**
  * @brief  获取3.3V参考电压
  * @param  None
  * @retval 3.3V参考电压值(V)
  */
float Power_ADC_GetVrefVoltage(void)
{
    return Power_ADC_GetRawVoltage(ADC_CH_VREF);
}

void Power_Init()
{
	Power_Charging_Init();
	Power_ADC_Init();
}
