// shell/shell.h
#ifndef SHELL_H
#define SHELL_H

#include "../kernel/kernel.h"

void shell_init(void);
void shell_run(void);
char* shell_get_current_dir(void);
void shell_set_current_dir(const char* dir);
char* shell_get_username(void);

#endif
