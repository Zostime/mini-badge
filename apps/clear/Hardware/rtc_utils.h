#ifndef __RTC_UTILS_H
#define __RTC_UTILS_H

#include <stdint.h>

// 日期时间结构体
typedef struct {
    uint8_t hour;        // 0-23
    uint8_t minute;      // 0-59
    uint8_t second;      // 0-59
    uint8_t weekday;     // 1-7
    uint8_t day;         // 1-31
    uint8_t month;       // 1-12
    uint16_t year;       // 完整年份
} RTC_DateTime_t;

void RTC_GetDateTime(RTC_DateTime_t *dt);
void RTC_SetDateTime(RTC_DateTime_t *dt);

#endif
