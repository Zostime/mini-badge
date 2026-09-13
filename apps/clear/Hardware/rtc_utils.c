#include "rtc_utils.h"
#include "rtc.h"

void RTC_GetDateTime(RTC_DateTime_t *dt)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    dt->hour    = sTime.Hours;
    dt->minute  = sTime.Minutes;
    dt->second  = sTime.Seconds;
    dt->weekday = sDate.WeekDay; 
    dt->day     = sDate.Date;
    dt->month   = sDate.Month;
    dt->year    = sDate.Year + 2000; 
}

void RTC_SetDateTime(RTC_DateTime_t *dt)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours   = dt->hour;
    sTime.Minutes = dt->minute;
    sTime.Seconds = dt->second;

    sDate.WeekDay = dt->weekday;
    sDate.Date    = dt->day;
    sDate.Month   = dt->month;
    sDate.Year    = dt->year - 2000;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}
