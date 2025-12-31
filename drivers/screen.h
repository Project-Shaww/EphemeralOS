#ifndef SCREEN_H
#define SCREEN_H

#include "../kernel/kernel.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void screen_init(void);
void screen_clear(void);
void screen_print(const char* str);
void screen_print_char(char c);
void screen_set_color(uint8_t fg, uint8_t bg);
void screen_newline(void);
void screen_backspace(void);

#endif
