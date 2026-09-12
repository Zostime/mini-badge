#ifndef __POWER_H__
#define __POWER_H__

#include "main.h"

// 分压电阻定义
#define R1_VALUE      47000.0f  // 47kΩ
#define R2_VALUE      100000.0f // 100kΩ
#define VDIV_RATIO    ((R1_VALUE + R2_VALUE) / R2_VALUE)  // 分压比

// ADC通道定义
#define ADC_CH_BATTERY    ADC_CHANNEL_0  // PA0 - 电池电压检测
#define ADC_CH_VREF       ADC_CHANNEL_1  // PA1 - 3.3V参考电压

// 充电状态枚举
typedef enum {
    CHARGING_UNKNOWN = 0,    // 未知状态
    CHARGING_CHARGING,       // 充电中
    CHARGING_FULL,           // 充电完成
    CHARGING_NO_INPUT,       // 未充电或无电池
    CHARGING_ERROR           // 硬件错误
} Charging_Status;
             
#define Power_Init()	HAL_ADCEx_Calibration_Start(&hadc1)
Charging_Status Power_GetChargeStatus(void);
float Power_GetBatteryVoltage(void);
float Power_GetVrefVoltage(void);

#endif
