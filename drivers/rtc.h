// drivers/rtc.h
#ifndef RTC_H
#define RTC_H

#include "../kernel/kernel.h"

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t weekday;
} rtc_time_t;

void rtc_init(void);
void rtc_get_time(rtc_time_t* time);
uint32_t rtc_get_unix_timestamp(void);

#endif