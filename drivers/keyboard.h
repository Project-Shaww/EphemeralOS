// drivers/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../kernel/kernel.h"

#define KEY_UP 0x48
#define KEY_DOWN 0x50

void keyboard_init(void);
char keyboard_getchar(void);
void keyboard_getline(char* buffer, int max_length);
uint8_t keyboard_get_scancode(void);

#endif
