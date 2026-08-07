#ifndef __Power_H__
#define __Power_H__

// 分压电阻定义
#define R1_VALUE      47000.0f  // 47kΩ
#define R2_VALUE      100000.0f // 100kΩ
#define VDIV_RATIO    ((R1_VALUE + R2_VALUE) / R2_VALUE)  // 分压比

// ADC通道定义
#define ADC_CH_BATTERY    ADC_Channel_0  // PA0 - 电池电压检测
#define ADC_CH_VREF       ADC_Channel_1  // PA1 - 3.3V参考电压

// 充电状态枚举
typedef enum {
	CHARGING_UNKNOWN = 0,    // 未知状态
	CHARGING_CHARGING,       // 充电中
	CHARGING_FULL,           // 充电完成
	CHARGING_NO_INPUT,    	 // 未充电或无电池
	CHARGING_ERROR           // 硬件错误
} Charging_Status;

void Power_Init(void);
// TP4056函数声明
Charging_Status Power_Charging_GetStatus(void);
const char* Power_Charging_GetStatusString(Charging_Status status);

// ADC函数声明
uint16_t Power_ADC_ReadSingleChannel(uint8_t channel);
uint16_t Power_ADC_ReadChannelAverage(uint8_t channel, uint8_t sample_times);
float Power_ADC_GetBatteryVoltage(void);
float Power_ADC_GetVrefVoltage(void);
float Power_ADC_GetRawVoltage(uint8_t channel);

#endif
