// drivers/screen.h
#ifndef SCREEN_H
#define SCREEN_H

#include "../kernel/kernel.h"

void screen_init(void);
void screen_clear(void);
void screen_print(const char* str);
void screen_print_char(char c);
void screen_backspace(void);
void screen_set_color(uint8_t foreground, uint8_t background);
void screen_set_row(int row);

#endif
