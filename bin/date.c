// bin/date.c
#include "commands.h"
#include "../drivers/screen.h"
#include "../drivers/rtc.h"
#include "../kernel/kernel.h"

static const char* weekday_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char* month_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void int_to_str(int num, char* str, int min_width) {
    char temp[16];
    int i = 0;
    
    if (num == 0) {
        temp[i++] = '0';
    } else {
        while (num > 0) {
            temp[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    int j = 0;
    while (i < min_width) {
        str[j++] = ' ';
        min_width--;
    }
    
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

void cmd_date(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    rtc_time_t time;
    rtc_get_time(&time);
    
    char buffer[128];
    char num_str[16];
    
    buffer[0] = '\0';
    
    // Weekday
    if (time.weekday >= 1 && time.weekday <= 7) {
        strcat(buffer, weekday_names[time.weekday - 1]);
    } else {
        strcat(buffer, weekday_names[0]);
    }
    strcat(buffer, " ");
    
    // Month
    if (time.month >= 1 && time.month <= 12) {
        strcat(buffer, month_names[time.month - 1]);
    } else {
        strcat(buffer, month_names[0]);
    }
    strcat(buffer, " ");
    
    // Day
    int_to_str(time.day, num_str, 2);
    strcat(buffer, num_str);
    strcat(buffer, " ");
    
    // Hour
    int_to_str(time.hour, num_str, 2);
    if (strlen(num_str) == 1) {
        strcat(buffer, "0");
    }
    strcat(buffer, num_str);
    strcat(buffer, ":");
    
    // Minute
    int_to_str(time.minute, num_str, 2);
    if (strlen(num_str) == 1) {
        strcat(buffer, "0");
    }
    strcat(buffer, num_str);
    strcat(buffer, ":");
    
    // Second
    int_to_str(time.second, num_str, 2);
    if (strlen(num_str) == 1) {
        strcat(buffer, "0");
    }
    strcat(buffer, num_str);
    strcat(buffer, " UTC ");
    
    // Year
    int_to_str(time.year, num_str, 4);
    strcat(buffer, num_str);
    strcat(buffer, "\n");
    
    screen_print(buffer);
}