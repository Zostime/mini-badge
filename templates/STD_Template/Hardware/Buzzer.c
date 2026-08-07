#include "stm32f10x.h"
#include "buzzer.h"
#include "Delay.h"

// 添加静态变量保存当前状态
static uint32_t current_frequency = 0;
static uint8_t current_volume = 100; // 默认音量100%
static bool buzzer_enabled = false;

/**
  * @brief  蜂鸣器初始化 - 支持20-5000Hz版本
  */
void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(BUZZER_GPIO_RCC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(BUZZER_TIM_RCC, ENABLE);
    
    // 配置PB1为复用推挽输出 (TIM3_CH4)
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
    
    // 定时器时基配置 - 优化预分频以支持20-5000Hz范围
    // 系统时钟72MHz，预分频144-1=143，得到500kHz计数频率
    TIM_TimeBaseStructure.TIM_Period = 4999;  // 初始ARR值
    TIM_TimeBaseStructure.TIM_Prescaler = 143;  // 72MHz/(143+1) = 500kHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(BUZZER_TIM, &TIM_TimeBaseStructure);
    
    // PWM输出配置
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OCInitStructure.TIM_Pulse = 0;
    
    TIM_OC4Init(BUZZER_TIM, &TIM_OCInitStructure);
    
    TIM_OC4PreloadConfig(BUZZER_TIM, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(BUZZER_TIM, ENABLE);
    
    TIM_Cmd(BUZZER_TIM, ENABLE);
    
    // 初始化状态变量
    current_frequency = 0;
    current_volume = 50;
    buzzer_enabled = false;
}

/**
  * @brief  设置蜂鸣器频率 - 20-5000Hz精确版本
  * @param  frequency: 声信号频率(Hz)，0表示停止
  * @note   使用500kHz计数频率，平衡高低频精度
  */
void Buzzer_SetFrequency(uint32_t frequency)
{
    if (frequency == 0) {
        BUZZER_TIM->CCR4 = 0;
        current_frequency = 0;
        buzzer_enabled = false;
        return;
    }
    
    // 确保定时器使能
    TIM_Cmd(BUZZER_TIM, ENABLE);
    
    // 限制频率范围
    if (frequency < 20) frequency = 20;
    if (frequency > 5000) frequency = 5000;
    
    // 新的计算公式（使用500kHz计数频率）：
    // 声信号频率 = 2 × 电信号频率
    // 电信号频率 = 500kHz / (ARR + 1)
    // 所以：frequency = 2 × (500000 / (ARR + 1))
    // 因此：ARR = (1000000 / frequency) - 1
    
    uint32_t arr_value;
    
    // 使用四舍五入提高精度
    arr_value = (1000000 + frequency / 2) / frequency - 1;
    
    // 限制ARR值范围
    // 最小ARR值对应最高频率：1000000/5000-1=199（5000Hz声信号）
    // 最大ARR值对应最低频率：1000000/20-1=49999（20Hz声信号）
    // 定时器ARR寄存器是16位的，最大值为65535，所以20Hz完全支持
    if (arr_value < 199) arr_value = 199;      // 最高5000Hz
    if (arr_value > 49999) arr_value = 49999;  // 最低20Hz
    
    // 设置占空比
    uint32_t pulse_value = ((arr_value + 1) * current_volume) / 100;
    if (pulse_value < 1) pulse_value = 1;
    if (pulse_value >= arr_value) pulse_value = arr_value / 2;
    
    // 更新定时器参数
    BUZZER_TIM->ARR = arr_value;
    BUZZER_TIM->CCR4 = pulse_value;
    
    current_frequency = frequency;
    buzzer_enabled = true;
}

/**
  * @brief  停止蜂鸣器
  */
void Buzzer_Stop(void)
{
    BUZZER_TIM->CCR4 = 0;
    current_frequency = 0;
    buzzer_enabled = false;
}

/**
  * @brief  蜂鸣器鸣叫
  */
void Buzzer_Beep(uint32_t frequency, uint32_t duration_ms)
{
    Buzzer_SetFrequency(frequency);
    Delay_ms(duration_ms);
    Buzzer_Stop();
}

/**
  * @brief  设置蜂鸣器音量
  * @param  volume: 音量百分比 (0-100)
  * @note   0表示静音，100表示最大音量
  */
void Buzzer_SetVolume(uint8_t volume)
{
    // 限制音量范围
    if (volume > 100) {
        volume = 100;
    }
    
    current_volume = volume;
    
    // 如果蜂鸣器当前正在发声，立即更新占空比
    if (buzzer_enabled && current_frequency > 0) {
        uint32_t arr_value = BUZZER_TIM->ARR;
        uint32_t pulse_value = ((arr_value + 1) * current_volume) / 100;
        
        if (pulse_value < 1) pulse_value = 1;
        if (pulse_value >= arr_value) pulse_value = arr_value - 1;
        
        BUZZER_TIM->CCR4 = pulse_value;
    }
}

/**
  * @brief  获取当前音量
  * @param  无
  * @retval 当前音量值 (0-100)
  */
uint8_t Buzzer_GetVolume(void)
{
    return current_volume;
}

/**
  * @brief  获取当前频率
  * @param  无
  * @retval 当前频率值
  */
uint32_t Buzzer_GetFrequency(void)
{
    return current_frequency;
}

/**
  * @brief  检查蜂鸣器是否正在发声
  * @param  无
  * @retval true-正在发声, false-停止
  */
bool Buzzer_IsPlaying(void)
{
    return buzzer_enabled;
}

/**
  * @brief  直接打开蜂鸣器（使用定时器输出100%占空比的PWM）
  * @param  无
  * @note   通过设置CCR4等于ARR实现100%占空比，输出持续高电平
  */
void Buzzer_ON(void)
{
    // 设置ARR为最小值，CCR4等于ARR，实现100%占空比
    // 使用较小的ARR值确保快速响应
    BUZZER_TIM->ARR = 1;    // 最小ARR值
    BUZZER_TIM->CCR4 = 1;   // CCR4等于ARR，100%占空比
    
    // 确保定时器使能
    TIM_Cmd(BUZZER_TIM, ENABLE);
    
    current_frequency = 0; // 标记为直接电平输出模式
    buzzer_enabled = true;
}

/**
  * @brief  直接关闭蜂鸣器（输出低电平）
  * @param  无
  */
void Buzzer_OFF(void)
{
    Buzzer_Stop();
}

/**
  * @brief  切换蜂鸣器开关状态
  * @param  无
  * @retval 切换后的状态 true-开启, false-关闭
  */
bool Buzzer_Toggle(void)
{
    if (buzzer_enabled) {
        Buzzer_OFF();
        return false;
    } else {
        Buzzer_ON();
        return true;
    }
}
