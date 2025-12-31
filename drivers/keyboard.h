#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../kernel/kernel.h"

void keyboard_init(void);
char keyboard_getchar(void);
void keyboard_getline(char* buffer, int max_length);

#endif
