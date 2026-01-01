// drivers/rtc.c
#include "rtc.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static int is_updating(void) {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

void rtc_init(void) {
    outb(CMOS_ADDRESS, 0x0B);
    uint8_t status = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, 0x0B);
    outb(CMOS_DATA, status | 0x02);
}

void rtc_get_time(rtc_time_t* time) {
    while (is_updating());
    
    time->second = cmos_read(0x00);
    time->minute = cmos_read(0x02);
    time->hour = cmos_read(0x04);
    time->day = cmos_read(0x07);
    time->month = cmos_read(0x08);
    time->year = cmos_read(0x09);
    time->weekday = cmos_read(0x06);
    
    uint8_t registerB = cmos_read(0x0B);
    
    if (!(registerB & 0x04)) {
        time->second = bcd_to_binary(time->second);
        time->minute = bcd_to_binary(time->minute);
        time->hour = bcd_to_binary(time->hour);
        time->day = bcd_to_binary(time->day);
        time->month = bcd_to_binary(time->month);
        time->year = bcd_to_binary(time->year);
        time->weekday = bcd_to_binary(time->weekday);
    }
    
    time->year += 2000;
    
    if (!(registerB & 0x02) && (time->hour & 0x80)) {
        time->hour = ((time->hour & 0x7F) + 12) % 24;
    }
}

uint32_t rtc_get_unix_timestamp(void) {
    rtc_time_t time;
    rtc_get_time(&time);
    
    uint32_t days = 0;
    
    for (uint16_t y = 1970; y < time.year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            days += 366;
        } else {
            days += 365;
        }
    }
    
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if ((time.year % 4 == 0 && time.year % 100 != 0) || (time.year % 400 == 0)) {
        days_in_month[1] = 29;
    }
    
    for (uint8_t m = 0; m < time.month - 1; m++) {
        days += days_in_month[m];
    }
    
    days += time.day - 1;
    
    uint32_t timestamp = days * 86400;
    timestamp += time.hour * 3600;
    timestamp += time.minute * 60;
    timestamp += time.second;
    
    return timestamp;
}